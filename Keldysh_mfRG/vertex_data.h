#ifndef FPP_MFRG_VERTEX_DATA_H
#define FPP_MFRG_VERTEX_DATA_H

/**
 * This header contains a class which is responsible for saving and retrieving vertex data
 */

#include "utilities/template_utils.h"
#include "utilities/math_utils.h"
#include "symmetries/Keldysh_symmetries.h"
#include "data_structures.h"          // real/complex vector classes
#include "parameters/master_parameters.h"               // system parameters (lengths of vectors etc.)
#include "parameters/frequency_parameters.h"
#include "grids/frequency_grid.h"            // functionality for the internal structure of the Hubbard model



template <typename Q> class rvert; // forward declaration of rvert
template <typename Q> class fullvert; // forward declaration of fullvert
template <typename Q> class State; // forward declaration of State
template <typename Q, template <typename> class symmetry_type> class GeneralVertex;
template <typename Q>class symmetric;
template <typename Q>using Vertex = GeneralVertex<Q, symmetric>;
class Buffer;

/**
 * Offers basic functionality that is identical for all K_classes
 * @tparam Q        data type of vertex data
 * @tparam rank     rank of data tensor
 */
template <typename Q, size_t rank>
class vertexContainerBase {
private:
    /**
     * Flattens multiIndex
     *    multiIndex convention for
     *    K1 --> iK, iw,          i_in
     *    K2 --> iK, iw, iv,      i_in
     *    K3 --> iK, iw, iv, ivp, i_in
     * If the flag FREQ_PADDING == 0, then the vertex data is stored as usual. With N frequency points one needs the indices iw in [0, N-1].
     * If the flag FREQ_PADDING == 1, then the vertex data is padded with zeros (corresponding to the asymptotic value at infinity.)
     * The values at infinity are accessed via the indices -1 and N.
     * Hence, to read out the regular data one needs the indices iw in [0, N-1]
     * e.g. for K1 and FREQ_PADDING == 1: There are nK_K1 Keldysh components, nBOS+2 frequency points and n_in internal dof.
     *                                    => flattenIndex(iK, iw, i_in) returns (iK * (nBOS+2) + iw+1) * n_in + i_in
     */
    template <typename... Types,    /// "..." is syntax for a parameter pack
            typename std::enable_if_t<(sizeof...(Types) == rank) and (are_all_integral<size_t, Types...>::value), bool> = true> /// checks that the number of arguments is rank and their types are integral (int, size_t etc.)
    size_t flattenIndex(const Types &... i) const {
#if FREQ_PADDING == 0
        return getFlatIndex({static_cast<size_t>(i)...}, dims);
#else
        std::array<size_t,rank> idx = {static_cast<size_t>(i+1)...};
        idx[0] -= 1; idx[rank-1] -= 1;

        size_t flatidx = getFlatIndex(idx, dims);
        assert(flatidx < data.size());
        return flatidx;
#endif
    }

protected:
    std::array<size_t,rank> dims;
    vec<Q> data;

public:
    /// constructor:
    explicit vertexContainerBase(const std::vector<size_t> dims_in) {
        assert(dims_in.size() == rank);
        for (size_t i = 0; i < rank; i++) dims[i] = dims_in[i];
    };
    /// Wrappers for above constructor - currently unused
    template <typename... Types,
            typename std::enable_if_t<(sizeof...(Types) == rank) and (are_all_integral<size_t, Types...>::value), bool> = true>
    explicit vertexContainerBase(const Types &... dims) : vertexContainerBase(std::vector<size_t>({static_cast<size_t>(dims)...})) {};
    vertexContainerBase(const size_t dims_in[rank], const vec<Q> &data_in) : data(data_in) {};

    /// Reserves space for the data and fills it with zeros
    void reserve() { data = vec<Q>(getFlatSize<rank>(dims)); }

    /// Access vertex data via flattened index. Only use this if you know what you are doing.
    Q acc(const size_t flatIndex) const {assert(flatIndex<data.size()); return data[flatIndex];}

    void direct_set(const size_t flatIndex, Q value) {assert(flatIndex < data.size()); data[flatIndex] = value;}

    /// Returns value for a multiIndex
    template <typename... Types,
            typename std::enable_if_t<(sizeof...(Types) == rank) and (are_all_integral<size_t, Types...>::value), bool> = true>
    Q val(const Types &... i) const {return data[flattenIndex(i...)];}
    /// Returns reference to a value for a multiIndex
    template <typename... Types,
            typename std::enable_if_t<(sizeof...(Types) == rank) and (are_all_integral<size_t, Types...>::value), bool> = true>
    const Q& at(const Types &... i) const {return data[flattenIndex(i...)];}
    /// Sets a value at a multiIndex
    template <typename... Types
            ,typename std::enable_if_t<(sizeof...(Types) == rank) and (are_all_integral<size_t, Types...>::value), bool> = true
                    >
    void setvert(const Q value, const Types &... i) {data[flattenIndex(i...)] = value;
    }

    auto get_dims() const {return dims;}

    /// Returns the vector containing the vertex data
    vec<Q> get_vec() const {return data;}
    /// Sets the data
    void set_vec(const vec<Q> &data_in) {assert(data.size() == data_in.size()); data = data_in;}
    /// Adds a vector to the data
    void add_vec(const vec<Q> &summand) {
#if FREQ_PADDING == 0
        assert(getFlatSize<rank>(dims) == summand.size()); /// Check that summand has the right length
        data += summand;
#else
        /// For FREQ_PADDING == 1 the vector summand does not contain the asymtotic values. Hence it needs to be padded with zeros at +/- infinity first.
        std::array<size_t,rank> dims_no_padding = dims; /// dims of summand
        for (size_t i = 1; i < rank-1; i++) dims_no_padding[i] -= 2*FREQ_PADDING;
        assert(getFlatSize<rank>(dims_no_padding) == summand.size()); /// Check that summand has the right length

        /// Translate the flat index of summand into a flat index of the padded data
        std::array<size_t,rank> multIndex;
        for (size_t i = 0; i < summand.size(); i++) {
            getMultIndex(multIndex, i, dims_no_padding); /// multiIndex of summand
            for (size_t j = 1; j < rank-1; j++) multIndex[j] += FREQ_PADDING; /// frequency indices need to be shifted by FREQ_PADDING(==1)
            size_t idx = getFlatIndex(multIndex, dims); /// gets flat index for accessing the correct value in data
            assert(idx < data.size());
            data[idx] += summand[i];
        }
#endif
    }


};

//template <K_class k, typename Q>
//class vertexDataContainer{};

template<K_class k, typename Q>
class vertexDataContainer: public vertexContainerBase<Q,4> {
    friend void test_PT4(double Lambda, bool write_flag);
    template <typename T> friend void test_PT_state(std::string outputFileName, double Lambda, bool write_flag);
    template <typename T> friend void result_set_frequency_grids(State<T>& result, Buffer& buffer);
    template<typename T> friend void check_FDTs(const State<T>& state, bool verbose);
    template<typename T> friend rvert<T> operator+ (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator+= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator- (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator-= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator* (rvert<T> lhs, const double& alpha);
    template<typename T> friend rvert<T> rvert<T>::operator*= (double alpha);

protected:
    VertexFrequencyGrid<k2> frequencies_K2;    // frequency grid
public:
    explicit vertexDataContainer(double Lambda) : frequencies_K2(Lambda), vertexContainerBase<Q,4>(dimsK2) { };

    /// Functions for getting and setting the frequency grid and its members

    /**
     * gets the frequency corresponding to the frequency index i
     * If FREQ_PADDING == 0: nothing unusual, for N frequency points i ranges in [0,N-1]
     * If FREQ_PADDING == 1: frequency grid is padded at +/- infinity
     *                       -infinity corresponds to i = -1
     *                       +infinity corresponds to i =  N
     */
    void K2_get_freqs_w(double& w, double& v, int iw, int iv) const;
    void K2_get_freqs_aux(double& w, double& v, int iw, int iv) const;

    auto K2_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k2>&;
    void K2_set_VertexFreqGrid(const VertexFrequencyGrid<k2> frequencyGrid);

    const double& K2_get_wlower_b() const;
    const double& K2_get_wupper_b() const;
    const double& K2_get_wlower_f() const;
    const double& K2_get_wupper_f() const;
    const FrequencyGrid& K2_get_freqGrid_b() const;
    const FrequencyGrid& K2_get_freqGrid_f() const;
    const double& K2_get_tlower_b_aux() const;
    const double& K2_get_tupper_b_aux() const;
    const double& K2_get_tlower_f_aux() const;
    const double& K2_get_tupper_f_aux() const;
    auto K2_gridtransf_b(double w) const -> double;
    auto K2_gridtransf_f(double w) const -> double;
    auto K2_gridtransf_inv_b(double w) const -> double;
    auto K2_gridtransf_inv_f(double w) const -> double;


    auto K2_get_correction_MFfiniteT(int iw) const -> double;


    vec<Q> get_deriv_K2_x (int order=5) const;
    vec<Q> get_deriv_K2_y (int order=5) const;
    vec<Q> get_deriv_K2_xy(int order=5) const;
    vec<Q> get_deriv_K2_xx(int order=5) const;
    vec<Q> get_deriv_K2_yy(int order=5) const;
    double get_deriv_maxK2() const;
    auto get_curvature_maxK2() const -> double;

    double analyze_tails_K2_x() const;

    double analyze_tails_K2_y() const;
    auto shrink_freq_box(const double rel_tail_threshold, bool verbose=true) const -> VertexFrequencyGrid<k2>;

};


/**
 * vertex data container for K1
 */
template<typename Q>
class vertexDataContainer<k1, Q> : public vertexContainerBase<Q,3>{
    friend void check_Kramers_Kronig(std::string filename);
    friend void test_PT4(double Lambda, bool write_flag);
    template <typename T> friend void test_PT_state(std::string outputFileName, double Lambda, bool write_flag);
    template <typename T> friend void result_set_frequency_grids(State<T>& result, Buffer& buffer);
    template<typename T> friend void susceptibilities_postprocessing(Vertex<T>& chi, Vertex<T>& chi_diff, const State<T>& state, double Lambda);
    template<typename T> friend rvert<T> operator+ (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator+= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator- (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator-= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator* (rvert<T> lhs, const double& alpha);
    template<typename T> friend rvert<T> rvert<T>::operator*= (double alpha);


protected:
    VertexFrequencyGrid<k1> frequencies_K1;    // frequency grid
public:

    explicit vertexDataContainer(double Lambda) : frequencies_K1(Lambda), vertexContainerBase<Q,3>(dimsK1) { };

    /// Functions for getting and setting frequency grid and its members;
    /// TODO: Can probably be shifted to vertexContainerBase    (problems: non-existing fermionic grid for K1 ==> use std::enable_if; need multiple parameter packs for get_freqs()
    /// Or shift these to the VertexFrequencyGrid class?
    /// For now I keep this to retain flexibility, e.g. to store data on very different grids in K1/K2/K3

    /**
     * gets the frequency corresponding to the frequency index i
     * If FREQ_PADDING == 0: nothing unusual, for N frequency points i ranges in [0,N-1]
     * If FREQ_PADDING == 1: frequency grid is padded at +/- infinity
     *                       -infinity corresponds to i = -1
     *                       +infinity corresponds to i =  N
     */
    void K1_get_freq_w(double& w, int i) const;     /// returns regular frequency
    void K1_get_freq_aux(double& w, int i) const;   /// returns frequency on the auxiliary grid

    auto K1_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k1>&;
    void K1_set_VertexFreqGrid(const VertexFrequencyGrid<k1> frequencyGrid);

    auto K1_get_freqGrid() const -> const FrequencyGrid&;

    const double& K1_get_wlower() const;
    const double& K1_get_wupper() const;
    const double& K1_get_tlower_aux() const;
    const double& K1_get_tupper_aux() const;
    auto K1_gridtransf(double w) const -> double;
    auto K1_gridtransf_inv(double w) const -> double;

    /// Compute derivative in w-direction using a finite-differences method
    vec<Q> get_deriv_K1_x(int order=5) const;
    /// Compute the maximum norm of the gradient
    double get_deriv_maxK1() const;
    /// Compute the maximum norm of the curvature
    double get_curvature_maxK1() const;

    double analyze_tails_K1() const;

    /// shrink the frequency box if the data on the outermost gridpoints is smaller than data.maxnorm()*rel_tail_threshold
    auto shrink_freq_box(const double rel_tail_threshold) const -> VertexFrequencyGrid<k1>;
};


template <typename Q>
class vertexDataContainer<k3, Q>: public vertexContainerBase<Q,5> {
    friend void test_PT4(double Lambda, bool write_flag);
    template <typename T> friend void test_PT_state(std::string outputFileName, double Lambda, bool write_flag);
    template <typename T> friend void result_set_frequency_grids(State<T>& result, Buffer& buffer);
    template<typename T> friend void check_FDTs(const State<T>& state, bool verbose);
    template<typename T> friend rvert<T> operator+ (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator+= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator- (rvert<T> lhs, const rvert<T>& rhs);
    template<typename T> friend rvert<T> rvert<T>::operator-= (const rvert<T>& rhs);
    template<typename T> friend rvert<T> operator* (rvert<T> lhs, const double& alpha);
    template<typename T> friend rvert<T> rvert<T>::operator*= (double alpha);


protected:
    VertexFrequencyGrid<k3> frequencies_K3;    // frequency grid


public:
    explicit vertexDataContainer(double Lambda) : frequencies_K3(Lambda), vertexContainerBase<Q,5>(dimsK3) { };


    void K3_get_freqs_w(double& w, double& v, double& vp, int iw, int iv, int ivp, char channel) const;
    void K3_get_freqs_aux(double& w, double& v, double& vp, int iw, int iv, int ivp) const;

    auto K3_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k3>&;
    void K3_set_VertexFreqGrid(const VertexFrequencyGrid<k3> frequencyGrid);
    const FrequencyGrid& K3_get_freqGrid_b() const;
    const FrequencyGrid& K3_get_freqGrid_f() const;

    const double& K3_get_wlower_b() const;
    const double& K3_get_wupper_b() const;
    const double& K3_get_wlower_f() const;
    const double& K3_get_wupper_f() const;
    const double& K3_get_tlower_b_aux() const;
    const double& K3_get_tupper_b_aux() const;
    const double& K3_get_tlower_f_aux() const;
    const double& K3_get_tupper_f_aux() const;
    auto K3_gridtransf_b(double w) const -> double;
    auto K3_gridtransf_f(double w) const -> double;
    auto K3_gridtransf_inv_b(double w) const -> double;
    auto K3_gridtransf_inv_f(double w) const -> double;

    auto K3_get_correction_MFfiniteT(int iw) const -> double;


    vec<Q> get_deriv_K3_x  (int order=5) const;
    vec<Q> get_deriv_K3_y  (int order=5) const;
    vec<Q> get_deriv_K3_z  (int order=5) const;
    vec<Q> get_deriv_K3_xy (int order=5) const;
    vec<Q> get_deriv_K3_xz (int order=5) const;
    vec<Q> get_deriv_K3_yz (int order=5) const;
    vec<Q> get_deriv_K3_xx (int order=5) const;
    vec<Q> get_deriv_K3_yy (int order=5) const;
    vec<Q> get_deriv_K3_zz (int order=5) const;
    vec<Q> get_deriv_K3_xyz(int order=5) const;
    double get_deriv_maxK3() const;
    auto get_curvature_maxK3() const -> double;
    double analyze_tails_K3_x() const;
    double analyze_tails_K3_y() const;
    double analyze_tails_K3_z() const;

    auto shrink_freq_box(const double rel_tail_threshold) const -> VertexFrequencyGrid<k3>;

};

/************************************ MEMBER FUNCTIONS OF THE VERTEX Data Container************************************/

/// K1:

template<typename Q>
auto vertexDataContainer<k1,Q>::K1_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k1>& {
    return frequencies_K1;
}
template<typename Q>
void vertexDataContainer<k1,Q>::K1_set_VertexFreqGrid(const VertexFrequencyGrid<k1> frequencyGrid) {
    frequencies_K1 = frequencyGrid;
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k2>& {
    return frequencies_K2;
}
template<K_class k, typename Q>
void vertexDataContainer<k,Q>::K2_set_VertexFreqGrid(const VertexFrequencyGrid<k2> frequencyGrid) {
    frequencies_K2 = frequencyGrid;
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_get_VertexFreqGrid() const -> const VertexFrequencyGrid<k3> & {
    return frequencies_K3;
}
template<typename Q>
void vertexDataContainer<k3,Q>::K3_set_VertexFreqGrid(const VertexFrequencyGrid<k3> frequencyGrid) {
    frequencies_K3 = frequencyGrid;
}

template<typename Q>
const double& vertexDataContainer<k1,Q>::K1_get_wlower() const {
    return frequencies_K1.b.w_lower;
}
template<typename Q>
const double& vertexDataContainer<k1,Q>::K1_get_wupper() const {
    return frequencies_K1.b.w_upper;
}
template<typename Q>
auto vertexDataContainer<k1,Q>::K1_get_freqGrid() const -> const FrequencyGrid& {
    return frequencies_K1.b;
}
template<typename Q>
void vertexDataContainer<k1,Q>::K1_get_freq_w(double& w, const int i) const {
    w = frequencies_K1.b.get_ws(i);
}
template<typename Q>
const double& vertexDataContainer<k1,Q>::K1_get_tlower_aux() const {
    return frequencies_K1.b.t_lower;
}
template<typename Q>
const double& vertexDataContainer<k1,Q>::K1_get_tupper_aux() const {
    return frequencies_K1.b.t_upper;
}
template<typename Q>
void vertexDataContainer<k1,Q>::K1_get_freq_aux(double& w, const int i) const {
    w = frequencies_K1.b.get_ts(i);
}
template<typename Q>
auto vertexDataContainer<k1,Q>::K1_gridtransf(double w) const -> double {
    return frequencies_K1.b.grid_transf(w);
}
template<typename Q>
auto vertexDataContainer<k1,Q>::K1_gridtransf_inv(double w) const -> double {
    return frequencies_K1.b.grid_transf_inv(w);
}


template <typename Q> auto vertexDataContainer<k1,Q>::get_deriv_K1_x(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,3>(vertexContainerBase<Q,3>::data, frequencies_K1.b.ts, vertexContainerBase<Q,3>::dims, 1, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k1,Q>::get_deriv_maxK1() const -> double {
    int order = 3;
    double dt = K1_get_freqGrid().dt;
    double Kmax = vertexContainerBase<Q,3>::get_vec().max_norm();
    double max_K1 = ::power2(get_deriv_K1_x(order)*dt*(1/Kmax)).max_norm();  // normalize by magnitude of vertex contribution and grid spacing
    return max_K1;
}
template <typename Q> auto vertexDataContainer<k1,Q>::get_curvature_maxK1() const -> double {
    int order = 3;
    double dt = K1_get_freqGrid().dt;
    double Kmax = vertexContainerBase<Q,3>::get_vec().max_norm();
    double max_K1 = ::power2( ::partial_deriv<Q,3>(::partial_deriv<Q,3>(vertexContainerBase<Q,3>::data, frequencies_K1.b.ts, vertexContainerBase<Q,3>::dims, 1, order), frequencies_K1.b.ts, vertexContainerBase<Q,3>::dims, 1, order)*dt*dt*(1/Kmax)).max_norm();
    return max_K1;
}


template <typename Q> double vertexDataContainer<k1,Q>::analyze_tails_K1() const {
    double maxabs_K1_total = vertexContainerBase<Q,3>::data.max_norm();
    vec<double> maxabsK1_along_w = maxabs(vertexContainerBase<Q,3>::data, vertexContainerBase<Q,3>::dims, 1);

    return maxabsK1_along_w[FREQ_PADDING] / maxabs_K1_total;
}


template <typename Q> auto vertexDataContainer<k1,Q>::shrink_freq_box(const double rel_tail_threshold) const -> VertexFrequencyGrid<k1>
{
    double maxmax = vertexContainerBase<Q,3>::data.max_norm();
    vec<double> maxabsK1_along_w = maxabs(vertexContainerBase<Q,3>::data, vertexContainerBase<Q,3>::dims, 1) * (1/maxmax);
    VertexFrequencyGrid<k1> frequencies_new = frequencies_K1;
    if (std::abs(maxmax) > 1e-30) { frequencies_new.b = freqGrid::shrink_freq_box(frequencies_K1.b, rel_tail_threshold, maxabsK1_along_w); }

    return frequencies_new;
}


/// K2:

template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_wlower_b() const {
    return frequencies_K2.b.w_lower;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_wupper_b() const {
    return frequencies_K2.b.w_upper;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_wlower_f() const {
    return frequencies_K2.f.w_lower;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_wupper_f() const {
    return frequencies_K2.f.w_upper;
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_get_freqGrid_b() const -> const FrequencyGrid& {
    return frequencies_K2.b;
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_get_freqGrid_f() const -> const FrequencyGrid& {
    return frequencies_K2.f;
}
template<K_class k, typename Q>
void vertexDataContainer<k,Q>::K2_get_freqs_w(double &w, double &v, const int iw, const int iv) const {
    frequencies_K2.get_freqs_w(w, v, iw, iv);
}

template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_tlower_b_aux() const {
    return frequencies_K2.b.t_lower;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_tupper_b_aux() const {
    return frequencies_K2.b.t_upper;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_tlower_f_aux() const {
    return frequencies_K2.f.t_lower;
}
template<K_class k, typename Q>
const double& vertexDataContainer<k,Q>::K2_get_tupper_f_aux() const {
    return frequencies_K2.f.t_upper;
}
template<K_class k, typename Q>
void vertexDataContainer<k,Q>::K2_get_freqs_aux(double &w, double &v, const int iw, const int iv) const {
    frequencies_K2.get_freqs_aux(w, v, iw, iv);
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_gridtransf_b(double w) const -> double {
    return frequencies_K2.b.grid_transf(w);
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_gridtransf_f(double w) const -> double {
    return frequencies_K2.f.grid_transf(w);
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_gridtransf_inv_b(double w) const -> double {
    return frequencies_K2.b.grid_transf_inv(w);
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_gridtransf_inv_f(double w) const -> double {
    return frequencies_K2.f.grid_transf_inv(w);
}
template<K_class k, typename Q>
auto vertexDataContainer<k,Q>::K2_get_correction_MFfiniteT(int iw) const -> double {
    if (not KELDYSH and not ZERO_T)
        return signFlipCorrection_MF(frequencies_K2.b.get_ws(iw));
    else return 0.;
}

template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_K2_x(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,4>(vertexContainerBase<Q,4>::data, frequencies_K2.b.ts, vertexContainerBase<Q,4>::dims, 1, order);
    return result;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_K2_y(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,4>(vertexContainerBase<Q,4>::data, frequencies_K2.f.ts, vertexContainerBase<Q,4>::dims, 2, order);
    return result;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_K2_xy(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,4>(vertexContainerBase<Q,4>::data, frequencies_K2.f.ts, vertexContainerBase<Q,4>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,4>(inter_result, frequencies_K2.b.ts, vertexContainerBase<Q,4>::dims, 1, order);
    return result;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_K2_xx(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,4>(vertexContainerBase<Q,4>::data, frequencies_K2.b.ts, vertexContainerBase<Q,4>::dims, 1, order);
    vec<Q> result       = ::partial_deriv<Q,4>(inter_result, frequencies_K2.b.ts, vertexContainerBase<Q,4>::dims, 1, order);
    return result;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_K2_yy(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,4>(vertexContainerBase<Q,4>::data, frequencies_K2.f.ts, vertexContainerBase<Q,4>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,4>(inter_result, frequencies_K2.f.ts, vertexContainerBase<Q,4>::dims, 2, order);
    return result;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_deriv_maxK2() const -> double {
    const int order = 3;

    double Kmax = vertexContainerBase<Q,4>::get_vec().max_norm();
    double dtb = K2_get_freqGrid_b().dt;
    double dtf = K2_get_freqGrid_f().dt;
    double max_K2 = (     ::power2(get_deriv_K2_x(order)*dtb*(1/Kmax)) // multiply by dt to make the result independent of the grid spacing
                        + ::power2(get_deriv_K2_y(order)*dtf*(1/Kmax))
                    ).max_norm();
    return max_K2;
}
template <K_class k, typename Q> auto vertexDataContainer<k,Q>::get_curvature_maxK2() const -> double {
    const int order = 3;

    double Kmax = vertexContainerBase<Q,4>::get_vec().max_norm();
    double dtb = K2_get_freqGrid_b().dt;
    double dtf = K2_get_freqGrid_f().dt;
    double max_K1 = (    ::power2( get_deriv_K2_xx(order)*dtb*dtb*(1/Kmax))
                        +::power2( get_deriv_K2_yy(order)*dtf*dtf*(1/Kmax))
                        +::power2( get_deriv_K2_xy(order)*dtb*dtf*(1/Kmax))*2.
                    ).max_norm();
    return max_K1;
}

template <K_class k, typename Q> double vertexDataContainer<k,Q>::analyze_tails_K2_x() const {
    double maxmax = vertexContainerBase<Q,4>::data.max_norm();
    vec<double> maxabsK2_along_w = maxabs(vertexContainerBase<Q,4>::data, vertexContainerBase<Q,4>::dims, 1) * (1/maxmax);

    return maxabsK2_along_w[FREQ_PADDING];
}
template <K_class k, typename Q> double vertexDataContainer<k,Q>::analyze_tails_K2_y() const {
    double maxmax = vertexContainerBase<Q,4>::data.max_norm();
    vec<double> maxabsK2_along_v = maxabs(vertexContainerBase<Q,4>::data, vertexContainerBase<Q,4>::dims, 2) * (1/maxmax);

    return maxabsK2_along_v[FREQ_PADDING];
}

template <K_class k, typename Q> auto vertexDataContainer<k,Q>::shrink_freq_box(const double rel_tail_threshold, const bool verbose) const -> VertexFrequencyGrid<k2> {

    VertexFrequencyGrid<k2> frequencies_new = frequencies_K2;

    double maxmax = vertexContainerBase<Q,4>::data.max_norm();
    vec<double> maxabsK2_along_w = maxabs(vertexContainerBase<Q,4>::data, vertexContainerBase<Q,4>::dims, 1) * (1/maxmax);
    vec<double> maxabsK2_along_v = maxabs(vertexContainerBase<Q,4>::data, vertexContainerBase<Q,4>::dims, 2) * (1/maxmax);


    frequencies_new.b = freqGrid::shrink_freq_box(frequencies_K2.b, rel_tail_threshold, maxabsK2_along_w, verbose);
    if (verbose and mpi_world_rank() == 0) std::cout << "in direction w to " << frequencies_new.b.w_upper << std::endl;
    frequencies_new.f = freqGrid::shrink_freq_box(frequencies_K2.f, rel_tail_threshold, maxabsK2_along_v, verbose);
    if (verbose and mpi_world_rank() == 0) std::cout << "in direction v to " << frequencies_new.f.w_upper << std::endl;

    return frequencies_new;
}


/// K3:

template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_wlower_b() const {
    return frequencies_K3.b.w_lower;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_wupper_b() const {
    return frequencies_K3.b.w_upper;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_wlower_f() const {
    return frequencies_K3.f.w_lower;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_wupper_f() const {
    return frequencies_K3.f.w_upper;
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_get_freqGrid_b() const -> const FrequencyGrid& {
    return frequencies_K3.b;
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_get_freqGrid_f() const -> const FrequencyGrid& {
    return frequencies_K3.f;
}
template<typename Q>
void vertexDataContainer<k3,Q>::K3_get_freqs_w(double &w, double &v, double& vp, const int iw, const int iv, const int ivp, const char channel) const {
    frequencies_K3.get_freqs_w(w, v, vp, iw, iv, ivp, channel);
}

template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_tlower_b_aux() const {
    return frequencies_K3.b.t_lower;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_tupper_b_aux() const {
    return frequencies_K3.b.t_upper;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_tlower_f_aux() const {
    return frequencies_K3.f.t_lower;
}
template<typename Q>
const double& vertexDataContainer<k3,Q>::K3_get_tupper_f_aux() const {
    return frequencies_K3.f.t_upper;
}
template<typename Q>
void vertexDataContainer<k3,Q>::K3_get_freqs_aux(double &w, double &v, double& vp, const int iw, const int iv, const int ivp) const {
    frequencies_K3.get_freqs_aux(w,v,vp, iw, iv, ivp);
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_gridtransf_b(double w) const -> double {
    return frequencies_K3.b.grid_transf(w);
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_gridtransf_f(double w) const -> double {
    return frequencies_K3.f.grid_transf(w);
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_gridtransf_inv_b(double w) const -> double {
    return frequencies_K3.b.grid_transf_inv(w);
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_gridtransf_inv_f(double w) const -> double {
    return frequencies_K3.f.grid_transf_inv(w);
}
template<typename Q>
auto vertexDataContainer<k3,Q>::K3_get_correction_MFfiniteT(int iw) const -> double {
    if (not KELDYSH and not ZERO_T)
    return floor2bfreq(frequencies_K3.b.get_ws(iw) / 2) - ceil2bfreq(frequencies_K3.b.get_ws(iw) / 2);
    else return 0.;
}


template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_x(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_y(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_z(const int order) const -> vec<Q> {
    vec<Q> result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_xy(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_xz(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_yz(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_xx(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_yy(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_zz(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    return result;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_K3_xyz(const int order) const -> vec<Q> {
    vec<Q> inter_result = ::partial_deriv<Q,5>(vertexContainerBase<Q,5>::data, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 3, order);
    vec<Q> inter_result2= ::partial_deriv<Q,5>(                  inter_result, frequencies_K3.f.ts, vertexContainerBase<Q,5>::dims, 2, order);
    vec<Q> result       = ::partial_deriv<Q,5>(                 inter_result2, frequencies_K3.b.ts, vertexContainerBase<Q,5>::dims, 1, order);
    return result;
}

template <typename Q> auto vertexDataContainer<k3,Q>::get_deriv_maxK3() const -> double {
    const int order = 3;

    double Kmax = vertexContainerBase<Q,5>::get_vec().max_norm();
    double dtb = K3_get_freqGrid_b().dt;
    double dtf = K3_get_freqGrid_f().dt;
    double max_K3 = (::power2(get_deriv_K3_z(order) * dtf * (1/Kmax))
                   + ::power2(get_deriv_K3_y(order) * dtf * (1/Kmax))
                   + ::power2(get_deriv_K3_x(order) * dtb * (1/Kmax))
    ).max_norm();
    return max_K3;
}
template <typename Q> auto vertexDataContainer<k3,Q>::get_curvature_maxK3() const -> double {
    const int order = 3;

    double Kmax = vertexContainerBase<Q,5>::get_vec().max_norm();
    double dtb = K3_get_freqGrid_b().dt;
    double dtf = K3_get_freqGrid_f().dt;
    double max_K3 = (   ::power2(get_deriv_K3_xx(order)*dtb*dtb*(1/Kmax))
                      + ::power2(get_deriv_K3_yy(order)*dtf*dtf*(1/Kmax))
                      + ::power2(get_deriv_K3_zz(order)*dtf*dtf*(1/Kmax))
                      +(::power2(get_deriv_K3_xy(order)*dtb*dtf*(1/Kmax))
                      + ::power2(get_deriv_K3_xz(order)*dtb*dtf*(1/Kmax))
                      + ::power2(get_deriv_K3_yz(order)*dtf*dtf*(1/Kmax)))*2.
                    ).max_norm();
    return max_K3;
}

template <typename Q> double vertexDataContainer<k3,Q>::analyze_tails_K3_x() const {
    double maxabs_K3_total = vertexContainerBase<Q,5>::data.max_norm();
    vec<double> maxabsK3_along_w = maxabs(vertexContainerBase<Q,5>::data, vertexContainerBase<Q,5>::dims, 1);

    return maxabsK3_along_w[1] / maxabs_K3_total;
}
template <typename Q> double vertexDataContainer<k3,Q>::analyze_tails_K3_y() const {
    double maxabs_K3_total = vertexContainerBase<Q,5>::data.max_norm();
    vec<double> maxabsK3_along_v = maxabs(vertexContainerBase<Q,5>::data, vertexContainerBase<Q,5>::dims, 2);

    return maxabsK3_along_v[1] / maxabs_K3_total;
}
template <typename Q> double vertexDataContainer<k3,Q>::analyze_tails_K3_z() const {
    double maxabs_K3_total = vertexContainerBase<Q,5>::data.max_norm();
    vec<double> maxabsK3_along_vp = maxabs(vertexContainerBase<Q,5>::data, vertexContainerBase<Q,5>::dims, 3);

    return maxabsK3_along_vp[1] / maxabs_K3_total;
}

template <typename Q> auto vertexDataContainer<k3,Q>::shrink_freq_box(const double rel_tail_threshold) const -> VertexFrequencyGrid<k3> {

    VertexFrequencyGrid<k3> frequencies_new = frequencies_K3;

    double maxmax = vertexContainerBase<Q,5>::data.max_norm();
    vec<double> maxabsK3_along_w = maxabs(vertexContainerBase<Q,5>::data, vertexContainerBase<Q,5>::dims, 1) * (1/maxmax);
    vec<double> maxabsK3_along_v = maxabs(vertexContainerBase<Q,5>::data, vertexContainerBase<Q,5>::dims, 2) * (1/maxmax);

    frequencies_new.b = freqGrid::shrink_freq_box(frequencies_K3.b, rel_tail_threshold, maxabsK3_along_w);
    frequencies_new.f = freqGrid::shrink_freq_box(frequencies_K3.f, rel_tail_threshold, maxabsK3_along_v);

    return frequencies_new;
}

#endif //FPP_MFRG_VERTEX_DATA_H
