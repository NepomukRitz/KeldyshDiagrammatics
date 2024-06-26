#ifndef KELDYSH_MFRG_INTEGRAND_HPP
#define KELDYSH_MFRG_INTEGRAND_HPP

#include <cmath>                            // for using the macro M_PI as pi
#include "../symmetries/Keldysh_symmetries.hpp"  // for independent Keldysh components and utilities
#include "../correlation_functions/four_point/vertex.hpp"                         // vertex class
#include "../correlation_functions/two_point/selfenergy.hpp"                     // self-energy class
#include "../correlation_functions/two_point/propagator.hpp"                     // propagator class
#include "../integrator/integrator.hpp"          // integration routines
#include "../utilities/util.hpp"                 // measuring time, printing text output
#include "../utilities/mpi_setup.hpp"            // mpi parallelization routines
#include "../asymptotic_corrections/correction_functions.hpp"            // correction terms due to finite integration range
#include "../utilities/write_data2file.hpp"      // write vectors into hdf5 file
#include "bubble.hpp"
#include "../multidimensional/multiarray.hpp"

//Class created for debugging of the Bubbles
template <typename Q>
class IntegrandBubble{
    const Propagator<Q>& g1;
    const Propagator<Q>& g2;
    bool diff;
    freqType w;
    int iK;
    const char channel;

public:
    /**
     * Constructor for the IntegrandBubble
     * @param g1_in     : Propagator object for the lower (right) leg
     * @param g2_in     : Propagator object for the upper (left) leg
     * @param diff_in   : Boolean defining whether bubble is differentiated or not
     * @param w_in      : Transfer frequency at which the bubble should be integrated
     * @param iK_in     : Keldysh index to be taken
     * @param channel_in: Char indicating the channel in which the bubble should be calculated and which determines the frequency transformations
     */
    IntegrandBubble(const Propagator<Q>& g1_in, const Propagator<Q>& g2_in, bool diff_in, freqType w_in, int iK_in, char channel_in)
            : g1(g1_in), g2(g2_in), diff(diff_in), w(w_in), iK(iK_in), channel(channel_in) {};

    /**
     * Call operator
     * @param vpp : v'', the frequency over which is being integrated
     * @return The value g1(v1)*g2(v2), where v1 and v2 are calculated according to the channel. The components of the
     * propagators taken depend on the Keldysh component
     */
    auto operator() (freqType vpp) const -> Q {
        Q ans;
        freqType v1, v2;
        Bubble<Q> Pi(g1, g2, diff);

        switch(channel){
            case 'p':
                v1 = w/2.+vpp;
                v2 = w/2.-vpp;
                break;
            case 'a': case 't':
                v1 = vpp-w/2.;
                v2 = vpp+w/2.;
                break;
            default:
                v1 = 0.;
                v2 = 0.;
                utils::print("Error in IntegrandBubble! Abort.");
                assert(false);
        }
        //Make reference to the Bubble object of the actual code, making this into a useful test of code correctnes and compliance
        return Pi.value(iK, v1, v2, 0)/(2.*M_PI*glb_i);
    }
};


/// Refactoring of the classes Integrand_K1, Integrand_K2, Integrand_K3 into one single class

/**
 * Integrand invoked by the BubbleFunctionCalculator class for every combination of external arguments.
 * @tparam diag_class Diagrammatic class to be computed (K1, K2, K2' or K3).
 * @tparam channel Two-particle channel of the computation. Can be a, p, or t.
 * @tparam spin Spin index to be computed.
 * @tparam Q Type of the data.
 * @tparam vertexType_left Type of the vertex that enters as the left part of the computation.
 * @tparam vertexType_right Type of the vertex that enters as the right part of the computation.
 * @tparam Bubble_Object Type of the Bubble object to connect the two vertices.
 * @tparam return_type Type of the data that is produced by the integral. Typically = Q.
 */
template <K_class diag_class, char channel, int spin, typename Q,
        typename vertexType_left,
        typename vertexType_right,
        class Bubble_Object,
        typename return_type = Q>
class Integrand {
private:
    const vertexType_left& vertex1;
    const vertexType_right& vertex2;
    const Bubble_Object& Pi;
    int i0_symmred;
    int i0 = 0;
    int i0_left;
    int i0_right;
    const int i2;
    const int iw=0;
    const freqType w, v = 0., vp = 0.;
    const int i_in;
    const int i_spin;
    const bool diff;
public:
    const VertexInput input_external; // vertex parameters of the component(s) that should be computed (dgamma)

private:
#if KELDYSH_FORMALISM
    using buffer_type_vertex_l = Eigen::Matrix<Q,myRowsAtCompileTime<return_type>(),4>;
    using buffer_type_vertex_r = Eigen::Matrix<Q,4,myColsAtCompileTime<return_type>()>; // buffer_type_vertex_l;
#else
    using buffer_type_vertex_l = Q;
    using buffer_type_vertex_r = Q;
#endif

    //K_class diag_class;

    Q res_l_V_initial, res_r_V_initial, res_l_Vhat_initial, res_r_Vhat_initial; // To be precomputed for K1

    void set_Keldysh_index_i0(int i0_in);
    void set_Keldysh_index_i0_left_right(int i0_in);
    void precompute_vertices();

    bool case_always_has_to_be_zero() const;

    void compute_vertices(freqType vpp, Q& res_l_V, Q& res_r_V, Q& res_l_Vhat, Q& res_r_Vhat) const;

    template<int ispin> void load_vertex_keldyshComponents_left_scalar (buffer_type_vertex_l& values_vertex, const VertexInput& input) const;
    template<int ispin> void load_vertex_keldyshComponents_right_scalar(buffer_type_vertex_r& values_vertex, const VertexInput& input) const;
    template<int ispin> void load_vertex_keldyshComponents_left_vectorized (buffer_type_vertex_l& values_vertex, const VertexInput& input) const;
    template<int ispin> void load_vertex_keldyshComponents_right_vectorized(buffer_type_vertex_r& values_vertex, const VertexInput& input) const;

public:
    auto load_vertex_keldysh_and_spin_Components_left_vectorized (const VertexInput& input) const -> Eigen::Matrix<Q, 1, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1>;
    auto load_vertex_keldysh_and_spin_Components_right_vectorized(const VertexInput& input) const -> Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>;
    auto load_Pi_keldysh_and_spin_Components_vectorized(const freqType input) const -> Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>;

private:
    Q sum_over_internal_scalar(freqType vpp) const;
    return_type sum_over_internal_vectorized(freqType vpp) const;
        public:

    /**
     * Constructor for asymptotic class Ki:
     * @param vertex1_in : left vertex
     * @param vertex2_in : right vertex
     * @param Pi_in      : Bubble object connecting the left and right vertex
     * @param i0_in      : index specifying the (external) Keldysh component of integrand object; i0_in = [0, .. nK_Ki]
     *                     where nK_Ki is the number of symmetry-reduced Keldysh components in the result
     *                     (converted into actual Keldysh index i0 within the constructor)
     * @param w_in       : external bosonic frequency ω
     * @param i_in_in    : external index for internal structure
     * @param ch_in      : diagrammatic channel ('a', 'p', 't')
     * @param diff_in    : determines whether to compute differentiated or non-differentiated bubble
     */
    Integrand(const vertexType_left& vertex1_in,
              const vertexType_right& vertex2_in,
              const Bubble_Object& Pi_in,
              int i0_in, int i2_in, const int iw_in, const freqType w_in, const freqType v_in, const freqType vp_in, const int i_in_in,
              const int i_spin_in, const bool diff_in)
              :vertex1(vertex1_in), vertex2(vertex2_in), Pi(Pi_in), i0_symmred(i0_in),
              i2(i2_in), iw(iw_in), w(w_in), v(v_in), vp(vp_in), i_in(i_in_in), i_spin(i_spin_in), diff(diff_in),
               input_external(i0_in, 0, w_in, v_in, vp_in, i_in_in, channel, diag_class, iw_in) {
        set_Keldysh_index_i0(i0_in);
        if constexpr(MAX_DIAG_CLASS < 2 and ! VECTORIZED_INTEGRATION) {precompute_vertices();}
    }

    /**
     * Call operator:
     * @param vpp : frequency at which to evaluate integrand (to be integrated over)
     * @return Q  : value of the integrand object evaluated at frequency vpp (comp or double)
     */
    auto operator() (freqType vpp) const -> return_type;

    void save_integrand() const;
    void save_integrand(const rvec& freqs, const std::string& filename_prefix) const;
    void get_integrand_vals(const rvec& freqs, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& integrand_vals, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& Pivals, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& vertex_vals1, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& vertex_vals2)  const;

    auto get_projection_lambdaBar() const -> Eigen::Matrix<Q,4,4> {
        Eigen::Matrix<Q,4,4> result;
        if constexpr(CONTOUR_BASIS) {
            result << 1, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 1;
        }
        else {
            result << 1, 0, 0, 1,
                    0, 1, 1, 0,
                    0, 1, 1, 0,
                    1, 0, 0, 1;
            result *= 0.5;
        }
        return result;
    }
    auto get_projection_lambda() const -> Eigen::Matrix<Q,4,4> {
        Eigen::Matrix<Q,4,4> result;
        if constexpr(CONTOUR_BASIS) {
            result << 1, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 1;
        }
        else {
            result << 1, 0, 0, 1,
                    0, 1, 1, 0,
                    0, 1, 1, 0,
                    1, 0, 0, 1;
            result *= 0.5;
        }
        return result;
    }
    const Eigen::Matrix<Q,4,4> projection_lambdaBar = get_projection_lambdaBar();
    const Eigen::Matrix<Q,4,4> projection_lambda = get_projection_lambda();

};


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::set_Keldysh_index_i0(const int i0_in) {
if constexpr(KELDYSH){
   i0 = i0_in;
   set_Keldysh_index_i0_left_right(i0);
   }
   else{
       i0 = 0;
       i0_left = 0;
       i0_right= 0;
   }
}

// i0_in in {0,...,15}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::set_Keldysh_index_i0_left_right(const int i0_in) {
    my_index_t left, right;
    get_i0_left_right<channel>(i0_in, left, right);
    i0_left  = left * 4; //
    i0_right = right * 4; //
}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::precompute_vertices() {
    // For K1 class, left and right vertices do not depend on integration frequency
    // -> precompute them to save time
    if constexpr(not DEBUG_SYMMETRIES) {
#if KELDYSH_FORMALISM
        std::vector<int> indices = indices_sum(i0, i2, channel);

        VertexInput input_l(indices[0], spin, w, 0., 0., i_in, channel);
        VertexInput input_r(indices[1], spin, w, 0., 0., i_in, channel);
#else
        VertexInput input_l (0, spin, w, 0., 0., i_in, channel);
        VertexInput &input_r = input_l;
#endif
        res_l_V_initial = vertex1.template left_same_bare<channel>(input_l);
        res_r_V_initial = vertex2.template right_same_bare<channel>(input_r);
        if (channel == 't') {
            input_l.spin = 1;
            input_r.spin = 1;
            res_l_Vhat_initial = vertex1.template left_same_bare<channel>(input_l);
            res_r_Vhat_initial = vertex2.template right_same_bare<channel>(input_r);
        }
    }
    else {// DEBUG_SYMMETRIES

    #if KELDYSH_FORMALISM
        std::vector<int> indices = indices_sum(i0, i2, channel);
        VertexInput input_l(indices[0], spin, w, 0., 0., i_in, channel);
        VertexInput input_r(indices[1], spin, w, 0., 0., i_in, channel);
    #else
        VertexInput input_l (0, spin, w, 0., 0., i_in, channel);
        VertexInput &input_r = input_l;
    #endif
        res_l_V_initial = vertex1.template left_same_bare<channel>(input_l);
        res_r_V_initial = vertex2.template right_same_bare<channel>(input_r);
        if (channel == 't' and spin == 0) {
            input_l.spin = 1 - input_l.spin; // flip spin 0 <-> 1
            input_r.spin = 1 - input_r.spin; // flip spin 0 <-> 1
            res_l_Vhat_initial = vertex1.template left_same_bare<channel>(input_l);
            res_r_Vhat_initial = vertex2.template right_same_bare<channel>(input_r);
        } else if (channel == 'a' and spin == 1) {
            input_l.spin = 1 - input_l.spin; // flip spin 0 <-> 1
            input_r.spin = 1 - input_r.spin; // flip spin 0 <-> 1
            res_l_Vhat_initial = vertex1.template left_same_bare<channel>(input_l);
            res_r_Vhat_initial = vertex2.template right_same_bare<channel>(input_r);
        } else if (channel == 'p' and spin == 1) {
            input_l.spin = 1 - input_l.spin; // flip spin 0 <-> 1
            input_r.spin = 1 - input_r.spin; // flip spin 0 <-> 1
            res_l_Vhat_initial = vertex1.template left_same_bare<channel>(input_l);
            res_r_Vhat_initial = vertex2.template right_same_bare<channel>(input_r);
        }
    } // DEBUG_SYMMETRIES
}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
auto Integrand<diag_class, channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::operator()(freqType vpp) const -> return_type {
    return_type result;
#if not SWITCH_SUM_N_INTEGRAL

    if (case_always_has_to_be_zero()) {return 0.;}
    Q res_l_V, res_r_V, res_l_Vhat, res_r_Vhat;
    compute_vertices(vpp, res_l_V, res_r_V, res_l_Vhat, res_r_Vhat);

    Q Pival = Pi.value(i2, w, vpp, i_in, channel);

    if (channel != 't')  // no spin sum in a and p channel
        result = res_l_V * Pival * res_r_V;
    else {
        // in t channel, spin sum has 3 terms:
        // result = res_l_V * Pival * (res_r_V + res_r_Vhat) + (res_l_V + res_l_Vhat) * Pival * res_r_V;
        //        = 2. * res_l_V * Pival * res_r_V + res_l_V * Pival * res_r_Vhat + res_l_Vhat * Pival * res_r_V;
        switch (i_spin) {
            case 0:
                result = 2. * res_l_V * Pival * res_r_V;
                break;
            case 1:
                result = res_l_V * Pival * res_r_Vhat;
                break;
            case 2:
                result = res_l_Vhat * Pival * res_r_V;
                break;
            default:;
        }
    }
#if DEBUG_SYMMETRIES
    if (spin == 0 and channel ==  'p') {
        result = (res_l_V * Pival * res_r_V);
    }
    if (spin == 1) {
        if (channel == 't')  // no spin sum in t channel
            result = res_l_V * Pival * res_r_V;
        else if (channel == 'p') {
            result = (res_l_V * Pival * res_r_Vhat);
        }
        else { // channel == 'a'
            // in a channel, spin sum has 3 terms:
            // result = res_l_V * Pival * (res_r_V + res_r_Vhat) + (res_l_V + res_l_Vhat) * Pival * res_r_V;
            //        = 2. * res_l_V * Pival * res_r_V + res_l_V * Pival * res_r_Vhat + res_l_Vhat * Pival * res_r_V;
            switch (i_spin) {
                case 0:
                    result = 2. * res_l_V * Pival * res_r_V;
                    break;
                case 1:
                    result = res_l_V * Pival * res_r_Vhat;
                    break;
                case 2:
                    result = res_l_Vhat * Pival * res_r_V;
                    break;
                default:;
            }
        }
    }
#endif // DEBUG_SYMMETRIES


#else

    //result = sum_over_internal_scalar(vpp);
    if constexpr(KELDYSH)
    {
        result = sum_over_internal_vectorized(vpp);
    }
    else {
        result = sum_over_internal_scalar(vpp);
    }
    /// comment in to test vectorized access
    //Q result2 = sum_over_internal_scalar(vpp);
    //assert(std::abs(result-result2) < 1e-10);

#endif

    return result;
}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
bool Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::case_always_has_to_be_zero() const {
    bool zero_result = false;
    if (KELDYSH && (MAX_DIAG_CLASS <= 1)){
        if (!diff) {
            switch (channel) {
                case 'a':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 11 && i2 != 13)) {zero_result = true;}
                    if (i0 == 3 &&
                        (i2 != 6 && i2 != 7 && i2 != 9 && i2 != 11 && i2 != 13 && i2 != 14 && i2 != 15))
                        {zero_result = true;}
                    break;
                case 'p':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 7 && i2 != 11)) {zero_result = true;}
                    if (i0 == 5 &&
                        (i2 != 3 && i2 != 7 && i2 != 11 && i2 != 12 && i2 != 13 && i2 != 14 && i2 != 15))
                        {zero_result = true;}
                    break;
                case 't':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 11 && i2 != 13)) {zero_result = true;}
                    if (i0 == 3 &&
                        (i2 != 6 && i2 != 7 && i2 != 9 && i2 != 11 && i2 != 13 && i2 != 14 && i2 != 15))
                        {zero_result = true;}
                    break;
                default:;
            }
        }
    }
    return zero_result;
}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::compute_vertices(const freqType vpp,
                                                                                  Q& res_l_V, Q& res_r_V,
                                                                                  Q& res_l_Vhat, Q& res_r_Vhat) const{
    if (MAX_DIAG_CLASS <= 1){
        res_l_V = res_l_V_initial;
        res_r_V = res_r_V_initial;
        res_l_Vhat = res_l_Vhat_initial;
        res_r_Vhat = res_r_Vhat_initial;
    }
    else{
        std::vector<int> indices = indices_sum(i0, i2, channel);
        VertexInput input_l (indices[0], spin, w, v, vpp,  i_in, channel, diag_class, iw);
        VertexInput input_r (indices[1], spin, w, vpp, vp, i_in, channel, diag_class, iw);

        if (i_spin == 0) { // first summand in all channels is res_l_V * Pival * res_r_V
            if (diag_class == k1 or diag_class == k2b)
                res_l_V = vertex1.template left_same_bare<channel>(input_l);
            else
                res_l_V = vertex1.template left_diff_bare<channel>(input_l);

            if (diag_class == k3 or diag_class == k2b)
                res_r_V = vertex2.template right_diff_bare<channel>(input_r);
            else
                res_r_V = vertex2.template right_same_bare<channel>(input_r);
#if DEBUG_SYMMETRIES
            if (channel == 'p') {  // res_l_Vhat * Pival * res_r_Vhat
                // compute res_l_Vhat
                input_l.spin = 1 - spin;
                if (diag_class == k1 or diag_class == k2b)
                    res_l_Vhat = vertex1.template left_same_bare<channel>(input_l);
                else
                    res_l_Vhat = vertex1.template left_diff_bare<channel>(input_l);
                // compute res_r_Vhat
                input_r.spin = 1 - spin;
                if (diag_class == k3 or diag_class == k2b)
                    res_r_Vhat = vertex2.template right_diff_bare<channel>(input_r);
                else
                    res_r_Vhat = vertex2.template right_same_bare<channel>(input_r);
            }
#endif
        }
        else { // relevant for t-channel (spin component 0) and a-channel (spin component 1): there i_spin = 0, 1, 2

            if constexpr(channel == 't'
#if DEBUG_SYMMETRIES
                or channel == 'a'
#endif
            ) {
                // channel = t, i_spin = 1
                if (i_spin == 1) {  // res_l_V * Pival * res_r_Vhat
                    // compute res_l_V
                    if (diag_class == k1 or diag_class == k2b)
                        res_l_V = vertex1.template left_same_bare<channel>(input_l);
                    else
                        res_l_V = vertex1.template left_diff_bare<channel>(input_l);
                    // compute res_r_Vhat
                    input_r.spin = 1 - spin;
                    if (diag_class == k3 or diag_class == k2b)
                        res_r_Vhat = vertex2.template right_diff_bare<channel>(input_r);
                    else
                        res_r_Vhat = vertex2.template right_same_bare<channel>(input_r);
                }
                // channel = t, i_spin = 2
                else {              // res_l_Vhat * Pival * res_r_V
                    assert(i_spin == 2);
                    // compute res_r_V
                    if (diag_class == k3 or diag_class == k2b)
                        res_r_V = vertex2.template right_diff_bare<channel>(input_r);
                    else
                        res_r_V = vertex2.template right_same_bare<channel>(input_r);
                    // compute res_l_Vhat
                    input_l.spin = 1 - spin;
                    if (diag_class == k1 or diag_class == k2b)
                        res_l_Vhat = vertex1.template left_same_bare<channel>(input_l);
                    else
                        res_l_Vhat = vertex1.template left_diff_bare<channel>(input_l);
                }
            }
        }
    }
}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
template<int ispin>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldyshComponents_left_scalar(buffer_type_vertex_l& values_vertex, const VertexInput& input) const {
    //size_t len_1 = values_vertex.length()[0];
    const auto vertexvalue_scalar = [&] (const VertexInput& input_l) {if constexpr(diag_class == k1 or diag_class == k2b) return vertex1.template left_same_bare_symmetry_expanded<ispin,channel,Q>(input_l) ; else return vertex1.template left_diff_bare_symmetry_expanded<ispin,channel,Q>(input_l);};
    //const auto vertexvalue_scalar = [&] (const VertexInput& input_l) {if constexpr(diag_class == k1 or diag_class == k2b) return vertex1.template left_same_bare<channel>(input_l) ; else return vertex1.template left_diff_bare<channel>(input_l);};
    //assert(len_1 == glb_number_of_Keldysh_components_bubble);

    if constexpr(not KELDYSH) {
        values_vertex =  vertexvalue_scalar(input);
    }
    else {

        // fill v_temp:
        const std::array<size_t,4> dims_K = {2,2, 2,2};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                VertexInput input_tmp = input;
                input_tmp.iK = input.iK + i*2+j;

                values_vertex[i*2+j] = vertexvalue_scalar(input_tmp);
            }
        }
    }

}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
template<int ispin>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldyshComponents_right_scalar(buffer_type_vertex_r& values_vertex, const VertexInput& input) const {
    const auto vertexvalue_scalar = [&](const VertexInput& input_r) {if constexpr(diag_class == k3 or diag_class == k2b) return vertex2.template right_diff_bare_symmetry_expanded<ispin,channel,Q>(input_r); else return vertex2.template right_same_bare_symmetry_expanded<ispin,channel,Q>(input_r);};

    if constexpr(not KELDYSH) {
        values_vertex =  vertexvalue_scalar(input);
    }
    else {
        const std::array<size_t, 4> dims_K = {2, 2, 2, 2};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                VertexInput input_tmp = input;
                input_tmp.iK = input.iK + i*2+j;

                values_vertex[i*2+j] = vertexvalue_scalar(input_tmp);
            }
        }
    }
}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
template<int ispin>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldyshComponents_left_vectorized(buffer_type_vertex_l& values_vertex, const VertexInput& input) const {
    using result_type_fetch = Eigen::Matrix<Q, 4 * myRowsAtCompileTime<return_type>(),1>;
    const auto vertexvalue_vector = [&] (const VertexInput& input_l) {if constexpr(diag_class == k1 or diag_class == k2b) return vertex1.template left_same_bare_symmetry_expanded<ispin,channel,result_type_fetch>(input_l) ; else return vertex1.template left_diff_bare_symmetry_expanded<ispin,channel,result_type_fetch>(input_l);};

    if constexpr(not KELDYSH) {
        values_vertex = vertexvalue_vector(input);
    }
    else {
        Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> result_temp = vertexvalue_vector(input);
        result_temp.resize(4, myRowsAtCompileTime<return_type>());
        values_vertex = result_temp.transpose();
    }

}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
template<int ispin>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldyshComponents_right_vectorized(buffer_type_vertex_r& values_vertex, const VertexInput& input) const {
    //size_t len_1 = values_vertex.length()[0];
    using result_type_fetch = Eigen::Matrix<Q, 4 * myColsAtCompileTime<return_type>(), 1>;
    const auto vertexvalue_vector = [&](const VertexInput& input_r) {if constexpr(diag_class == k3 or diag_class == k2b) return vertex2.template right_diff_bare_symmetry_expanded<ispin,channel,result_type_fetch>(input_r); else return vertex2.template right_same_bare_symmetry_expanded<ispin,channel,result_type_fetch>(input_r);};
    //assert(len_1 == glb_number_of_Keldysh_components_bubble);

    if constexpr(not KELDYSH) {
        //static_assert(KELDYSH, "Currently vectorized integrand only available for Keldysh.");
        values_vertex = vertexvalue_vector(input);
    }
    else {

        Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> result_temp = vertexvalue_vector(input);
        result_temp.resize(4, myColsAtCompileTime<return_type>());
        values_vertex = result_temp;
    }

}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
auto Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldysh_and_spin_Components_left_vectorized(const VertexInput& input) const -> Eigen::Matrix<Q, 1, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1> {
    assert(!KELDYSH and !ZERO_T);

    using result_type = Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>;
    result_type result;

    if constexpr(SBE_DECOMPOSITION) {
        using result_type_fetch = Q;


        if constexpr((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                )
        {
            result_type_fetch values_vertex_l;
            result_type_fetch values_vertex_l_upup;
            load_vertex_keldyshComponents_left_scalar<spin>(values_vertex_l, input);
            load_vertex_keldyshComponents_left_scalar<2>(values_vertex_l_upup, input);

            if constexpr(diag_class == k1) {
                const Q K1L    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);

                const Q K1L_upup    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);


                result(0) = K1L_upup * values_vertex_l
                          + K1L      * values_vertex_l_upup
                          ;
                result(1) = K1L_upup * values_vertex_l_upup
                          + K1L      * values_vertex_l
                        ;
            }
            else {
                result_type_fetch values_vertex_l;
                result_type_fetch values_vertex_l_upup;
                load_vertex_keldyshComponents_left_scalar<spin>(values_vertex_l, input);
                load_vertex_keldyshComponents_left_scalar<2>(values_vertex_l_upup, input);
                result(0) = values_vertex_l;
                result(1) = values_vertex_l_upup;
            }


        }
#if DEBUG_SYMMETRIES
            else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_l values_vertex_l;
            buffer_type_vertex_l values_vertex_l_other;

            load_vertex_keldyshComponents_left_scalar<    spin>(values_vertex_l      , input);
            load_vertex_keldyshComponents_left_scalar<1 - spin>(values_vertex_l_other, input);


            if constexpr(diag_class == k1) {
                const Q K1L          = vertex1.template get_w_r_value_symmetry_expanded_nondiff<  spin, channel, channel, diag_class, result_type_fetch>(input_external);
                result(0) = K1L * values_vertex_l_other;
            }
            else if constexpr (diag_class == k2){
                result(0) = values_vertex_l_other;
            }
            else {
                result(0) = values_vertex_l;
            }


        }
#endif
        else {
            buffer_type_vertex_l values_vertex_l;

            load_vertex_keldyshComponents_left_scalar<    spin>(values_vertex_l, input);

            if constexpr(diag_class == k1) {
                const Q K1L    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                result(0) = K1L * values_vertex_l;
            }
            else {
                result(0) = values_vertex_l;
            }
        }
    } // SBE_DECOMPOSITION
    else{
        // load other spin component
        if constexpr((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                )
        {

            buffer_type_vertex_l values_vertex_l;
            buffer_type_vertex_l values_vertex_l_upup;

            load_vertex_keldyshComponents_left_scalar<spin>(values_vertex_l, input);
            load_vertex_keldyshComponents_left_scalar<2>(values_vertex_l_upup, input);

            result(0) = values_vertex_l;
            result(1) = values_vertex_l_upup;
        }
#if DEBUG_SYMMETRIES
            else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_l values_vertex_l;

            load_vertex_keldyshComponents_left_scalar<spin>(values_vertex_l, input);

            result(0) = values_vertex_l;
        }
#endif
        else {
            buffer_type_vertex_l values_vertex_l;

            load_vertex_keldyshComponents_left_scalar<spin>(values_vertex_l, input);
            result(0) = values_vertex_l;
        }
    }

    return result;
}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
auto Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_vertex_keldysh_and_spin_Components_right_vectorized(const VertexInput& input) const -> Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>{
    assert(!KELDYSH and !ZERO_T);

    using result_type = Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>;
    result_type result;


    if constexpr(SBE_DECOMPOSITION) {
        using result_type_fetch = Q;


        if constexpr((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                )
        {
            result_type_fetch values_vertex_r;
            result_type_fetch values_vertex_r_upup;
            load_vertex_keldyshComponents_right_scalar<spin>(values_vertex_r, input);
            load_vertex_keldyshComponents_right_scalar<2>(values_vertex_r_upup, input);

            if constexpr(diag_class == k1) {
                const Q K1R    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);

                const Q K1R_upup    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);


                result(1) = values_vertex_r      * K1R_upup
                          + values_vertex_r_upup * K1R
                        ;
                result(0) =  values_vertex_r_upup * K1R_upup
                          +  values_vertex_r      * K1R
                        ;
            }
            else {
                result_type_fetch values_vertex_r;
                result_type_fetch values_vertex_r_upup;
                load_vertex_keldyshComponents_right_scalar<spin>(values_vertex_r, input);
                load_vertex_keldyshComponents_right_scalar<2   >(values_vertex_r_upup, input);
                result(0) = values_vertex_r_upup;
                result(1) = values_vertex_r;
            }


        }
#if DEBUG_SYMMETRIES
            else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_l values_vertex_r;
            buffer_type_vertex_l values_vertex_r_other;

            load_vertex_keldyshComponents_right_scalar<    spin>(values_vertex_r      , input);
            load_vertex_keldyshComponents_right_scalar<1 - spin>(values_vertex_r_other, input);


            if constexpr(diag_class == k1) {
                const Q K1R          = vertex2.template get_w_r_value_symmetry_expanded_nondiff<  spin, channel, channel, diag_class, result_type_fetch>(input_external);
                const Q K1R_other    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<1-spin, channel, channel, diag_class, result_type_fetch>(input_external);

                result(0) = values_vertex_r_other * K1R_other;
            }
            else if constexpr (diag_class == k2){
                result(0) = values_vertex_r;
            }
            else {
                result(0) = values_vertex_r_other;
            }


        }
#endif
        else {
            buffer_type_vertex_r values_vertex_r;

            load_vertex_keldyshComponents_right_scalar<    spin>(values_vertex_r, input);

            if constexpr(diag_class == k1) {
                const Q K1R    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                result(0) = values_vertex_r * K1R;
            }
            else {
                result(0) = values_vertex_r;
            }
        }
    } // SBE_DECOMPOSITION
    else{
        // load other spin component
        if constexpr((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                )
        {

            buffer_type_vertex_r values_vertex_r;
            buffer_type_vertex_r values_vertex_r_upup;

            load_vertex_keldyshComponents_right_scalar<spin>(values_vertex_r, input);
            load_vertex_keldyshComponents_right_scalar<2   >(values_vertex_r_upup, input);

            result(0) = values_vertex_r_upup;
            result(1) = values_vertex_r;
        }
#if DEBUG_SYMMETRIES
            else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_r values_vertex_r_other;

            load_vertex_keldyshComponents_right_scalar<1 - spin>(values_vertex_r_other, input);

            result(0) = values_vertex_r_other;
        }
#endif
        else {
            buffer_type_vertex_r values_vertex_r;

            load_vertex_keldyshComponents_right_scalar<spin>(values_vertex_r, input);
            result(0) = values_vertex_r;
        }
    }

    return result;
}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
auto Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::load_Pi_keldysh_and_spin_Components_vectorized(const freqType vpp) const -> Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1> {

    using result_type = Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1>;
    auto Pi_matrix = Pi.template value_vectorized<channel>(input_external.w, vpp, input_external.i_in);
    result_type result;

    if constexpr ((channel == 't' and spin == 0) or (channel == 'a' and spin == 1)) {
        result << Pi_matrix, Pi_matrix;
    }
    else {
        result(0) = Pi_matrix;
    }

    return result;
}



template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
Q Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::sum_over_internal_scalar(const freqType vpp) const {

    VertexInput input_l = input_external, input_r = input_external;
    input_l.v2 = vpp; input_r.v1 = vpp;
    input_l.iK = i0_left; input_r.iK = i0_right;
    input_l.spin = 0; input_r.spin = 0;

    // create multiarrays to store loaded values
    buffer_type_vertex_l values_vertex_l;
    buffer_type_vertex_r values_vertex_r;
    auto Pi_matrix = Pi.template value_vectorized<channel>(input_external.w, vpp, input_external.i_in);
    // load vertex values:
    load_vertex_keldyshComponents_left_scalar<spin> (values_vertex_l, input_l);
    load_vertex_keldyshComponents_right_scalar<spin>(values_vertex_r, input_r);


    Q result;
    // load other spin component
    if constexpr(SBE_DECOMPOSITION) {
        using result_type_fetch = Q;


        if constexpr((channel == 't' and spin == 0)
                     #if DEBUG_SYMMETRIES
                     or (channel == 'a' and spin == 1)
                    #endif
            )
        {

            buffer_type_vertex_l values_vertex_l_upup;
            buffer_type_vertex_r values_vertex_r_upup;

            load_vertex_keldyshComponents_left_scalar<2>(values_vertex_l_upup, input_l);
            load_vertex_keldyshComponents_right_scalar<2>(values_vertex_r_upup, input_r);

            if constexpr(diag_class == k1) {
                const Q K1L    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                const Q K1R    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);


                const Q K1L_upup    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);
                const Q K1R_upup    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);



                result = K1L_upup * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r_upup) * K1R
                       + K1L_upup * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r     ) * K1R_upup
                       + K1L_upup * (values_vertex_l     ) * Pi_matrix * (values_vertex_r_upup) * K1R_upup
                       + K1L_upup * (values_vertex_l     ) * Pi_matrix * (values_vertex_r     ) * K1R
                       + K1L      * (values_vertex_l     ) * Pi_matrix * (values_vertex_r_upup) * K1R
                       + K1L      * (values_vertex_l     ) * Pi_matrix * (values_vertex_r     ) * K1R_upup
                       + K1L      * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r_upup) * K1R_upup
                       + K1L      * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r     ) * K1R
                ;
            }
            else {
                result = (values_vertex_l_upup * Pi_matrix * values_vertex_r
                        + values_vertex_l      * Pi_matrix * values_vertex_r_upup);
            }


        }
#if DEBUG_SYMMETRIES
        else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_l values_vertex_l_other;
            buffer_type_vertex_r values_vertex_r_other;

            load_vertex_keldyshComponents_left_scalar<1 - spin>(values_vertex_l_other, input_l);
            load_vertex_keldyshComponents_right_scalar<1 - spin>(values_vertex_r_other, input_r);


            if constexpr(diag_class == k1) {
                const Q K1L          = vertex1.template get_w_r_value_symmetry_expanded_nondiff<  spin, channel, channel, diag_class, result_type_fetch>(input_external);
                const Q K1R_other    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<1-spin, channel, channel, diag_class, result_type_fetch>(input_external);

                result = (K1L * values_vertex_l_other * Pi_matrix * values_vertex_r_other * K1R_other);
            }
            else if constexpr (diag_class == k2){
                result = (values_vertex_l_other * Pi_matrix * values_vertex_r);
            }
            else {
                result = (values_vertex_l * Pi_matrix * values_vertex_r_other);
            }


        }
#endif
        else {
            if constexpr(diag_class == k1) {
                const Q K1L    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                const Q K1R    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);

                result = (K1L * values_vertex_l * Pi_matrix * values_vertex_r * K1R);
            }
            else {
                result = (values_vertex_l * Pi_matrix * values_vertex_r);
            }
        }
    } // SBE_DECOMPOSITION
    else{
        // load other spin component
        if constexpr((channel == 't' and spin == 0)
                     #if DEBUG_SYMMETRIES
                     or (channel == 'a' and spin == 1)
                    #endif
            )
        {

            buffer_type_vertex_l values_vertex_l_upup;
            buffer_type_vertex_r values_vertex_r_upup;

            load_vertex_keldyshComponents_left_scalar<2>(values_vertex_l_upup, input_l);
            load_vertex_keldyshComponents_right_scalar<2>(values_vertex_r_upup, input_r);
            if constexpr(KELDYSH)
            {
                result = (values_vertex_l_upup * Pi_matrix * values_vertex_r +
                          values_vertex_l      * Pi_matrix * values_vertex_r_upup).eval()[0];
            }
            else {
                result = (values_vertex_l_upup * Pi_matrix * values_vertex_r +
                          values_vertex_l      * Pi_matrix * values_vertex_r_upup);
            }
        }
        #if DEBUG_SYMMETRIES
        else  if constexpr(channel == 'p' and spin == 1)
        {
            buffer_type_vertex_l values_vertex_l_other;
            buffer_type_vertex_r values_vertex_r_other;

            load_vertex_keldyshComponents_left_scalar<1 - spin>(values_vertex_l_other, input_l);
            load_vertex_keldyshComponents_right_scalar<1 - spin>(values_vertex_r_other, input_r);

            result = (values_vertex_l * Pi_matrix * values_vertex_r_other);
        }
        #endif
        else {
            result = values_vertex_l * Pi_matrix * values_vertex_r;
        }
    }


    return result;
}


template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
return_type Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::sum_over_internal_vectorized(const freqType vpp) const {

    VertexInput input_l = input_external, input_r = input_external;
    input_l.v2 = vpp; input_r.v1 = vpp;
    input_l.iK = i0_left; input_r.iK = i0_right;
    input_l.spin = 0;
    input_r.spin = 0;

    // create multiarrays to store loaded values
    buffer_type_vertex_l values_vertex_l;
    buffer_type_vertex_r values_vertex_r;

    auto Pi_matrix = Pi.template value_vectorized<channel>(input_external.w, vpp, input_external.i_in);
    // load vertex values:
    load_vertex_keldyshComponents_left_vectorized <spin>(values_vertex_l, input_l);
    load_vertex_keldyshComponents_right_vectorized<spin>(values_vertex_r, input_r);


    // load other spin component
    if constexpr(SBE_DECOMPOSITION) {

        using result_type_fetch = Eigen::Matrix<Q, 4 * myRowsAtCompileTime<return_type>(),1>;


        if constexpr ((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                ) {

            buffer_type_vertex_l values_vertex_l_upup;
            buffer_type_vertex_r values_vertex_r_upup;

            load_vertex_keldyshComponents_left_vectorized<2>(values_vertex_l_upup, input_l);
            load_vertex_keldyshComponents_right_vectorized<2>(values_vertex_r_upup, input_r);

            if constexpr(VECTORIZED_INTEGRATION) {
                assert(input_external.iK == 0);
                if constexpr(diag_class == k1) {
                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1L_temp    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1R_temp    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                    K1L_temp.resize(4, 4);
                    K1R_temp.resize(4, 4);

                    const Eigen::Matrix<Q,4,4> K1L = K1L_temp.transpose();
                    const Eigen::Matrix<Q,4,4> K1R = K1R_temp;

                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1L_temp_upup    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);
                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1R_temp_upup    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<2, channel, channel, diag_class, result_type_fetch>(input_external);

                    K1L_temp_upup.resize(4, 4);
                    K1R_temp_upup.resize(4, 4);

                    const Eigen::Matrix<Q,4,4> K1L_upup = K1L_temp_upup.transpose();
                    const Eigen::Matrix<Q,4,4> K1R_upup = K1R_temp_upup;


                    const return_type result = (K1L_upup * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r_upup) * K1R
                                              + K1L_upup * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r     ) * K1R_upup
                                              + K1L_upup * (values_vertex_l     ) * Pi_matrix * (values_vertex_r_upup) * K1R_upup
                                              + K1L_upup * (values_vertex_l     ) * Pi_matrix * (values_vertex_r     ) * K1R
                                              + K1L      * (values_vertex_l     ) * Pi_matrix * (values_vertex_r_upup) * K1R
                                              + K1L      * (values_vertex_l     ) * Pi_matrix * (values_vertex_r     ) * K1R_upup
                                              + K1L      * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r_upup) * K1R_upup
                                              + K1L      * (values_vertex_l_upup) * Pi_matrix * (values_vertex_r     ) * K1R
                                                );
                    return result;
                }
                else {
                    const return_type result = (values_vertex_l_upup * Pi_matrix * values_vertex_r
                                              + values_vertex_l      * Pi_matrix * values_vertex_r_upup);
                    if constexpr (diag_class == k2) {
                        return result * projection_lambdaBar;
                    }
                    else if constexpr (diag_class == k2b) {
                        return projection_lambda * result;
                    }
                    else {
                        return result;
                    }
                }

            } else {
                assert(false);
            }
        }
#if DEBUG_SYMMETRIES
            else if constexpr(channel == 'p' and spin == 1) {
                buffer_type_vertex_l values_vertex_l_other;
                buffer_type_vertex_r values_vertex_r_other;

                load_vertex_keldyshComponents_left_vectorized <1-spin>(values_vertex_l_other, input_l);
                load_vertex_keldyshComponents_right_vectorized<1-spin>(values_vertex_r_other, input_r);
                if constexpr (VECTORIZED_INTEGRATION) {
                    if constexpr(diag_class == k1) {
                        Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1L_temp    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<  spin, channel, channel, diag_class, result_type_fetch>(input_external);
                        Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1R_temp    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<1-spin, channel, channel, diag_class, result_type_fetch>(input_external);
                        K1L_temp.resize(4, 4);
                        K1R_temp.resize(4, 4);

                        const Eigen::Matrix<Q,4,4> K1L = K1L_temp.transpose();
                        const Eigen::Matrix<Q,4,4> K1R_other = K1R_temp;

                        const return_type result = (K1L * values_vertex_l_other * Pi_matrix * values_vertex_r_other * K1R_other);
                        return result;
                    }
                    else if constexpr (diag_class == k2){
                        const return_type result = (values_vertex_l_other * Pi_matrix * values_vertex_r);
                        return result * projection_lambdaBar;
                    }
                    else {
                        const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r_other);
                        if constexpr (diag_class == k2b) {
                            return projection_lambda * result;
                        }
                        else {
                            return result;
                        }

                    }

                }
                else {
                    assert(false);
                    const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r_other).eval()[0];
                    return result;
                }
            }
#endif
        else {
            if constexpr(VECTORIZED_INTEGRATION) {
                if constexpr(diag_class == k1) {
                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1L_temp    = vertex1.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                    Eigen::Matrix<Q,Eigen::Dynamic, Eigen::Dynamic> K1R_temp    = vertex2.template get_w_r_value_symmetry_expanded_nondiff<spin, channel, channel, diag_class, result_type_fetch>(input_external);
                    K1L_temp.resize(4, 4);
                    K1R_temp.resize(4, 4);

                    const Eigen::Matrix<Q,4,4> K1L = K1L_temp.transpose();
                    const Eigen::Matrix<Q,4,4> K1R = K1R_temp;

                    const return_type result = (K1L * values_vertex_l * Pi_matrix * values_vertex_r * K1R);

                    return result;
                }
                else {
                    const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r);
                    if constexpr (diag_class == k2) {
                        return result * projection_lambdaBar;
                    }
                    else if constexpr (diag_class == k2b) {
                        return projection_lambda * result;
                    }
                    else {
                        return result;
                    }
                }
            } else {
                assert(false);
            }
        }
    }
    else { // SBE_DECOMPOSITION
        if constexpr ((channel == 't' and spin == 0)
#if DEBUG_SYMMETRIES
            or (channel == 'a' and spin == 1)
#endif
                ) {

            buffer_type_vertex_l values_vertex_l_updnBar;
            buffer_type_vertex_r values_vertex_r_updnBar;

            load_vertex_keldyshComponents_left_vectorized<1-spin>(values_vertex_l_updnBar, input_l);
            load_vertex_keldyshComponents_right_vectorized<1-spin>(values_vertex_r_updnBar, input_r);
            if constexpr(VECTORIZED_INTEGRATION) {
                assert(input_external.iK == 0);
                const return_type result = ((values_vertex_l + values_vertex_l_updnBar) * Pi_matrix * values_vertex_r +
                                            values_vertex_l * Pi_matrix * (values_vertex_r + values_vertex_r_updnBar));
                return result;
            } else {
                const return_type result = ((values_vertex_l + values_vertex_l_updnBar) * Pi_matrix * values_vertex_r +
                                            values_vertex_l * Pi_matrix * (values_vertex_r + values_vertex_r_updnBar)).eval()[0];
                return result;
            }
        }
#if DEBUG_SYMMETRIES
            else if constexpr(channel == 'p' and spin == 1) {
                buffer_type_vertex_l values_vertex_l_other;
                buffer_type_vertex_r values_vertex_r_other;

                load_vertex_keldyshComponents_left_vectorized <1-spin>(values_vertex_l_other, input_l);
                load_vertex_keldyshComponents_right_vectorized<1-spin>(values_vertex_r_other, input_r);
                if constexpr (VECTORIZED_INTEGRATION) {
                    const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r_other);
                    return result;
                }
                else {
                    const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r_other).eval()[0];
                    return result;
                }
            }
#endif
        else {
            if constexpr(VECTORIZED_INTEGRATION) {
                const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r);
                return result;
            } else {
                const return_type result = (values_vertex_l * Pi_matrix * values_vertex_r).eval()[0];
                return result;
            }
        }
    }


}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::get_integrand_vals(const rvec& freqs, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& integrand_vals, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& Pivals, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& vertex_vals1, Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>& vertex_vals2) const {
    int npoints = freqs.size();

    Pivals = Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>(KELDYSH?16:1, npoints);
    integrand_vals = Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic>(VECTORIZED_INTEGRATION and KELDYSH_FORMALISM ?16:1, npoints);
    for (int i=0; i<npoints; ++i) {

        freqType vpp = freqs[i];


        Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> Pival;
#if VECTORIZED_INTEGRATION and KELDYSH_FORMALISM
        VertexInput input_l = input_external, input_r = input_external;
        input_l.v2 = vpp; input_r.v1 = vpp;
        input_l.iK = i0_left; input_r.iK = i0_right;
        input_l.spin = 0;
        input_r.spin = 0;
        Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> integrand_value;
        Eigen::Matrix<Q,4,4> vertex_val1(4,4);
        Eigen::Matrix<Q,4,4> vertex_val2(4,4);
        load_vertex_keldyshComponents_left_vectorized<0> (vertex_val1, input_l);
        load_vertex_keldyshComponents_right_vectorized<0>(vertex_val2, input_r);
        Pival = Pi.template value_vectorized<channel>(w, vpp, i_in);
#else
        Q integrand_value;
        Pival(0) = Pi.value(i2, w, vpp, i_in);
#endif

        integrand_value = (*this)(vpp);


#if VECTORIZED_INTEGRATION and KELDYSH_FORMALISM
        integrand_value.resize(16,1);

        Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> vertex_val1_temp = vertex_val1;
        Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> vertex_val2_temp = vertex_val2;
        vertex_val1_temp.resize(16,1);
        vertex_val2_temp.resize(16,1);
        integrand_vals.col(i) = integrand_value;
        vertex_vals1.col(i) = vertex_val1_temp;
        vertex_vals2.col(i) = vertex_val2_temp;
#else
        integrand_vals(i) = integrand_value;
#endif


        Pival.resize(KELDYSH?16:1,1);
        Pivals.col(i) = Pival;

    }


}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::save_integrand() const {
    /// Define standard frequency points on which to evaluate the integrand
    int npoints = 10000;//nBOS;
    if (diag_class == k2) {npoints = 1000;}
    else if (diag_class == k3) {npoints = 100;}

    rvec freqs (npoints);

    for (int i=0; i<npoints; ++i) {
        freqType wl, wu;
        switch (diag_class) {
            case k1:
                wl = vertex1.avertex().K1.frequencies.get_wlower_b();
                wu = vertex1.avertex().K1.frequencies.get_wupper_b();
                wl *= 2.;
                wu *= 2.;
                break;
            case k2:
                wl = vertex1.avertex().K2.frequencies.get_wlower_f();
                wu = vertex1.avertex().K2.frequencies.get_wupper_f();
                break;
            case k3:
                wl = vertex1.avertex().K3.frequencies.get_wlower_f();
                wu = vertex1.avertex().K3.frequencies.get_wupper_f();
                break;
            default:;
        }
        freqType vpp = wl + i * (wu - wl) / (npoints - 1);
        freqs[i] = vpp;
    }

    std::string filename_prefix = "";
    save_integrand(freqs, filename_prefix);

}

template<K_class diag_class, char channel, int spin, typename Q, typename vertexType_left, typename vertexType_right, class Bubble_Object,typename return_type>
void Integrand<diag_class,channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,return_type>::save_integrand(const rvec& freqs, const std::string& filename_prefix) const {
    /// evaluate the integrand on frequency points in freqs
    int npoints = freqs.size();

    rvec integrand_re (npoints);
    rvec integrand_im (npoints);
    rvec Pival_re (npoints);
    rvec Pival_im (npoints);
    Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> integrand_vals(VECTORIZED_INTEGRATION ? 16 : 1, npoints);
    Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> Pivals(KELDYSH?16:1,npoints);
    Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> vertex_vals1(VECTORIZED_INTEGRATION ? 16 : 1, npoints);
    Eigen::Matrix<Q,Eigen::Dynamic,Eigen::Dynamic> vertex_vals2(VECTORIZED_INTEGRATION ? 16 : 1,npoints);

    get_integrand_vals(freqs, integrand_vals, Pivals, vertex_vals1, vertex_vals2);

    std::string filename = "";
    filename += data_dir;
    filename += filename_prefix+"integrand_K" + (diag_class == k1 ? "1" : (diag_class == k2 ? "2" : (diag_class == k3 ? "3" : "2b")));
    filename += channel;
    filename += "_i0=" + std::to_string(i0_symmred)
                + "_i2=" + std::to_string(i2)
                + "_w=" + std::to_string(w);
    if (diag_class == k2) {filename += "_v=" + std::to_string(v);}
    else if (diag_class == k3) {filename += "_vp=" + std::to_string(vp);}
    filename += "_i_in=" + std::to_string(i_in);
    filename += + ".h5";

    if (mpi_world_rank() == 0) {
        H5::H5File file(filename, H5F_ACC_TRUNC);
        write_to_hdf(file, "v", freqs, false);
        write_to_hdf(file, "integrand", integrand_vals, false);
        write_to_hdf(file, "Pival", Pivals, false);
        write_to_hdf(file, "vertex1", vertex_vals1, false);
        write_to_hdf(file, "vertex2", vertex_vals2, false);
    }
}


#endif //KELDYSH_MFRG_INTEGRAND_HPP
