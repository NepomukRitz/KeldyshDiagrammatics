#ifndef KELDYSH_MFRG_BUBBLE_FUNCTION_HPP
#define KELDYSH_MFRG_BUBBLE_FUNCTION_HPP

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
#include "integrand.hpp"

/**
 * Class that actually performs the bubble computation. Invoked by the bubble_function.
 * @tparam channel Two-particle channel of the computation. Can be a, p, or t.
 * @tparam Q Template parameter specifying the type of the data.
 * @tparam vertexType_result Type of the vertex that results in the bubble computation.
 * @tparam vertexType_left Type of the vertex that enters as the left part of the computation.
 * @tparam vertexType_right Type of the vertex that enters as the right part of the computation.
 * @tparam Bubble_Object Type of the Bubble object to connect the two vertices
 */
template <char channel,
        typename Q,
        typename vertexType_result,
        typename vertexType_left,
        typename vertexType_right,
        class Bubble_Object>
class BubbleFunctionCalculator{
    using value_type = vec<Q>;
    private:
    vertexType_result& dgamma;

    const vertexType_left vertex1;  /// THIS IS A COPY; needed for symmetry-expansion
    const vertexType_right vertex2; /// THIS IS A COPY; needed for symmetry-expansion

    const Bubble_Object& Pi;
    const bool diff = Pi.diff;

    const double Delta = (Pi.g.Lambda + Pi.g.Gamma) / 2.; // hybridization (needed for proper splitting of the integration domain)


    int nw1_w = 0, nw2_w = 0, nw2_v = 0, nw3_w = 0, nw3_v = 0, nw3_v_p = 0;
    Q prefactor = 1.;

    size_t number_of_nodes;
    size_t mpi_size = mpi_world_size(); // number of mpi processes
    size_t mpi_rank = mpi_world_rank(); // number of the current mpi process
    std::array<std::size_t,my_defs::K1::rank> dimsK1; // number of vertex components over which i_mpi and i_omp are looped
    std::array<std::size_t,my_defs::K2::rank> dimsK2; // number of vertex components over which i_mpi and i_omp are looped
    std::array<std::size_t,my_defs::K3::rank> dimsK3; // number of vertex components over which i_mpi and i_omp are looped
    size_t dims_flat_K1, dims_flat_K2, dims_flat_K3; // flat dimensions of above arrays;
    size_t n_vectorization_K1, n_vectorization_K2, n_vectorization_K3;


    freqType vmin = 0, vmax = 0;
    int Nmin, Nmax; // Matsubara indices for minimal and maximal frequency. Only needed for finite-temperature Matsubara calculations!

    double tK1 = 0, tK2 = 0, tK3 = 0;

    std::array<bool,3> tobecomputed; // indicates whether classes K1, K2 and K3 are to be computed

    void check_presence_of_symmetry_related_contributions();
    void set_channel_specific_freq_ranges_and_prefactor();
    void find_vmin_and_vmax();

    template<K_class diag_class> void calculate_bubble_function();
    template<K_class diag_class> value_type get_value(int i_mpi, int i_omp, int n_omp, int n_vectorization);
    template<K_class diag_class,int spin> void calculate_value(value_type &value, int i0, int i_in, int iw, freqType w, freqType v, freqType vp);

    void write_out_results(const vec<Q>& Ordered_result, K_class diag_class);
    void write_out_results_K1(const vec<Q>& K1_ordered_result);
    void write_out_results_K2(const vec<Q>& K2_ordered_result);
    void write_out_results_K2b(const vec<Q>& K2b_ordered_result);
    void write_out_results_K3(const vec<Q>& K3_ordered_result);

    void set_external_arguments_for_parallelization(size_t& n_mpi, size_t& n_omp, size_t& n_vectorization, K_class diag_class);

    void convert_external_MPI_OMP_indices_to_physical_indices_K1(int& iK1, int& i0, int& ispin, int& iw, int& i_in, freqType & w,
                                                                 int i_mpi, int n_omp, int i_omp);
    void convert_external_MPI_OMP_indices_to_physical_indices_K2(int& iK2, int& i0, int& ispin, int& iw, int& iv, int& i_in,
                                                                 freqType& w, freqType& v,
                                                                 int i_mpi, int n_omp, int i_omp);
    void convert_external_MPI_OMP_indices_to_physical_indices_K2b(int& iK2, int& i0, int& ispin, int& iw, int& ivp, int& i_in,
                                                                  freqType& w, freqType& vp,
                                                                 int i_mpi, int n_omp, int i_omp);
    void convert_external_MPI_OMP_indices_to_physical_indices_K3(int& iK3, int& i0, int& ispin, int& iw, int& iv, int& ivp, int& i_in,
                                                                 freqType& w, freqType& v, freqType& vp,
                                                                 int i_mpi, int n_omp, int i_omp);

    int get_trafo_K1(int i0, freqType w);
    int get_trafo_K2(int i0, freqType w, freqType v);
    int get_trafo_K3(int i0, freqType w, freqType v, freqType vp);

    void get_Matsubara_integration_intervals(size_t& num_intervals, vec<vec<freqType>>& intervals, freqType w);

    Q bubble_value_prefactor();

    const bool store_integrand_for_PT = false; // set to true if the integrand for the fully retarded up-down component at zero frequency shall be saved

    public:
    void perform_computation();

    BubbleFunctionCalculator(vertexType_result& dgamma_in,
                             const vertexType_left& vertex1_in,
                             const vertexType_right& vertex2_in,
                             const Bubble_Object& Pi_in,
                             const int number_of_nodes_in,
                             const std::array<bool,3> tobecomputed
                             )
                             :dgamma(dgamma_in), vertex1(vertex1_in), vertex2(vertex2_in),
                             Pi(Pi_in), number_of_nodes(number_of_nodes_in), tobecomputed(tobecomputed){
#if not  DEBUG_SYMMETRIES
        //check_presence_of_symmetry_related_contributions();
#endif
        set_channel_specific_freq_ranges_and_prefactor();
        find_vmin_and_vmax();

#if SWITCH_SUM_N_INTEGRAL
        vertex1.template symmetry_expand<channel,true ,false>();
        vertex2.template symmetry_expand<channel,false,false>();
#endif
        /// TODO(high): Figure out computations which need gamma_a_uu = gamma_a_ud - gamma_t_ud in a t-bubble,
        ///  i.e. CP_to_t(gamma_a_uu) = CP_to_t(gamma_a_ud) - CP_to_a(gamma_t_ud).
        ///  The integrand will need vertex AND vertex_initial to have access to cross-projected parts and non-crossprojected parts.

        vec<int> vectorization_dimensions_K1;
        vec<int> vectorization_dimensions_K2;
        vec<int> vectorization_dimensions_K3;
        if constexpr (VECTORIZED_INTEGRATION) {
            if constexpr (KELDYSH_FORMALISM) {
                vectorization_dimensions_K1 = {my_defs::K1::keldysh};
                vectorization_dimensions_K2 = {my_defs::K2::keldysh};
                vectorization_dimensions_K3 = {my_defs::K3::keldysh};
            }
            else if constexpr (!KELDYSH_FORMALISM and !ZERO_T) {
                vectorization_dimensions_K2 = {my_defs::K2::nu};
                vectorization_dimensions_K3 = {my_defs::K3::nu, my_defs::K3::nup};
            }
        }

        dimsK1 = vertex1.get_rvertex(channel).K1.get_dims();
        dimsK2 = MAX_DIAG_CLASS > 1 ? vertex1.get_rvertex(channel).K2.get_dims() : std::array<std::size_t, my_defs::K2::rank>();
        dimsK3 = MAX_DIAG_CLASS > 2 ? vertex1.get_rvertex(channel).K3.get_dims() : std::array<std::size_t, my_defs::K3::rank>();
        n_vectorization_K1 = 1;
        n_vectorization_K2 = 1;
        n_vectorization_K3 = 1;
        for (int i : vectorization_dimensions_K1) {
            n_vectorization_K1 *= dimsK1[i];
            dimsK1[i] = 1;
            //utils::print("vectorization_dimensions_K1 \t i=", i, "\n");
        }
        for (int i : vectorization_dimensions_K2) {
            n_vectorization_K2 *= dimsK2[i];
            dimsK2[i] = 1;
            //utils::print("vectorization_dimensions_K2 \t i=", i, "\n");
        }
        for (int i : vectorization_dimensions_K3) {
            n_vectorization_K3 *= dimsK3[i];
            dimsK3[i] = 1;
            //utils::print("vectorization_dimensions_K3 \t i=", i, "\n");
        }
        dims_flat_K1 = getFlatSize(dimsK1);
        dims_flat_K2 = getFlatSize(dimsK2);
        dims_flat_K3 = getFlatSize(dimsK3);


    }
};

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
Bubble_Object>::set_channel_specific_freq_ranges_and_prefactor() {
// set channel-specific frequency ranges and prefactor (1, 1, -1 for a, p, t) for sum over spins.

    nw1_w = dgamma.get_rvertex(channel).K1.get_dims()[my_defs::K1::omega];
    nw2_w = dgamma.get_rvertex(channel).K2.get_dims()[my_defs::K2::omega];
    nw2_v = dgamma.get_rvertex(channel).K2.get_dims()[my_defs::K2::nu];
    nw3_w = dgamma.get_rvertex(channel).K3.get_dims()[my_defs::K3::omega];
    nw3_v = dgamma.get_rvertex(channel).K3.get_dims()[my_defs::K3::nu];
    nw3_v_p=dgamma.get_rvertex(channel).K3.get_dims()[my_defs::K3::nup];

    prefactor = channel == 't' ? -1. : 1.;

}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right, Bubble_Object>::find_vmin_and_vmax() {
    vmin =-Delta * 10.;
    vmax = Delta * 10.;

    if constexpr((!KELDYSH) && (!ZERO_T)) { // for finite-temperature Matsubara calculations
        // make sure that the limits for the Matsubara sum are fermionic
        Nmin = - POSINTRANGE;
        Nmax = - Nmin - 1;
        vmin = (Nmin*2+1)*(M_PI*Pi.g.T);
        vmax = (Nmax*2+1)*(M_PI*Pi.g.T);
    }
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
                Bubble_Object>::perform_computation(){

    double t_start = utils::get_time();
    if constexpr(MAX_DIAG_CLASS >= 0) {
        if (tobecomputed[0]) {
            calculate_bubble_function<k1>();
            tK1 = utils::get_time() - t_start;
            //utils::print("K1", channel, " done, ");
            //utils::get_time(t_start);
        }
    }
    if constexpr(MAX_DIAG_CLASS >= 2) {
        if (tobecomputed[1]) {
            t_start = utils::get_time();
            calculate_bubble_function<k2>();
            tK2 = utils::get_time() - t_start;
            //utils::print("K2", channel, " done, ");
            //utils::get_time(t_start);

#if DEBUG_SYMMETRIES
            t_start = utils::get_time();
            calculate_bubble_function<k2b>();
            tK2 = utils::get_time() - t_start;
            //utils::print("K2b", channel, " done, ");
            //utils::get_time(t_start);
#endif
        }
    }
    if constexpr(MAX_DIAG_CLASS >= 3) {
        if (tobecomputed[2]) {
            t_start = utils::get_time();
            calculate_bubble_function<k3>();
            tK3 = utils::get_time() - t_start;
            //utils::print("K3", channel, " done, ");
            //utils::get_time(t_start);
        }
    }

}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
                template<K_class diag_class>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::calculate_bubble_function(){
    if (diag_class < k1 || diag_class > k3){utils::print("Incompatible diagrammatic class! Abort."); assert(false); return;}

    std::size_t n_mpi, n_omp, n_vectorization;
    set_external_arguments_for_parallelization(n_mpi, n_omp, n_vectorization, diag_class);


    // initialize buffer into which each MPI process writes their results
    vec<Q> Buffer = mpi_initialize_buffer<Q>(n_mpi, n_omp * n_vectorization);
    vertex1.initializeInterpol();
    vertex2.initializeInterpol();

    // start for-loop over external arguments, using MPI and OMP
    int iterator = 0;
    // total number of processes for MPI and OMP parallelization:
    const int dimsKi_flat = diag_class == k1 ? dims_flat_K1 : (diag_class == k3 ? dims_flat_K3 : dims_flat_K2);
    for (int i_mpi = 0; i_mpi < n_mpi; ++i_mpi) {
        if (i_mpi % mpi_size == mpi_rank) {
#pragma omp parallel for schedule(dynamic)
            for (int i_omp = 0; i_omp < n_omp; ++i_omp) {
                if (i_mpi * n_omp + i_omp < dimsKi_flat) {
                    value_type value = get_value<diag_class>(i_mpi, i_omp, n_omp, n_vectorization);
                    for (int k = 0; k < n_vectorization; k++) {
                        Buffer[(iterator * n_omp + i_omp) * n_vectorization + k] = value[k]; // write result of integration into MPI buffer
                    }
                }
            }
            ++iterator;
        }
    }
    // collect+combine results from different MPI processes, reorder them appropriately
    vec<Q> Result = mpi_initialize_result<Q> (n_mpi, n_omp * n_vectorization);
    mpi_collect(Buffer, Result, n_mpi, n_omp * n_vectorization);
    vec<Q> Ordered_result = mpi_reorder_result(Result, n_mpi, n_omp * n_vectorization);

    const int target_size_of_result = diag_class == k1 ? dims_flat_K1 * n_vectorization_K1 : (diag_class == k3 ? dims_flat_K3 * n_vectorization_K3 : dims_flat_K2 * n_vectorization_K2);
#ifndef NDEBUG
    const int current_size_of_result = Ordered_result.size();
    for (int i = target_size_of_result; i < current_size_of_result; i++) {
        assert(std::abs(Ordered_result[i]) < 1e-12);
    }
#endif
    // cut off superflous vector elements:
    Ordered_result.resize(target_size_of_result);
    write_out_results(Ordered_result, diag_class);

    vertex1.set_initializedInterpol(false);
    vertex2.set_initializedInterpol(false);

}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
template<K_class diag_class>
auto
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::get_value(const int i_mpi, const int i_omp, const int n_omp, const int n_vectorization) -> value_type{
    value_type value(n_vectorization);
    int iK1, iK2, i0, ispin, iw, iv, ivp, i_in;
    freqType w, v, vp;
    int trafo;
    switch (diag_class) {
        case k1:
            convert_external_MPI_OMP_indices_to_physical_indices_K1(iK1, i0, ispin,iw, i_in, w,
                                                                    i_mpi, n_omp, i_omp);
            trafo = get_trafo_K1(i0, w);
            if (trafo == 0) {
                if (ispin == 0 or n_spin == 1)  calculate_value<diag_class,0>(value, i0, i_in, 0, w, 0, 0);
                else                            calculate_value<diag_class,1>(value, i0, i_in, 0, w, 0, 0);
            }
            break;
        case k2:
            convert_external_MPI_OMP_indices_to_physical_indices_K2(iK2, i0, ispin, iw, iv, i_in, w, v,
                                                                    i_mpi, n_omp, i_omp);
            trafo = get_trafo_K2(i0, w, v);
            if (trafo == 0) {
                if (ispin == 0 or n_spin == 1) calculate_value<diag_class,0>(value, i0, i_in, 0, w, v, 0);
                else                           calculate_value<diag_class,1>(value, i0, i_in, 0, w, v, 0);}
            break;
#if DEBUG_SYMMETRIES
        case k2b:
            convert_external_MPI_OMP_indices_to_physical_indices_K2b(iK2, i0, ispin, iw, ivp, i_in, w, vp,
                                                                    i_mpi, n_omp, i_omp);
            trafo = 0; // compute integrals for all frequency components
            if (!KELDYSH and !ZERO_T and -vp + signFlipCorrection_MF(w)*0.5 < vertex1.avertex().K2b.frequencies.get_wlower_f()) {
                trafo = -1;
            }
            if (trafo == 0) {
                if (ispin == 0 or n_spin == 1) calculate_value<diag_class,0>(value, i0, i_in, 0, w, 0, vp);
                else                           calculate_value<diag_class,1>(value, i0, i_in, 0, w, 0, vp);
            }
            break;
#endif
        case k3:
            convert_external_MPI_OMP_indices_to_physical_indices_K3(iK2, i0, ispin, iw, iv, ivp, i_in, w, v, vp,
                                                                    i_mpi, n_omp, i_omp);
            trafo = get_trafo_K3(i0, w, v, vp);
            if (trafo == 0) {
                if (ispin == 0 or n_spin == 1) calculate_value<diag_class,0>(value, i0, i_in, channel=='a' ? iw : (channel=='p' ? iv : ivp), w, v, vp);  // for 2D interpolation of K3 we need to know the index of the constant bosonic frequency w_r (r = channel of the bubble)
                else                           calculate_value<diag_class,1>(value, i0, i_in, channel=='a' ? iw : (channel=='p' ? iv : ivp), w, v, vp);
            }
            break;
        default:;
    }
    return value;
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
template<K_class k, int spin>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::calculate_value(value_type& value, const int i0, const int i_in, const int iw,
                                           const freqType w, const freqType v, const freqType vp){
#if VECTORIZED_INTEGRATION==1 and KELDYSH_FORMALISM
    using Integrand_class = Integrand<k, channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,Eigen::Matrix<Q,4,4>>;
#else
    using Integrand_class = Integrand<k, channel, spin, Q, vertexType_left, vertexType_right, Bubble_Object,Q>;
#endif
    using integrand_valtype = std::result_of_t<Integrand_class(freqType)>;
    integrand_valtype integration_result = myzero<integrand_valtype>();

#if not SWITCH_SUM_N_INTEGRAL
    static_assert(n_spin == 1, "SWITCH_SUM_N_INTEGRAL not ready for DEBUG_SYMMETRIES.");
    for (int i2 : glb_non_zero_Keldysh_bubble) {
        int n_spin_sum = 1;                  // number of summands in spin sum (=1 in the a channel)
        if ((channel == 't' and spin == 0) or (channel == 'a' and spin == 1)) n_spin_sum = 3;  // in the t channel, spin sum includes three terms
        for (int i_spin=0; i_spin < n_spin_sum; ++i_spin) {
#else
    int i2 = 0;
    int i_spin = 0;
#endif
            // initialize the integrand object and perform frequency integration
            Integrand_class integrand(vertex1, vertex2, Pi, i0, i2, iw, w, v, vp, i_in, i_spin, diff);

            if (store_integrand_for_PT){ // for PT-Calculations: Store the integrand for the fully retarded up-down component at zero frequency:
#if SWITCH_SUM_N_INTEGRAL
                assert(false); // Cannot do it this way if the integration is vectorized over the internal Keldysh sum
#endif
                if (channel == 'a' and i0 == 7 and i_spin == 0 and w == 0 and v == 0 and vp == 0) integrand.save_integrand();
            }


            if constexpr(ZERO_T) {
                switch (k) {
                    case k1:
                        integration_result += bubble_value_prefactor() *
                                 integrator_Matsubara_T0(integrand, vmin, vmax, std::abs(w / 2),
                                                               {0.}, Delta, true);

                break;
            case k2:
                integration_result += bubble_value_prefactor() * integrator_Matsubara_T0(integrand, vmin, vmax, std::abs(w / 2), {0., v, v + w, v - w}, Delta, true);
                break;
            case k3:
                integration_result += bubble_value_prefactor() * integrator_Matsubara_T0(integrand, vmin, vmax, std::abs(w / 2), {0, v, vp, w - vp, w + vp, w - v, w + v}, Delta, true);
                break;
            case k2b:
                integration_result += bubble_value_prefactor() * integrator_Matsubara_T0(integrand, vmin, vmax, std::abs(w / 2), {0, vp, vp + w, vp - w}, Delta, true);
                break;
            default:
                break;
        }
    } else {
        if constexpr(KELDYSH) {
            integration_result += bubble_value_prefactor() * integrator(integrand, vmin, vmax, -w / 2., w / 2., Delta, true);
        } else {
            const int interval_correction = signFlipCorrection_MF_int(w);
            const int W = (int) (w / 2);
            const int Nmin_sum = -POSINTRANGE - std::abs(W / 2) + interval_correction;
            const int Nmax_sum = POSINTRANGE - 1 + std::abs(W / 2);
            const freqType vmin_temp = (2 * Nmin_sum + 1.);
            const freqType vmax_temp = (2 * Nmax_sum + 1.);


            if constexpr (VECTORIZED_INTEGRATION) // (VECTORIZED_INTEGRATION and k == k3)
            {
                int Nmin_v, Nmax_v, Nmin_vp, Nmax_vp, N_vp;
                if (k == k1) {
                    Nmin_v = 0; Nmax_v = 0; Nmin_vp = 0; Nmax_vp = 0; N_vp = 1;
                }
                else if (k == k2) {
                    Nmin_v = -nFER2/2; Nmax_v = DEBUG_SYMMETRIES ? nFER2/2-1 : -1; Nmin_vp = 0; Nmax_vp = 0; N_vp = 1;
                }
                else if (k == k2b) {
                    Nmin_v = 0; Nmax_v = 0; Nmin_vp = -nFER2/2; Nmax_vp = DEBUG_SYMMETRIES ? nFER2/2-1 : -1; N_vp = nFER2;
                }
                else {
                    Nmin_v = -nFER3/2; Nmax_v = DEBUG_SYMMETRIES ? nFER3/2-1 : -1; Nmin_vp = -nFER3/2; Nmax_vp = nFER3/2-1; N_vp = nFER3;
                }
                Eigen::Matrix<Q, Eigen::Dynamic, Eigen::Dynamic> summation_result =
                        matsubarasum_vectorized<spin,channel>(integrand, Nmin_v, Nmax_v, Nmin_sum, Nmax_sum, Nmin_vp, Nmax_vp) * bubble_value_prefactor() * (2 * M_PI) * Pi.g.T;

                for (int i = Nmin_v; i <= Nmax_v; i++) {
                    for (int j = Nmin_vp; j <= Nmax_vp; j++) {
                        const freqType v_temp  = (i*2 + 1);
                        const freqType vp_temp = (j*2 + 1);
                        Integrand_class integrand_asymp(vertex1, vertex2, Pi, i0, i2, iw, w, v_temp, vp_temp, i_in, i_spin, diff);
#if ANALYTIC_TAILS
                        /// determine integral of the integrand's tails via formula (corresponds to quadrature of bare bubble):
                        summation_result(-Nmin_v+i,-Nmin_vp+j) +=
                                bubble_value_prefactor() * asymp_corrections_bubble<channel,spin>(k, vertex1, vertex2, Pi.g,
                                                                                                  vmin_temp, vmax_temp, w, v_temp, vp_temp, i0, i2,
                                                                                                  i_in, diff);
#else
                        /// determine integral of the integrand's tails via quadrature:
                        summation_result(-Nmin_v+i,-Nmin_vp+j) += bubble_value_prefactor() * M_PI * Pi.g.T *
                                                                  asymp_corrections_bubble_via_quadrature<Q>(integrand_asymp, vmin_temp, vmax_temp);

#endif
                    }
                }

                for (int i = Nmin_v; i <= Nmax_v; i++) {
                    for (int j = Nmin_vp; j <= Nmax_vp; j++) {
                        value[(-Nmin_v+i)*N_vp  + (-Nmin_vp+j)] = summation_result(-Nmin_v+i,-Nmin_vp+j);
                    }
                }
            }
            else
            {
                integration_result = bubble_value_prefactor() * (2 * M_PI) * Pi.g.T *
                                     matsubarasum<Q>(integrand, Nmin_sum, Nmax_sum);
#if ANALYTIC_TAILS

                /// determine integral of the integrand's tails via formula:
                integration_result +=
                        bubble_value_prefactor() * asymp_corrections_bubble<channel,spin>(k, vertex1, vertex2, Pi.g,
                                                                                     vmin_temp, vmax_temp, w, v, vp, i0, i2,
                                                                                     i_in, diff);
#else
                /// Compute high-frequency contributions via quadrature:
                integration_result += bubble_value_prefactor() * (2 * M_PI) * Pi.g.T *
                                      asymp_corrections_bubble_via_quadrature<Q>(integrand, vmin_temp, vmax_temp);
#endif
            }
        }
    }

#if not SWITCH_SUM_N_INTEGRAL
    }
#else
    //for (int i2: glb_non_zero_Keldysh_bubble)
    {
#endif
        // asymptotic corrections include spin sum
        if constexpr ((ZERO_T or KELDYSH) and false) {
            integration_result +=
                    bubble_value_prefactor() * asymp_corrections_bubble<channel>(k, vertex1, vertex2, Pi.g,
                                                                                 vmin, vmax, w, v, vp, i0, i2,
                                                                                 i_in, diff, spin);
        }

    }

    /// write integration_result into value
    if constexpr(VECTORIZED_INTEGRATION == 1) {

        if constexpr (KELDYSH_FORMALISM) {
            // for vector-/matrix-valued result:
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    value[rotate_Keldysh_matrix<channel, true>(i * 4 + j)] = integration_result(i, j);
                }
            }
        }
    }
    else {
        // for scalar result:
        value[0] = integration_result;
    }
}


template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::write_out_results(const vec<Q>& Ordered_result, const K_class diag_class){
    switch (diag_class) {
        case k1:
            write_out_results_K1(Ordered_result);
            break;
        case k2:
            write_out_results_K2(Ordered_result);
            break;
#if DEBUG_SYMMETRIES
        case k2b:
            write_out_results_K2b(Ordered_result);
            break;
#endif
        case k3:
            write_out_results_K3(Ordered_result);
            break;
        default: ;
    }
    dgamma.set_initializedInterpol(false);      // above initialization of the Interpolator is with the symmetry-reduced sector only (rest = zero)
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
                Bubble_Object>::write_out_results_K1(const vec<Q>& K1_ordered_result){
    dgamma.get_rvertex(channel).K1.set_vec(K1_ordered_result);
    if constexpr(not DEBUG_SYMMETRIES) {
        dgamma.initializeInterpol();     // initialize Interpolator with the symmetry-reduced sector of the vertex to retrieve all remaining entries
        dgamma.get_rvertex(channel).enforce_freqsymmetriesK1(dgamma.get_rvertex(channel));
    }

}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::write_out_results_K2(const vec<Q>& K2_ordered_result){
    //assert( K2_ordered_result.size() == nK_K2*n_spin*nBOS2*nFER2*n_in);
    dgamma.get_rvertex(channel).K2.set_vec(K2_ordered_result);
    if constexpr(not DEBUG_SYMMETRIES) {
        dgamma.initializeInterpol();     // initialize Interpolator with the symmetry-reduced sector of the vertex to retrieve all remaining entries
        dgamma.get_rvertex(channel).enforce_freqsymmetriesK2(dgamma.get_rvertex(channel));
    }
}

#if DEBUG_SYMMETRIES
template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::write_out_results_K2b(const vec<Q>& K2b_ordered_result){
    //assert( K2b_ordered_result.size() == nK_K2*n_spin*nBOS2*nFER2*n_in);
    dgamma.get_rvertex(channel).K2b.set_vec(K2b_ordered_result);
}
#endif

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::write_out_results_K3(const vec<Q>& K3_ordered_result){
    dgamma.get_rvertex(channel).K3.set_vec(K3_ordered_result);
    if constexpr(not DEBUG_SYMMETRIES) {
        dgamma.initializeInterpol();     // initialize Interpolator with the symmetry-reduced sector of the vertex to retrieve all remaining entries
        dgamma.get_rvertex(channel).enforce_freqsymmetriesK3(dgamma.get_rvertex(channel));
    }
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::set_external_arguments_for_parallelization(size_t& n_mpi, size_t& n_omp, size_t& n_vectorization, const K_class diag_class){

    /// The computation of the vertex components of dgamma.Ki (i = 1, 2, 2', 3) can be parallelized
    /// parallelization is achieved via...
    ///     ... MPI --> distribute over multiple nodes (n_mpi = maximal number of MPI processes)
    ///     ... OMP --> distribute over multiple threads within the same node (n_omp = number of
    ///     ... vectorization --> use efficient matrix routines
    ///                           If vectorization is used (*this)->get_value() returns a vector with the components over which we vectorize
    ///                           and in (*this)->calculate_bubble_function() we need to loop over the remaining dimensions (these are given in dimsK1, dimsK2 and dimsK3)

    const size_t nK_K1 = dimsK1[my_defs::K1::keldysh];
    const size_t nK_K2 = dimsK2[my_defs::K2::keldysh];
    const size_t nK_K3 = dimsK3[my_defs::K3::keldysh];

    switch (diag_class) {
        case k1:
            // set external arguments for MPI-parallelization (# of tasks distributed via MPI)
            n_mpi = number_of_nodes;
            n_vectorization = n_vectorization_K1;
            // set external arguments for OMP-parallelization (# of tasks per MPI-task distributed via OMP)
            // below formula performs integer division with rounding up
            n_omp = (dims_flat_K1 + n_mpi - 1) / n_mpi;
            break;
        case k2:
        case k2b:
            n_mpi = number_of_nodes;        // set external arguments for MPI-parallelization (# of tasks distributed via MPI)
            n_vectorization = n_vectorization_K2;
            n_omp = (dims_flat_K2 + n_mpi - 1) / n_mpi;
            break;
        case k3:
            n_mpi = number_of_nodes;        // set external arguments for MPI-parallelization (# of tasks distributed via MPI)
            n_vectorization = n_vectorization_K3;
            n_omp = (dims_flat_K3 + n_mpi - 1) / n_mpi;
            break;
        default: ;
    }
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::convert_external_MPI_OMP_indices_to_physical_indices_K1(int& iK1, int& i0, int& ispin, int& iw, int& i_in, freqType& w,
                                                                                     const int i_mpi, const int n_omp, const int i_omp){
    iK1 = i_mpi * n_omp + i_omp;

    my_defs::K1::index_type idx;
    getMultIndex<rank_K1>(idx, iK1, dimsK1);
    i0       = (int) idx[my_defs::K1::keldysh];
    ispin    = (int) idx[my_defs::K1::spin];
    iw       = (int) idx[my_defs::K1::omega];
    i_in     = (int) idx[my_defs::K1::internal];
   //getMultIndex<4,int,int,int,int>(ispin, iw, i0, i_in, iK1, vertex1.avertex().K1.get_dims());

    if (channel == 'a') dgamma.avertex().K1.frequencies.get_freqs_w(w, iw);           // frequency acc. to frequency index
    if (channel == 'p') dgamma.pvertex().K1.frequencies.get_freqs_w(w, iw);           // frequency acc. to frequency index
    if (channel == 't') dgamma.tvertex().K1.frequencies.get_freqs_w(w, iw);           // frequency acc. to frequency index
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::convert_external_MPI_OMP_indices_to_physical_indices_K2(int& iK2, int& i0, int& ispin,  int& iw, int& iv, int& i_in,
                                                                                freqType& w, freqType& v,
                                                                                const int i_mpi, const int n_omp, const int i_omp){
    iK2 = i_mpi * n_omp + i_omp;

    my_defs::K2::index_type idx;
    getMultIndex<rank_K2>(idx, iK2, dimsK2);
    i0       = (int) idx[my_defs::K2::keldysh];
    ispin    = (int) idx[my_defs::K2::spin];
    iw       = (int) idx[my_defs::K2::omega];
    iv       = (int) idx[my_defs::K2::nu];
    i_in     = (int) idx[my_defs::K2::internal];
    //getMultIndex<5,int,int,int,int,int>(ispin, iw, iv, i0, i_in, iK2, vertex1.avertex().K2.get_dims());
    if (channel == 'a') dgamma.avertex().K2.frequencies.get_freqs_w(w, v, iw, iv);
    if (channel == 'p') dgamma.pvertex().K2.frequencies.get_freqs_w(w, v, iw, iv);
    if (channel == 't') dgamma.tvertex().K2.frequencies.get_freqs_w(w, v, iw, iv);
}

#if DEBUG_SYMMETRIES
template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::convert_external_MPI_OMP_indices_to_physical_indices_K2b(int& iK2, int& i0, int& ispin,  int& iw, int& ivp, int& i_in,
                                                                                freqType& w, freqType& vp,
                                                                                const int i_mpi, const int n_omp, const int i_omp){
    iK2 = i_mpi * n_omp + i_omp;

    my_defs::K2::index_type idx;
    getMultIndex<rank_K2>(idx, iK2, dimsK2);
    i0       = (int) idx[my_defs::K2b::keldysh];
    ispin    = (int) idx[my_defs::K2b::spin];
    iw       = (int) idx[my_defs::K2b::omega];
    ivp      = (int) idx[my_defs::K2b::nup];
    i_in     = (int) idx[my_defs::K2b::internal];
    //getMultIndex<5,int,int,int,int,int>(ispin, iw, ivp, i0, i_in, iK2, vertex1.avertex().K2b.get_dims());
    if (channel == 'a') dgamma.avertex().K2b.frequencies.get_freqs_w(w, vp, iw, ivp);
    if (channel == 'p') dgamma.pvertex().K2b.frequencies.get_freqs_w(w, vp, iw, ivp);
    if (channel == 't') dgamma.tvertex().K2b.frequencies.get_freqs_w(w, vp, iw, ivp);
}
#endif

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::convert_external_MPI_OMP_indices_to_physical_indices_K3(int& iK3, int& i0, int& ispin,  int& iw, int& iv, int& ivp, int& i_in,
                                                                                freqType& w, freqType& v, freqType& vp,
                                                                                     const int i_mpi, const int n_omp, const int i_omp){
    iK3 = i_mpi * n_omp + i_omp;

    my_defs::K3::index_type idx;
    getMultIndex<rank_K3>(idx, iK3, dimsK3);
    i0       = (int) idx[my_defs::K3::keldysh];
    ispin    = (int) idx[my_defs::K3::spin];
    iw       = (int) idx[my_defs::K3::omega];
    iv       = (int) idx[my_defs::K3::nu];
    ivp      = (int) idx[my_defs::K3::nup];
    i_in     = (int) idx[my_defs::K3::internal];
    //getMultIndex<6,int,int,int,int,int,int>(ispin, iw, iv, ivp, i0, i_in, iK3, vertex1.avertex().K3.get_dims());
    if (channel == 'a') dgamma.avertex().K3.frequencies.get_freqs_w(w, v, vp, iw, iv, ivp);
    if (channel == 'p') dgamma.pvertex().K3.frequencies.get_freqs_w(w, v, vp, iw, iv, ivp);
    if (channel == 't') dgamma.tvertex().K3.frequencies.get_freqs_w(w, v, vp, iw, iv, ivp);
}


template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
int
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
                Bubble_Object>::get_trafo_K1(const int i0, const freqType w){
    int trafo = 1;

    if constexpr(DEBUG_SYMMETRIES) {
        trafo = 0; // compute integrals for all frequency components
    }
    else {
        if constexpr(VECTORIZED_INTEGRATION) {
            if constexpr (KELDYSH_FORMALISM) {
                // Make sure that the frequency point does not belong to the symmetry-reduced sector for all relevant Keldysh components
                // otherwise we have to compute that point
                int sign_w = sign_index<freqType>(w );
                const int number_of_indep_Components = dgamma.get_rvertex(channel).K1.get_dims()[my_defs::K1::keldysh];
                if (dgamma.get_rvertex(channel).freq_transformations.K1[sign_w] == 0) trafo = 0;
                /*
                for (int i0_temp = 0; i0_temp < number_of_indep_Components; i0_temp++) {
                    if (dgamma.get_rvertex(channel).freq_transformations.K1[i0_temp][sign_w] == 0) trafo = 0;
                }
                 */
            }
            else { // Matsubara finite T
                int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
                if (sign_w == 0) trafo = 0;
            }
        }
        else {
            int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
            trafo = dgamma.get_rvertex(channel).freq_transformations.K1[i0][sign_w];

        } // VECTORIZED_INTEGRATION

    } // DEBUG_SYMMETRIES
    return trafo;
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
int
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::get_trafo_K2(const int i0, const freqType w, const freqType v){
    int trafo = 1;
    if constexpr (DEBUG_SYMMETRIES) {
        trafo = 0; // compute integrals for all frequency components
        if (!KELDYSH and !ZERO_T and -v + signFlipCorrection_MF(w) * 0.5 < vertex1.avertex().K2.frequencies.get_wlower_f()) {
            trafo = -1;
        }
    }
    else {
        if constexpr (VECTORIZED_INTEGRATION) {
            if constexpr (KELDYSH_FORMALISM) {
                // Make sure that the frequency point does not belong to the symmetry-reduced sector for any relevant Keldysh component
                // otherwise we have to compute that point via quadrature
                int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
                int sign_v = sign_index<freqType>(v ); // safety to ensure that w=0 gets sign_w=-1
                if (dgamma.get_rvertex(channel).freq_transformations.K2[sign_w * 2 + sign_v] == 0) trafo = 0;
                /*
                const int number_of_indep_Components = dgamma.get_rvertex(channel).K2.get_dims()[my_defs::K2::keldysh];
                for (int i0_temp = 0; i0_temp < number_of_indep_Components; i0_temp++) {
                    if (dgamma.get_rvertex(channel).freq_transformations.K2[i0_temp][sign_w * 2 + sign_v] == 0) trafo = 0;
                }
                 */
            }
            else {// Matsubara finite T
                int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
                if (sign_w == 0) trafo = 0;
            }
        }
        else{
            int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
            int sign_v = sign_index<freqType>(v ); // safety to ensure that w=0 gets sign_w=-1
            trafo = dgamma.get_rvertex(channel).freq_transformations.K2[i0][sign_w*2 + sign_v];

        if (!KELDYSH and !ZERO_T and -v + signFlipCorrection_MF(w)*0.5 < vertex1.avertex().K2.frequencies.get_wlower_f()) {
            trafo = -1;
        }

        } // VECTORIZED_INTEGRATION
    } // DEBUG_SYMMETRIES
    return trafo;
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
int
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
        Bubble_Object>::get_trafo_K3(const int i0, const freqType w, const freqType v, const freqType vp){
    int trafo = 1;
    if constexpr(DEBUG_SYMMETRIES) {
        trafo = 0; // compute integrals for all frequency components

        if (!KELDYSH and !ZERO_T and
            (-v + signFlipCorrection_MF(w) * 0.5 < vertex1.avertex().K3.frequencies.get_wlower_f() or
             -vp + signFlipCorrection_MF(w) * 0.5 < vertex1.avertex().K3.frequencies.get_wlower_f())) {
            trafo = -1;
        }
    }
    else {
        if constexpr(VECTORIZED_INTEGRATION) {
            if constexpr (KELDYSH_FORMALISM) {

                // Make sure that the frequency point does not belong to the symmetry-reduced sector for all relevant Keldysh components
                // otherwise we have to compute that point
                int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
                int sign_f = sign_index<freqType>(v + vp );
                int sign_fp = sign_index<freqType>(v - vp );
                if (dgamma.get_rvertex(channel).freq_transformations.K3[sign_w * 4 + sign_f * 2 + sign_fp] == 0) trafo = 0;
                /*
                const int number_of_indep_Components = dgamma.get_rvertex(channel).K3.get_dims()[my_defs::K3::keldysh];

                for (int i0_temp = 0; i0_temp < number_of_indep_Components; i0_temp++) {
                    if (dgamma.get_rvertex(channel).freq_transformations.K3[i0_temp][sign_w * 4 + sign_f * 2 + sign_fp] ==
                        0)
                        trafo = 0;
                }
                 */
            }
            else {// Matsubara finite T
                int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
                if (sign_w == 0) trafo = 0;

            }

        }
        else {
            int sign_w = sign_index<freqType>(w ); // safety to ensure that w=0 gets sign_w=-1
            int sign_f = sign_index<freqType>(v + vp );
            int sign_fp= sign_index<freqType>(v - vp );
            trafo = dgamma.get_rvertex(channel).freq_transformations.K3[i0][sign_w * 4 + sign_f * 2 + sign_fp];


            if (!KELDYSH and !ZERO_T and (-v + signFlipCorrection_MF(w)*0.5 < vertex1.avertex().K3.frequencies.get_wlower_f() or -vp + signFlipCorrection_MF(w)*0.5 < vertex1.avertex().K3.frequencies.get_wlower_f())) {
                trafo = -1;
                //std::cout << "omitted frequencies: " << v << "\t" << vp << std::endl;
                //std::cout << "with limits " << vertex1.avertex().K3.frequencies.get_wlower_f() << std::endl;
            }

        } // VECTORIZED_INTEGRATION
    } // DEBUG_SYMMETRIES
    return trafo;
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
                Bubble_Object>::get_Matsubara_integration_intervals(size_t& num_intervals, vec<vec<freqType>>& intervals,
                                                                 const freqType w){
    if( -std::abs(w/2)+inter_tol < std::abs(w/2)-inter_tol){
        intervals = {{vmin, -std::abs(w/2)}, {-std::abs(w/2), std::abs(w/2)}, {std::abs(w/2), vmax}};
        num_intervals = 3;
    }
    else {
        intervals = {{vmin, -std::abs(w/2)}, {std::abs(w/2), vmax}};
        num_intervals = 2;
    }
}

template<char channel, typename Q, typename vertexType_result, typename vertexType_left,
        typename vertexType_right, class Bubble_Object>
Q
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right,
                Bubble_Object>::bubble_value_prefactor(){
    if constexpr (KELDYSH) return prefactor * (1. / (2. * M_PI * glb_i));
    else                   return prefactor * (1. / (2. * M_PI));
}

template<char channel, typename Q,
        typename vertexType_result,
        typename vertexType_left,
        typename vertexType_right,
        class Bubble_Object>
void
BubbleFunctionCalculator<channel, Q, vertexType_result, vertexType_left, vertexType_right, Bubble_Object>::check_presence_of_symmetry_related_contributions() {
    bool vertex1_is_bare = false;
    bool vertex2_is_bare = false;
    if ((vertex1.avertex().max_norm() < 1e-18)
        && (vertex1.pvertex().max_norm() < 1e-18)
        && (vertex1.tvertex().max_norm() < 1e-18)) {vertex1_is_bare = true;}
    if ((vertex2.avertex().max_norm() < 1e-18)
        && (vertex2.pvertex().max_norm() < 1e-18)
        && (vertex2.tvertex().max_norm() < 1e-18)) {vertex2_is_bare = true;}

    if (channel == 'a'){ // There must be a non-vanishing contribution in the t-channel for both vertices
        if (not vertex1_is_bare) assert(vertex1.tvertex().max_norm() > 1e-18);
        if (not vertex2_is_bare) assert(vertex2.tvertex().max_norm() > 1e-18);
    }
    if (channel == 't'){ // There must be a non-vanishing contribution in the a-channel for both vertices
        if (not vertex1_is_bare) assert(vertex1.avertex().max_norm() > 1e-18);
        if (not vertex2_is_bare) assert(vertex2.avertex().max_norm() > 1e-18);
    }
}



/**
 * Function to connect two vertices with a Bubble, i.e. a propagator pair.
 * Graphically in the `a` channel:
 * ```
 *   -->--|-----|--->---|-----|-->--
 *        | Γ_1 |   Π   | Γ_2 |
 *   --<--|-----|---<---|-----|--<--
 * ```
 * @tparam vertexType_result Type of the vertex that results in the bubble computation.
 * @tparam vertexType_left Type of the vertex that enters as the left part of the computation.
 * @tparam vertexType_right Type of the vertex that enters as the right part of the computation.
 * @tparam Bubble_Object Type of the Bubble object to connect the two vertices
 * @param dgamma Reference to the vertex that the result of the computation shall be added to.
 * @param vertex1 Reference to the left input vertex.
 * @param vertex2 Reference to the right input vertex.
 * @param Pi Reference to the Bubble.
 * @param channel Two-particle channel of the computation. Can be a, p, or t.
 * @param config Struct with essential parameters.
 * @param tobecomputed Array of three booleans, specifying what diagrammatic classes shall be computed.
 */
template <
        typename vertexType_result,
        typename vertexType_left,
        typename vertexType_right,
        class Bubble_Object>
void bubble_function(vertexType_result& dgamma,
                                 const vertexType_left& vertex1,
                                 const vertexType_right& vertex2,
                                 const Bubble_Object& Pi,
                                 const char channel,
                                 const fRG_config& config,
                                 const std::array<bool,3> tobecomputed = {true,true,true}){
    using Q = typename vertexType_result::base_type;
    if (channel == 'a') {
        BubbleFunctionCalculator<'a', Q, vertexType_result, vertexType_left, vertexType_right, Bubble_Object>
                BubbleComputer (dgamma, vertex1, vertex2, Pi, config.number_of_nodes,tobecomputed);
        BubbleComputer.perform_computation();
    }
    else if (channel == 'p') {
        BubbleFunctionCalculator<'p', Q, vertexType_result, vertexType_left, vertexType_right, Bubble_Object>
                BubbleComputer (dgamma, vertex1, vertex2, Pi, config.number_of_nodes,tobecomputed);
        BubbleComputer.perform_computation();
    }
    else if (channel == 't') {
        BubbleFunctionCalculator<'t', Q, vertexType_result, vertexType_left, vertexType_right, Bubble_Object>
                BubbleComputer (dgamma, vertex1, vertex2, Pi, config.number_of_nodes,tobecomputed);
        BubbleComputer.perform_computation();
    }
    //else {utils::print("Error! Incompatible channel given to bubble_function. Abort"); }

}

/**
 * Overload of the bubble_function in case no Bubble object has been initialized yet.
 * @tparam Q Template parameter specifying the type of the data.
 * @tparam vertexType_result Type of the vertex that results in the bubble computation.
 * @tparam vertexType_left Type of the vertex that enters as the left part of the computation.
 * @tparam vertexType_right Type of the vertex that enters as the right part of the computation.
 * @param dgamma Reference to the vertex that the result of the computation shall be added to.
 * @param vertex1 Reference to the left input vertex.
 * @param vertex2 Reference to the right input vertex.
 * @param G Propagator to be used for the Bubble.
 * @param S Single-scale propagator to be used for the Bubble. Only used if diff=true.
 * @param channel Two-particle channel of the computation. Can be a, p, or t.
 * @param diff Boolean specifying whether the Bubble is differentiated or not.
 * @param config Struct with essential parameters.
 * @param tobecomputed Array of three booleans, specifying what diagrammatic classes shall be computed.
 */
template <typename Q,
        typename vertexType_result,
        typename vertexType_left,
        typename vertexType_right>
void bubble_function(vertexType_result& dgamma,
                     const vertexType_left& vertex1,
                     const vertexType_right& vertex2,
                     const Propagator<Q>& G, const Propagator<Q>& S, const char channel, const bool diff, const fRG_config& config, const std::array<bool,3> tobecomputed = {true,true,true}){
    Bubble<Q> Pi(G, S, diff);
    bubble_function(dgamma, vertex1, vertex2, Pi, channel, config, tobecomputed);
}

#endif //KELDYSH_MFRG_BUBBLE_FUNCTION_HPP
