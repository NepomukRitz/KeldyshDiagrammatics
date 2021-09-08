/**
 * Reducible vertex in channel r, split into diagrammatic classes K1, K2, K2b, K3.
 * Member functions allow for access of values at arbitrary frequencies and in each channel-specific
 * frequency representation.
 */

#ifndef KELDYSH_MFRG_R_VERTEX_H
#define KELDYSH_MFRG_R_VERTEX_H

#include "data_structures.h"          // real/complex vector classes
#include "parameters/master_parameters.h"               // system parameters (lengths of vectors etc.)
#include "symmetries/Keldysh_symmetries.h"       // transformations on Keldysh indices
#include "symmetries/internal_symmetries.h"      // symmetry transformations for internal indices (momentum etc.), currently trivial
#include "interpolations/vertex_interpolations.h"           // frequency interpolations for vertices
#include "symmetries/symmetry_transformations.h" // symmetry transformations of frequencies
#include "symmetries/symmetry_table.h"           // table containing information when to apply which symmetry transformations
#include "grids/momentum_grid.h"            // functionality for the internal structure of the Hubbard model

template <typename Q> class fullvert; // forward declaration of fullvert

template <typename Q>
class rvert{

private:
    vec<Q> empty_K2() { // for pure K1-calculation no memory should be allocated unnecessarily for K2
        if (MAX_DIAG_CLASS >= 2) return vec<Q> (nK_K2 * nw2 * nv2 * n_in);  // data points of K2;
        else                     return vec<Q> (0);                         // empty vector, never used in calculations
    }
    vec<Q> empty_K3() { // for  K2-calculation no memory should be allocated unnecessarily for K3
        if (MAX_DIAG_CLASS >= 3) return vec<Q> (nK_K3 * nw3 * nv3 * nv3 * n_in);    // data points of K3  // data points of K2;
        else                     return vec<Q> (0);                                 // empty vector, never used in calculations
    }

public:
    char channel;                       // reducibility channel
    Components components;              // lists providing information on how all Keldysh components are related to the
                                        // independent ones
    Transformations transformations;    // lists providing information on which transformations to apply on Keldysh
                                        // components to relate them to the independent ones
    FrequencyTransformations freq_transformations;  // lists providing information on which transformations to apply on
                                                    // frequencies to relate them to the independent ones
    FrequencyComponents freq_components;  // lists providing information on which transformations to apply on
    // frequencies to relate them to the independent ones

    VertexFrequencyGrid frequencies;    // frequency grid

    rvert(const char channel_in, double Lambda) : channel(channel_in), frequencies(Lambda),
                                                  components (Components(channel_in)),
                                                  transformations (Transformations(channel_in)),
                                                  freq_transformations (FrequencyTransformations(channel_in)),
                                                  freq_components (FrequencyComponents(channel_in)) { };

    /// Member functions for accessing the reducible vertex in channel r at arbitrary frequencies ///
    /// by interpolating stored data, in all possible channel-dependent frequency representations ///

    /**
     * Return the value of the reducible vertex in channel r.
     * @param input          : Combination of input arguments.
     * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
     *                         symmetry transformations that map between channels a <--> t.
     */
    auto value(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    /** Overload for accessing non-symmetric vertices, with
     * @param vertex_half2 : vertex related to the calling vertex by symmetry, needed for transformations with
     *                       asymmetry_transform=true */
    auto value(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;

    /**
     * Return the value of the vertex Ki in channel r.
     * @param input          : Combination of input arguments.
     * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
     *                         symmetry transformations that map between channels a <--> t.
     */
    template <K_class k>
    auto valsmooth(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    /** Overload for accessing non-symmetric vertices, with
     * @param vertex_half2 : vertex related to the calling vertex by symmetry, needed for transformations with
     *                       asymmetry_transform=true */
    template <K_class k>
    auto valsmooth(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;

    /** Parts of the r vertex that connect to the same/different bare vertices on the left/right of an r bubble */
    auto left_same_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    auto left_same_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;
    auto right_same_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    auto right_same_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;
    auto left_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    auto left_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;
    auto right_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    auto right_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q;

    /**
     * Transform the frequencies from the frequency convention of input.channel to the frequency convention of
     * this->channel. Necessary when accessing the r vertex from a different channel r'.
     */
    void transfToR(VertexInput& input) const;

    /**
     * Interpolate the vertex to updated grid when rescaling the grid to new flow parameter Lambda.
     */
    void update_grid(double Lambda);
    void update_grid_K1(const VertexFrequencyGrid& frequencies_new);
    void update_grid_K2(const VertexFrequencyGrid& frequencies_new);
    void update_grid_K3(const VertexFrequencyGrid& frequencies_new);

    /** K1-functionality */
    vec<Q> K1 = vec<Q> (nK_K1 * nw1 * n_in);  // data points of K1


    /// Member functions for accessing/setting values of the vector K1 ///

    /** Return the value of the vector K1 at index i. */
    auto K1_acc(int i) const -> Q;

    /** Set the value of the vector K1 at index i to "value". */
    void K1_direct_set(int i, Q value);

    /** Set the value of the vector K1 at Keldysh index iK, frequency index iw,
     * internal structure index i_in to "value". */
    void K1_setvert(int iK, int iw, int i_in, Q value);

    /** Add "value" to the value of the vector K1 at Keldysh index iK, frequency index iw,
     * internal structure index i_in. */
    void K1_addvert(int iK, int iw, int i_in, Q value);

    /** Return the value of the vector K1 at Keldysh index iK, frequency index iw,
     * internal structure index i_in. */
    auto K1_val(int iK, int iw, int i_in) const -> Q;

    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK1(const rvert<Q>& vertex_symmrelated);

    void K1_crossproject();
    Q K1_BZ_average(const int iK, const int iw);

    /** K2 functionality */
    vec<Q> K2 = empty_K2();

    /// Member functions for accessing/setting values of the vector K2 ///

    /** Return the value of the vector K2 at index i. */
    auto K2_acc(int i) const -> Q;

    /** Set the value of the vector K2 at index i to "value". */
    void K2_direct_set(int i, Q value);

    /** Set the value of the vector K2 at Keldysh index iK, frequency indices iw, iv,
     * internal structure index i_in to "value". */
    void K2_setvert(int iK, int iw, int iv, int i_in, Q value);

    /** Add "value" to the value of the vector K2 at Keldysh index iK, frequency indices iw, iv,
     * internal structure index i_in. */
    void K2_addvert(int iK, int iw, int iv, int i_in, Q value);

    /** Return the value of the vector K2 at Keldysh index iK, frequency indices iw, iv,
     * internal structure index i_in. */
    auto K2_val(int iK, int iw, int iv, int i_in) const -> Q;

    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK2(const rvert<Q>& vertex_symmrelated);

    // TODO: Implement! Needed for the Hubbard model.
    void K2_crossproject(char channel_out);

    /** K3 functionality */
    vec<Q> K3 = empty_K3();

    /// Member functions for accessing/setting values of the vector K3 ///

    /** Return the value of the vector K3 at index i. */
    auto K3_acc(int i) const -> Q;

    /** Set the value of the vector K3 at index i to "value". */
    void K3_direct_set(int i, Q value);

    /** Set the value of the vector K3 at Keldysh index iK, frequency indices iw, iv, ivp,
     * internal structure index i_in to "value". */
    void K3_setvert(int iK, int iw, int iv, int ivp, int i_in, Q);

    /** Add "value" to the value of the vector K3 at Keldysh index iK, frequency indices iw, iv, ivp,
     * internal structure index i_in. */
    void K3_addvert(int iK, int iw, int iv, int ivp, int i_in, Q);

    /** Return the value of the vector K3 at Keldysh index iK, frequency indices iw, iv, ivp,
     * internal structure index i_in. */
    auto K3_val(int iK, int iw, int iv, int ivp, int i_in) const -> Q;

    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK3(const rvert<Q>& vertex_symmrelated);

    // TODO: Implement! Needed for the Hubbard model.
    void K3_crossproject(char channel_out);


    auto operator+= (const rvert<Q>& rhs) -> rvert<Q> {
        if (MAX_DIAG_CLASS >= 0) this->K1 += rhs.K1;
        if (MAX_DIAG_CLASS >= 2) this->K2 += rhs.K2;
        if (MAX_DIAG_CLASS >= 3) this->K3 += rhs.K3;
        return *this;
    }
    friend rvert<Q> operator+ (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs += rhs;
        return lhs;
    }
    auto operator*= (double alpha) -> rvert<Q> {
        if (MAX_DIAG_CLASS >= 0) this->K1 *= alpha;
        if (MAX_DIAG_CLASS >= 2) this->K2 *= alpha;
        if (MAX_DIAG_CLASS >= 3) this->K3 *= alpha;
        return *this;
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const double& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator*= (const rvert<Q>& rhs) -> rvert<Q> {
        if (MAX_DIAG_CLASS >= 0) this->K1 *= rhs.K1;
        if (MAX_DIAG_CLASS >= 2) this->K2 *= rhs.K2;
        if (MAX_DIAG_CLASS >= 3) this->K3 *= rhs.K3;
        return *this;
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator-= (const rvert<Q>& rhs) -> rvert<Q> {
        if (MAX_DIAG_CLASS >= 0) this->K1 -= rhs.K1;
        if (MAX_DIAG_CLASS >= 2) this->K2 -= rhs.K2;
        if (MAX_DIAG_CLASS >= 3) this->K3 -= rhs.K3;
        return *this;
    }
    friend rvert<Q> operator- (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs -= rhs;
        return lhs;
    }

    double get_deriv_maxK1() const;
    double get_deriv_maxK2() const;
    double get_deriv_maxK3() const;
};

/****************************************** MEMBER FUNCTIONS OF THE R-VERTEX ******************************************/
template <typename Q> auto rvert<Q>::value(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {

    transfToR(input);

    Q K1_val, K2_val, K2b_val, K3_val {};   // force zero initialization

    if (MAX_DIAG_CLASS >= 0) K1_val = valsmooth<k1>(input, rvert_crossing);
    if (MAX_DIAG_CLASS >= 2) {
        K2_val = valsmooth<k2>(input, rvert_crossing);
        K2b_val = valsmooth<k2b>(input, rvert_crossing);
    }
    if (MAX_DIAG_CLASS >= 3) K3_val = valsmooth<k3>(input, rvert_crossing);

    return K1_val + K2_val + K2b_val + K3_val;
}
template <typename Q> auto rvert<Q>::value(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {

    transfToR(input);

    Q K1_val, K2_val, K2b_val, K3_val {};   // force zero initialization

    if (MAX_DIAG_CLASS >= 0) K1_val = valsmooth<k1>(input, rvert_crossing, vertex_half2);
    if (MAX_DIAG_CLASS >= 2) {
        K2_val = valsmooth<k2>(input, rvert_crossing, vertex_half2);
        K2b_val = valsmooth<k2b>(input, rvert_crossing, vertex_half2);
    }
    if (MAX_DIAG_CLASS >= 3) K3_val = valsmooth<k3>(input, rvert_crossing, vertex_half2);

    return K1_val + K2_val + K2b_val + K3_val;
}

template <typename Q>
template <K_class k>
auto rvert<Q>::valsmooth(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {

    IndicesSymmetryTransformations indices (input, channel);  // write input indices into transformable data structure

    Ti(indices, transformations.K[k][input.spin][input.iK]);  // apply necessary symmetry transformations
    indices.iK = components.K[k][input.spin][input.iK];  // check which symmetry-transformed component should be read
    if (indices.iK < 0) return 0.;  // components with label -1 in the symmetry table are zero --> return 0. directly

    Q value{};

    if (indices.channel != channel)
        // if the symmetry transformation switches between channels (a <--> t), return the interpolated value of the
        // r vertex in the channel related by crossing symmetry
        value = Interpolate<k,Q>()(indices, rvert_crossing);
    else
        // otherwise return the interpolated value of the calling r vertex
        value = Interpolate<k,Q>()(indices, *(this));

    if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate) return myconj(value);  // apply complex conjugation if T_C has been used

    assert(isfinite(value));
    return value;
}
template <typename Q>
template <K_class k>
auto rvert<Q>::valsmooth(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {

    IndicesSymmetryTransformations indices (input, channel);  // write input indices into transformable data structure

    Ti(indices, transformations.K[k][input.spin][input.iK]);  // apply necessary symmetry transformations
    indices.iK = components.K[k][input.spin][input.iK];  // check which symmetry-transformed component should be read
    if (indices.iK < 0) return 0.;  // components with label -1 in the symmetry table are zero --> return 0. directly

    Q value;

    // first check if the applied transformations switch between half 1 and half 2 of the vertex
    if (indices.asymmetry_transform) {
        // if yes, return the interpolated value of half 2 in the appropriate channel
        switch (channel) {
            case 'a':
                // calling vertex is in channel a
                if (indices.channel == 'a')
                    // if the applied transformation(s) do not switch between channels a,t, return a vertex of half 2
                    value = Interpolate<k,Q>()(indices, vertex_half2.avertex);
                else
                    // if they do switch between channels a,t, return t vertex of half 2
                    value = Interpolate<k,Q>()(indices, vertex_half2.tvertex);
                break;
            case 'p':
                // calling vertex is in channel p (no switching between channels -> return p vertex of half 2)
                value = Interpolate<k,Q>()(indices, vertex_half2.pvertex);
                break;
            case 't':
                // calling vertex is in channel t
                if (indices.channel == 't')
                    // if the applied transformation(s) do not switch between channels a,t, return t vertex of half 2
                    value = Interpolate<k,Q>()(indices, vertex_half2.tvertex);
                else
                    // if they do switch between channels a,t, return t vertex of half 2
                    value = Interpolate<k,Q>()(indices, vertex_half2.avertex);
                break;
            default:;
        }
    }
    else {
        // if no, return the interpolated value of half 1 in the appropriate channel
        if (indices.channel != channel)
            // if the symmetry transformation switches between channels (a <--> t), return the interpolated value of the
            // r vertex in the channel related by crossing symmetry
            value = Interpolate<k,Q>()(indices, rvert_crossing);
        else
            // otherwise return the interpolated value of the calling r vertex
            value = Interpolate<k,Q>()(indices, *(this));
    }

    if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate) return myconj(value);  // apply complex conjugation if T_C has been used

    assert(isfinite(value));
    return value;
}

template <typename Q> auto rvert<Q>::left_same_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {
    if (MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing);
    else if (MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing) + valsmooth<k2b>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::left_same_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {
    if (MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing, vertex_half2);
    else if (MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing, vertex_half2) + valsmooth<k2b>(input, rvert_crossing, vertex_half2);
}

template <typename Q> auto rvert<Q>::right_same_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {
    if (MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing);
    else if (MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing) + valsmooth<k2>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::right_same_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {
    if (MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing, vertex_half2);
    else if (MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing, vertex_half2) + valsmooth<k2>(input, rvert_crossing, vertex_half2);
}

template <typename Q> auto rvert<Q>::left_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {
    if (MAX_DIAG_CLASS == 1)      return 0.;
    else if (MAX_DIAG_CLASS == 2) return valsmooth<k2>(input, rvert_crossing);
    else if (MAX_DIAG_CLASS == 3) return valsmooth<k2>(input, rvert_crossing) + valsmooth<k3>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::left_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {
    if (MAX_DIAG_CLASS == 1)      return 0.;
    else if (MAX_DIAG_CLASS == 2) return valsmooth<k2>(input, rvert_crossing, vertex_half2);
    else if (MAX_DIAG_CLASS == 3) return valsmooth<k2>(input, rvert_crossing, vertex_half2) + valsmooth<k3>(input, rvert_crossing, vertex_half2);
}

template <typename Q> auto rvert<Q>::right_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {
    if (MAX_DIAG_CLASS == 1)      return 0.;
    else if (MAX_DIAG_CLASS == 2) return valsmooth<k2b>(input, rvert_crossing);
    else if (MAX_DIAG_CLASS == 3) return valsmooth<k2b>(input, rvert_crossing) + valsmooth<k3>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::right_diff_bare(VertexInput input, const rvert<Q>& rvert_crossing, const fullvert<Q>& vertex_half2) const -> Q {
    if (MAX_DIAG_CLASS == 1)      return 0.;
    else if (MAX_DIAG_CLASS == 2) return valsmooth<k2b>(input, rvert_crossing, vertex_half2);
    else if (MAX_DIAG_CLASS == 3) return valsmooth<k2b>(input, rvert_crossing, vertex_half2) + valsmooth<k3>(input, rvert_crossing, vertex_half2);
}


template <typename Q> void rvert<Q>::transfToR(VertexInput& input) const {
    double w, v1, v2;

    // Needed for finite-temperature Matsubara
    double floor2bf_w;
    double floor2bf_inputw = floor2bfreq(input.w / 2.);
    switch (channel) {
        case 'a':
            switch (input.channel) {
                case 'a':
                    return;                                    // do nothing
                case 'p':
                    if (KELDYSH || ZERO_T){
                        w  = -input.v1-input.v2;                   // input.w  = w_p
                        v1 = 0.5*(input.w+input.v1-input.v2);      // input.v1 = v_p
                        v2 = 0.5*(input.w-input.v1+input.v2);      // input.v2 = v'_p
                    }
                    else{
                        w  = -input.v1-input.v2 - input.w + 2 * floor2bf_inputw;    // input.w  = w_p
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = input.w + input.v1 + floor2bf_w - floor2bf_inputw;     // input.v1 = v_p
                        v2 = input.w + input.v2 + floor2bf_w - floor2bf_inputw;     // input.v2 = v'_p
                    }
                    break;
                case 't':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1-input.v2;                    // input.w  = w_t
                        v1 = 0.5*( input.w+input.v1+input.v2);     // input.v1 = v_t
                        v2 = 0.5*(-input.w+input.v1+input.v2);     // input.v2 = v'_t
                    }
                    else{
                        w  = input.v1-input.v2;                                     // input.w  = w_t
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = input.w + input.v2 + floor2bf_w - floor2bf_inputw;     // input.v1 = v_t
                        v2 = input.v1 + floor2bf_w - floor2bf_inputw;               // input.v2 = v'_t
                    }
                    break;
                case 'f':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1-input.v2;                    // input.w  = v_1'
                        v1 = 0.5*(2.*input.w+input.v1-input.v2);   // input.v1 = v_2'
                        v2 = 0.5*(input.v1+input.v2);              // input.v2 = v_1
                    }
                    else{
                        w  = input.v1 - input.v2;                                   // input.w  = v_1'
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = input.w - floor2bf_w;                                  // input.v1 = v_2'
                        v2 = input.v2 - floor2bf_w;                                 // input.v2 = v_1
                    }

                    break;
                default:;
            }
            break;
        case 'p':
            switch (input.channel) {
                case 'a':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1+input.v2;                    // input.w  = w_a
                        v1 = 0.5*(-input.w+input.v1-input.v2);     // input.v1 = v_a
                        v2 = 0.5*(-input.w-input.v1+input.v2);     // input.v2 = v'_a
                    }
                    else{
                        w  = input.v1 + input.v2 + input.w - 2 * floor2bf_inputw;       // input.w  = w_a
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = - input.w - input.v2 + floor2bf_w + floor2bf_inputw;       // input.v1 = v_a
                        v2 = - input.w - input.v1 + floor2bf_w + floor2bf_inputw;       // input.v2 = v'_a
                    }
                    break;
                case 'p':
                    return;                                    // do nothing
                case 't':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1+input.v2;                    // input.w  = w_t
                        v1 = 0.5*( input.w-input.v1+input.v2);     // input.v1 = v_t
                        v2 = 0.5*(-input.w-input.v1+input.v2);     // input.v2 = v'_t
                    }
                    else {
                        w = input.v1 + input.v2 + input.w - 2 * floor2bf_inputw;       // input.w  = w_t
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = -input.v1 + floor2bf_w + floor2bf_inputw;                 // input.v1 = v_t
                        v2 = -input.w - input.v1 + floor2bf_w + floor2bf_inputw;       // input.v2 = v'_t
                    }
                    break;
                case 'f' :
                    if (KELDYSH || ZERO_T){w  = input.w+input.v1;  // input.w  = v_1'
                        v1 = 0.5*(input.w-input.v1);               // input.v1 = v_2'
                        v2 = 0.5*(2.*input.v2-input.w-input.v1);   // input.v2 = v_1
                    }
                    else{
                            w  = input.w + input.v1;                                        // input.w  = v_1'
                            floor2bf_w = floor2bfreq(w / 2.);
                            v1 = - input.v1 + floor2bf_w;                                   // input.v1 = v_2'
                            v2 = input.v2 - w + floor2bf_w;                                 // input.v2 = v_1
                    }
                    break;
                default:;
            }
            break;
        case 't':
            switch (input.channel) {
                case 'a':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1-input.v2;                    // input.w  = w_a
                        v1 = 0.5*( input.w+input.v1+input.v2);     // input.v1 = v_a
                        v2 = 0.5*(-input.w+input.v1+input.v2);     // input.v2 = v'_a'
                    }
                    else{
                        w  = input.v1-input.v2;                                     // input.w  = w_a
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = input.w + input.v2 + floor2bf_w - floor2bf_inputw;     // input.v1 = v_a
                        v2 =           input.v2 + floor2bf_w - floor2bf_inputw;     // input.v2 = v'_a'
                    }
                    break;
                case 'p':
                    if (KELDYSH || ZERO_T){
                        w  = input.v1-input.v2;                    // input.w  = w_p
                        v1 = 0.5*(input.w-input.v1-input.v2);      // input.v1 = v_p
                        v2 = 0.5*(input.w+input.v1+input.v2);      // input.v2 = v'_p
                    }
                    else{
                        w  = input.v1-input.v2;                                     // input.w  = w_p
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = - input.v1 + floor2bf_w + floor2bf_inputw;             // input.v1 = v_p
                        v2 = input.v2 + input.w + floor2bf_w - floor2bf_inputw;     // input.v2 = v'_p
                    }
                    break;
                case 't':
                    return;                                    // do nothing
                case 'f':
                    if (KELDYSH || ZERO_T){
                        w  = input.w-input.v2;                     // input.w  = v_1'
                        v1 = 0.5*(2*input.v1+input.w-input.v2);    // input.v1 = v_2'
                        v2 = 0.5*(input.w+input.v2);               // input.v2 = v_1
                    }
                    else{
                        w  = input.w - input.v2;                                    // input.w  = v_1'
                        floor2bf_w = floor2bfreq(w / 2.);
                        v1 = input.v1 + floor2bf_w;                                 // input.v1 = v_2'
                        v2 = input.v2 + floor2bf_w;                                 // input.v2 = v_1
                    }
                    break;
                default:;
            }
            break;
        default:;
    }
    input.w  = w;
    input.v1 = v1;
    input.v2 = v2;
}


template <typename Q> void rvert<Q>::update_grid(double Lambda) {
    VertexFrequencyGrid frequencies_new = this->frequencies;  // new frequency grid
    frequencies_new.rescale_grid(Lambda);                     // rescale new frequency grid

    if (MAX_DIAG_CLASS >= 1) update_grid_K1(frequencies_new);
    if (MAX_DIAG_CLASS >= 2) update_grid_K2(frequencies_new);
    if (MAX_DIAG_CLASS >= 3) update_grid_K3(frequencies_new);

    this->frequencies = frequencies_new; // update frequency grid to new rescaled grid
}

template <typename Q> void rvert<Q>::update_grid_K1(const VertexFrequencyGrid& frequencies_new){
    vec<Q> K1_new(nK_K1 * nw1 * n_in);  // temporary K1 vector
    for (int iK1 = 0; iK1 < nK_K1; ++iK1) {
        for (int iw = 0; iw < nw1; ++iw) {
            for (int i_in = 0; i_in < n_in; ++i_in) {
                IndicesSymmetryTransformations indices(iK1, frequencies_new.b_K1.ws[iw], 0., 0., i_in, channel);
                // interpolate old values to new vector
                K1_new[iK1 * nw1 * n_in + iw * n_in + i_in] = Interpolate<k1, Q>()(indices, *this);
            }
        }
    }
    this->K1 = K1_new; // update vertex to new interpolated values
}

template <typename Q> void rvert<Q>::update_grid_K2(const VertexFrequencyGrid& frequencies_new){
    vec<Q> K2_new(nK_K2 * nw2 * nv2 * n_in);  // temporary K2 vector
    for (int iK2 = 0; iK2 < nK_K2; ++iK2) {
        for (int iw = 0; iw < nw2; ++iw) {
            for (int iv = 0; iv < nv2; ++iv) {
                for (int i_in = 0; i_in < n_in; ++i_in) {
                    IndicesSymmetryTransformations indices(iK2, frequencies_new.b_K2.ws[iw],
                                                           frequencies_new.f_K2.ws[iv],
                                                           0.,
                                                           i_in, channel);
                    // interpolate old values to new vector
                    K2_new[iK2 * nw2 * nv2 * n_in + iw * nv2 * n_in + iv * n_in + i_in]
                            = Interpolate<k2, Q>()(indices, *this);
                }
            }
        }
    }
    this->K2 = K2_new; // update vertex to new interpolated values
}

template <typename Q> void rvert<Q>::update_grid_K3(const VertexFrequencyGrid& frequencies_new){
    vec<Q> K3_new(nK_K3 * nw3 * nv3 * nv3 * n_in);  // temporary K3 vector
    for (int iK3 = 0; iK3 < nK_K3; ++iK3) {
        for (int iw = 0; iw < nw3; ++iw) {
            for (int iv = 0; iv < nv3; ++iv) {
                for (int ivp = 0; ivp < nv3; ++ivp) {
                    for (int i_in = 0; i_in < n_in; ++i_in) {
                        IndicesSymmetryTransformations indices(iK3, frequencies_new.b_K3.ws[iw],
                                                               frequencies_new.f_K3.ws[iv],
                                                               frequencies_new.f_K3.ws[ivp],
                                                               i_in, channel);
                        // interpolate old values to new vector
                        K3_new[iK3 * nw3 * nv3 * nv3 * n_in + iw * nv3 * nv3 * n_in + iv * nv3 * n_in + ivp * n_in +
                               i_in]
                                = Interpolate<k3, Q>()(indices, *this);
                    }
                }
            }
        }
    }
    this->K3 = K3_new; // update vertex to new interpolated values
}

template<typename Q> auto sign_index(Q freq) -> int {
    return (freq > 0);
}

template <typename Q> void rvert<Q>::enforce_freqsymmetriesK1(const rvert<Q>& vertex_symmrelated) {

    for (int itK = 0; itK < nK_K1; itK++) {
        int i0_tmp = 0;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a': i0_tmp = non_zero_Keldysh_K1a[itK]; break;
            case 'p': i0_tmp = non_zero_Keldysh_K1p[itK]; break;
            case 't': i0_tmp = non_zero_Keldysh_K1t[itK]; break;
            default: ;
        }
        for (int itw = 0; itw < nw1; itw++) {
            double w_in = this->frequencies.b_K1.ws[itw];
            IndicesSymmetryTransformations indices(i0_tmp, w_in, 0., 0., 0, channel);
            int sign_w = sign_index(indices.w);
            int trafo_index = freq_transformations.K1[itK][sign_w];
            if (trafo_index != 0){
                Ti(indices, trafo_index);
                indices.iK = itK;

                int sign_w_new = freq_components.K1[itK][sign_w];
                int itw_new;
                if (sign_w == sign_w_new)
                    itw_new = itw;
                else
                    itw_new = nw1 - 1 - itw;
                Q result;
                if (indices.asymmetry_transform)
                    result = indices.prefactor * vertex_symmrelated.K1[itK * nw1 + itw_new];
                else
                    result = indices.prefactor * K1[itK * nw1 + itw_new];

                if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate)
                    K1[itK * nw1 + itw] = myconj(result);
                else
                    K1[itK * nw1 + itw] = result;
            }
        }

    }
}

template<typename Q>
void rvert<Q>::K1_crossproject() {
    /// Prescription: For K1 it suffices to calculate the average over the BZ, independent of the momentum argument and of the channel.
    for (int iK = 0; iK < nK_K1; ++iK) {
#pragma omp parallel for schedule(dynamic) default(none) shared(iK)
        for (int iw = 0; iw < nw1; ++iw) {
            Q projected_value = K1_BZ_average(iK, iw);
            for (int i_in = 0; i_in < n_in; ++i_in) { // TODO: Only works if internal structure does not include form-factors!
                K1_setvert(iK, iw, i_in, projected_value); // All internal arguments get the same value for K1!
            }
        }
    }
}

template<typename Q>
Q rvert<Q>::K1_BZ_average(const int iK, const int iw) {
    /// Perform the average over the BZ by calculating the q-sum over the REDUCED BZ (see notes for details!)
    Q value = 0.;
    value += K1_val(iK, iw, momentum_index(0, 0));
    value += K1_val(iK, iw, momentum_index(glb_N_q - 1, glb_N_q - 1));
    value += 2. * K1_val(iK, iw, momentum_index(glb_N_q - 1, 0));
    for (int n = 1; n < glb_N_q - 1; ++n) {
        value += 4. * K1_val(iK, iw, momentum_index(n, 0));
        value += 4. * K1_val(iK, iw, momentum_index(glb_N_q - 1, n));
        value += 4. * K1_val(iK, iw, momentum_index(n, n));
        for (int np = 1; np < n; ++np) {
            value += 8. * K1_val(iK, iw, momentum_index(n, np));
        }
    }
    value /= 4. * (glb_N_q - 1) * (glb_N_q - 1);
    return value;
}

template <typename Q> void rvert<Q>::enforce_freqsymmetriesK2(const rvert<Q>& vertex_symmrelated) {

    for (int itK = 0; itK < nK_K2; itK++){
        int i0_tmp;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a': i0_tmp = non_zero_Keldysh_K2a[itK]; break;
            case 'p': i0_tmp = non_zero_Keldysh_K2p[itK]; break;
            case 't': i0_tmp = non_zero_Keldysh_K2t[itK]; break;
            default: ;
        }

        for (int itw = 0; itw < nw2; itw++){
            for (int itv = 0; itv < nv2; itv++){
                double w_in = this->frequencies.b_K2.ws[itw];
                double v_in = this->frequencies.f_K2.ws[itv];
                IndicesSymmetryTransformations indices(i0_tmp, w_in, v_in, 0., 0, channel);
                int sign_w = sign_index(w_in);
                int sign_v1 = sign_index(v_in);
                int trafo_index = freq_transformations.K2[itK][sign_w*2 + sign_v1];
                Ti(indices, trafo_index);
                indices.iK = itK;

                if (trafo_index != 0) {

                    Q result;
                    if (indices.asymmetry_transform)
                        result = Interpolate<k2,Q>()(indices, vertex_symmrelated);
                    else
                        result = Interpolate<k2,Q>()(indices, *(this));

                    if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate)
                        K2[itK * nw2 * nv2 + itw * nv2 + itv] = myconj(result);
                    else
                        K2[itK * nw2 * nv2 + itw * nv2 + itv] = result;
                }
            }
        }
    }

}

template<typename Q>
void rvert<Q>::K2_crossproject(char channel_out) {
// TODO: Currently does nothing!
}


template <typename Q> void rvert<Q>::enforce_freqsymmetriesK3(const rvert<Q>& vertex_symmrelated) {

    for (int itK = 0; itK < nK_K3; itK++){
        int i0_tmp;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        i0_tmp = non_zero_Keldysh_K3[itK];

        for (int itw = 0; itw < nw3; itw++){
            for (int itv = 0; itv < nv3; itv++){
                for (int itvp = 0; itvp < nv3; itvp++) {
                    double w_in = this->frequencies.b_K3.ws[itw];
                    double v_in = this->frequencies.f_K3.ws[itv];
                    double vp_in = this->frequencies.f_K3.ws[itvp];
                    IndicesSymmetryTransformations indices(i0_tmp, w_in, v_in, vp_in, 0, channel);
                    int sign_w = sign_index(w_in);
                    int sign_f =  itv+itvp<nFER3? 0 : 1;    // this corresponds to "sign_index(v_in + vp_in)" assuming
                                                            // that both v and vp use the same fermionic frequency grid
                    int sign_fp = itv<=itvp? 0 : 1;         // this corresponds to "sign_index(v_in - vp_in)"  assuming
                                                            // that both v and vp use the same fermionic frequency grid
                    int trafo_index = freq_transformations.K3[itK][sign_w * 4 + sign_f * 2 + sign_fp];
                    Ti(indices, trafo_index);
                    indices.iK = itK;

                    if (trafo_index != 0) {

                        Q result;
                        if (indices.asymmetry_transform)
                            result = Interpolate<k3,Q>()(indices, vertex_symmrelated);
                        else
                            result = Interpolate<k3,Q>()(indices, *(this));

                        if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate)
                            K3[((itK * nw3 + itw) * nv3 + itv) * nv3 + itvp] = myconj(result);
                        else
                            K3[((itK * nw3 + itw) * nv3 + itv) * nv3 + itvp] = result;
                    }
                }
            }
        }
    }

}


template<typename Q>
void rvert<Q>::K3_crossproject(char channel_out) {
    // TODO: Currently does nothing!
}



template <typename Q> auto rvert<Q>::K1_acc(int i) const -> Q {
    if (i >= 0 && i < K1.size())
        return K1[i];
    else
        print("Error: Tried to access value outside of K1 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K1_direct_set(int i, Q value) {
    if (i >= 0 && i < K1.size())
        K1[i] = value;
    else
        print("Error: Tried to access value outside of K1 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K1_setvert(int iK, int iw, int i_in, Q value) {
    K1[iK*nw1*n_in + iw*n_in + i_in] = value;
}
template <typename Q> void rvert<Q>::K1_addvert(int iK, int iw, int i_in, Q value) {
    K1[iK*nw1*n_in + iw*n_in + i_in] += value;
}
template <typename Q> auto rvert<Q>::K1_val(int iK, int iw, int i_in) const -> Q {
        return K1[iK*nw1*n_in + iw*n_in + i_in];
}


template <typename Q> auto rvert<Q>::K2_acc(int i) const -> Q {
    if (i >= 0 && i < K2.size())
        return K2[i];
    else
        print("Error: Tried to access value outside of K2 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K2_direct_set(int i, Q value) {
    if (i >= 0 && i < K2.size())
        K2[i] = value;
    else
        print("Error: Tried to access value outside of K2 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K2_setvert(int iK, int iw, int iv, int i_in, Q value) {
    K2[iK*nw2*nv2*n_in + iw*nv2*n_in + iv*n_in + i_in] = value;
}
template <typename Q> void rvert<Q>::K2_addvert(int iK, int iw, int iv, int i_in, Q value) {
    K2[iK*nw2*nv2*n_in + iw*nv2*n_in + iv*n_in + i_in] += value;
}
template <typename Q> auto rvert<Q>::K2_val(int iK, int iw, int iv, int i_in) const -> Q {
        return K2[iK * nw2 * nv2 * n_in + iw * nv2 * n_in + iv * n_in + i_in];
}


template <typename Q> auto rvert<Q>::K3_acc(int i) const -> Q {
    if (i >= 0 && i < K3.size())
        return K3[i];
    else
        print("Error: Tried to access value outside of K3 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K3_direct_set(int i, Q value) {
    if (i >= 0 && i < K3.size())
        K3[i] = value;
    else
        print("Error: Tried to access value outside of K3 vertex in a-channel", true);
}
template <typename Q> void rvert<Q>::K3_setvert(int iK, int iw, int iv, int ivp, int i_in, Q value) {
    K3[iK*nw3*nv3*nv3*n_in + iw*nv3*nv3*n_in + iv*nv3*n_in + ivp*n_in + i_in] = value;
}
template <typename Q> void rvert<Q>::K3_addvert(int iK, int iw, int iv, int ivp, int i_in, Q value) {
    K3[iK*nw3*nv3*nv3*n_in + iw*nv3*nv3*n_in + iv*nv3*n_in + ivp*n_in + i_in] += value;
}
template <typename Q> auto rvert<Q>::K3_val(int iK, int iw, int iv, int ivp, int i_in) const -> Q {
        return K3[iK*nw3*nv3*nv3*n_in + iw*nv3*nv3*n_in + iv*nv3*n_in + ivp*n_in + i_in];
}

template <typename Q> auto rvert<Q>::get_deriv_maxK1() const -> double {
    double max_K1 = ::power2(::get_finite_differences(K1)).max_norm();
    return max_K1;

}
template <typename Q> auto rvert<Q>::get_deriv_maxK2() const -> double {
    double max_K2 = (::power2(::get_finite_differences<Q,2>(K2, {nBOS2, nFER2}, {0, 1}))
                   + ::power2(::get_finite_differences<Q,2>(K2, {nFER2}, {1, 0}))
    ).max_norm();
    return max_K2;

}

template <typename Q> auto rvert<Q>::get_deriv_maxK3() const -> double {
    double max_K3 = (::power2(::get_finite_differences<Q,3>(K3, {nBOS3, nFER3, nFER3}, {0, 1, 2}))
                   + ::power2(::get_finite_differences<Q,3>(K3, {nFER3, nBOS3, nFER3}, {1, 2, 0}))
                   + ::power2(::get_finite_differences<Q,3>(K3, {nFER3, nFER3, nBOS3}, {2, 0, 1}))
    ).max_norm();
    return max_K3;

}


#endif //KELDYSH_MFRG_R_VERTEX_H