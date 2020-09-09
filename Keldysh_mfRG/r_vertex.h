/**
 * Reducible vertex in channel r, split into diagrammatic classes K1, K2, K2b, K3.
 * Member functions allow for access of values at arbitrary frequencies and in each channel-specific
 * frequency representation.
 */

#ifndef KELDYSH_MFRG_R_VERTEX_H
#define KELDYSH_MFRG_R_VERTEX_H

#include "data_structures.h"          // real/complex vector classes
#include "parameters.h"               // system parameters (lengths of vectors etc.)
#include "Keldysh_symmetries.h"       // transformations on Keldysh indices
#include "internal_symmetries.h"      // symmetry transformations for internal indices (momentum etc.), currently trivial
#include "interpolations.h"           // frequency interpolations for vertices
#include "symmetry_transformations.h" // symmetry transformations of frequencies
#include "symmetry_table.h"           // table containing information when to apply which symmetry transformations

template <typename Q>
class rvert{
public:
    char channel;                       // reducibility channel
    Components components;              // lists providing information on how all Keldysh components are related to the
                                        // independent ones
    Transformations transformations;    // lists providing information on which transformations to apply on Keldysh
                                        // components to relate them to the independent ones

    rvert(const char channel_in) : channel(channel_in) {
        components = Components(channel);
        transformations = Transformations(channel);
    };

    /// Member functions for accessing the reducible vertex in channel r at arbitrary frequencies ///
    /// by interpolating stored data, in all possible channel-dependent frequency representations ///

    /**
     * Return the value of the reducible vertex in channel r (used for r = p).
     * @param input : Combination of input arguments.
     */
    auto value (VertexInput input) const -> Q;
    /**
     * Return the value of the reducible vertex in channel r (used for r = (a,t)).
     * @param input     : Combination of input arguments.
     * @param vertex_in : Reducible vertex in the related channel (t,a), needed to apply symmetry transformations that
     *                    map between channels a <--> t.
     */
    auto value (VertexInput input, const rvert<Q>& vertex_in) const -> Q;

    /**
     * Transform the frequencies from the frequency convention of input.channel to the frequency convention of
     * this->channel. Necessary when accessing the r vertex from a different channel r'.
     */
    void transfToR(VertexInput& input) const;

#ifdef DIAG_CLASS
#if DIAG_CLASS >= 0
    vec<Q> K1 = vec<Q> (nK_K1 * nw1_a * n_in);  // data points of K1

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

    /// Member functions for accessing the vertex K1 at arbitrary frequencies by interpolating stored data ///

    /**
     * Return the value of the vertex K1 in channel r (used for r = p).
     * @param input : Combination of input arguments.
     */
    auto K1_valsmooth(VertexInput input) const -> Q;
    /**
     * Return the value of the vertex K1 in channel r (used for r = (a,t)).
     * @param input     : Combination of input arguments.
     * @param vertex_in : Reducible vertex in the related channel (t,a), needed to apply symmetry transformations that
     *                    map between channels a <--> t.
     */
    auto K1_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q;


#endif
#if DIAG_CLASS >= 2
    vec<Q> K2 = vec<Q> (nK_K2 * nw2 * nv2 * n_in);  // data points of K2

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

    /// Member functions for accessing the vertex K2 at arbitrary frequencies by interpolating stored data ///

    /**
     * Return the value of the vertex K2 in channel r (used for r = p).
     * @param input : Combination of input arguments.
     */
    auto K2_valsmooth(VertexInput input) const-> Q;
    /**
     * Return the value of the vertex K2 in channel r (used for r = (a,t)).
     * @param input     : Combination of input arguments.
     * @param vertex_in : Reducible vertex in the related channel (t,a), needed to apply symmetry transformations that
     *                    map between channels a <--> t.
     */
    auto K2_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const-> Q;
    /**
     * Return the value of the vertex K2b in channel r (used for r = p).
     * @param input : Combination of input arguments.
     */
    auto K2b_valsmooth(VertexInput input) const -> Q;
    /**
     * Return the value of the vertex K2b in channel r (used for r = (a,t)).
     * @param input     : Combination of input arguments.
     * @param vertex_in : Reducible vertex in the related channel (t,a), needed to apply symmetry transformations that
     *                    map between channels a <--> t.
     */
    auto K2b_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q;

#endif
#if DIAG_CLASS >= 3
    vec<Q> K3 = vec<Q> (nK_K3 * nw3 * nv3 * nv3 * n_in);  // data points of K3

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

    /// Member functions for accessing the vertex K3 at arbitrary frequencies by interpolating stored data ///

    /**
     * Return the value of the vertex K3 in channel r (used for r = p).
     * @param input : Combination of input arguments.
     */
    auto K3_valsmooth(VertexInput) const -> Q;
    /**
     * Return the value of the vertex K3 in channel r (used for r = (a,t)).
     * @param input     : Combination of input arguments.
     * @param vertex_in : Reducible vertex in the related channel (t,a), needed to apply symmetry transformations that
     *                    map between channels a <--> t.
     */
    auto K3_valsmooth(VertexInput, const rvert<Q>&) const -> Q;

#endif
#endif

    auto operator+= (const rvert<Q>& rhs) -> rvert<Q>
    {
#if DIAG_CLASS >= 0
        this->K1 += rhs.K1;
#endif
#if DIAG_CLASS >= 2
        this->K2 += rhs.K2;
#endif
#if DIAG_CLASS >= 3
        this->K3 += rhs.K3;
#endif
        return *this;
    }
    friend rvert<Q> operator+ (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs += rhs;
        return lhs;
    }
    auto operator*= (double alpha) -> rvert<Q>
    {
#if DIAG_CLASS >= 0
        this->K1 *= alpha;
#endif
#if DIAG_CLASS >= 2
        this->K2 *= alpha;
#endif
#if DIAG_CLASS >= 3
        this->K3 *= alpha;
#endif
        return *this;
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const double& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator*= (const rvert<Q>& rhs) -> rvert<Q>
    {
#if DIAG_CLASS >= 0
        this->K1 *= rhs.K1;
#endif
#if DIAG_CLASS >= 2
        this->K2 *= rhs.K2;
#endif
#if DIAG_CLASS >= 3
        this->K3 *= rhs.K3;
#endif
        return *this;
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator-= (const rvert<Q>& rhs) -> rvert<Q>
    {
#if DIAG_CLASS >= 0
        this->K1 -= rhs.K1;
#endif
#if DIAG_CLASS >= 2
        this->K2 -= rhs.K2;
#endif
#if DIAG_CLASS >= 3
        this->K3 -= rhs.K3;
#endif
        return *this;
    }
    friend rvert<Q> operator- (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/****************************************** MEMBER FUNCTIONS OF THE R-VERTEX ******************************************/
template <typename Q> auto rvert<Q>::value(VertexInput input) const -> Q {

    transfToR(input);

    Q k1, k2, k2b, k3;

#if DIAG_CLASS>=0
    k1 = K1_valsmooth(input);
#endif
#if DIAG_CLASS >=2
    k2  = K2_valsmooth(input);
    k2b = K2b_valsmooth(input);
#endif
#if DIAG_CLASS >=3
    k3 = K3_valsmooth(input);
#endif

    return k1+k2+k2b+k3;
}
template <typename Q> auto rvert<Q>::value(VertexInput input, const rvert<Q>& vertex_in) const -> Q {

    transfToR(input);

    Q k1, k2, k2b, k3;

#if DIAG_CLASS>=0
    k1 = K1_valsmooth (input, vertex_in);
#endif
#if DIAG_CLASS >=2
    k2 = K2_valsmooth (input, vertex_in);
    k2b= K2b_valsmooth(input, vertex_in);
#endif
#if DIAG_CLASS >=3
    k3 = K3_valsmooth (input, vertex_in);
#endif

    return k1+k2+k2b+k3;
}

template <typename Q> void rvert<Q>::transfToR(VertexInput& input) const {
    double w, v1, v2;
    switch (channel) {
        case 'a':
            switch (input.channel) {
                case 'a':
                    return;                                    // do nothing
                case 'p':
                    w  = -input.v1-input.v2;                   // input.w  = w_p
                    v1 = 0.5*(input.w+input.v1-input.v2);      // input.v1 = v_p
                    v2 = 0.5*(input.w-input.v1+input.v2);      // input.v2 = v'_p
                    break;
                case 't':
                    w  = input.v1-input.v2;                    // input.w  = w_t
                    v1 = 0.5*( input.w+input.v1+input.v2);     // input.v1 = v_t
                    v2 = 0.5*(-input.w+input.v1+input.v2);     // input.v2 = v'_t
                    break;
                case 'f':
                    w  = input.v1-input.v2;                    // input.w  = v_1'
                    v1 = 0.5*(2.*input.w+input.v1-input.v2);   // input.v1 = v_2'
                    v2 = 0.5*(input.v1+input.v2);              // input.v2 = v_1
                    break;
                default:;
            }
            break;
        case 'p':
            switch (input.channel) {
                case 'a':
                    w  = input.v1+input.v2;                    // input.w  = w_a
                    v1 = 0.5*(-input.w+input.v1-input.v2);     // input.v1 = v_a
                    v2 = 0.5*(-input.w-input.v1+input.v2);     // input.v2 = v'_a
                    break;
                case 'p':
                    return;                                    // do nothing
                case 't':
                    w  = input.v1+input.v2;                    // input.w  = w_t
                    v1 = 0.5*( input.w-input.v1+input.v2);     // input.v1 = v_t
                    v2 = 0.5*(-input.w-input.v1+input.v2);     // input.v2 = v'_t
                    break;
                case 'f' :
                    w  = input.w+input.v1;                     // input.w  = v_1'
                    v1 = 0.5*(input.w-input.v1);               // input.v1 = v_2'
                    v2 = 0.5*(2.*input.v2-input.w-input.v1);   // input.v2 = v_1
                    break;
                default:;
            }
            break;
        case 't':
            switch (input.channel) {
                case 'a':
                    w  = input.v1-input.v2;                    // input.w  = w_a
                    v1 = 0.5*( input.w+input.v1+input.v2);     // input.v1 = v_a
                    v2 = 0.5*(-input.w+input.v1+input.v2);     // input.v2 = v'_a'
                    break;
                case 'p':
                    w  = input.v1-input.v2;                    // input.w  = w_p
                    v1 = 0.5*(input.w-input.v1-input.v2);      // input.v1 = v_p
                    v2 = 0.5*(input.w+input.v1+input.v2);      // input.v2 = v'_p
                    break;
                case 't':
                    return;                                    // do nothing
                case 'f':
                    w  = input.w-input.v2;                     // input.w  = v_1'
                    v1 = 0.5*(2*input.v1+input.w-input.v2);    // input.v1 = v_2'
                    v2 = 0.5*(input.w+input.v2);               // input.v2 = v_1
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

#if DIAG_CLASS >= 0
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

template <typename Q> auto rvert<Q>::K1_valsmooth(VertexInput input) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, 0., 0., input.i_in, channel);

    Ti(indices, transformations.K1[input.spin][input.iK]);
    indices.iK = components.K1[input.iK];
    if (indices.iK < 0) return 0.;
    if (indices.conjugate) return conj(interpolateK1(indices, *(this))); // conjugation only in t-channel --> no flip necessary
    return interpolateK1(indices, *(this));
}
template <typename Q> auto rvert<Q>::K1_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, 0., 0., input.i_in, channel);

    Ti(indices, transformations.K1[input.spin][input.iK]);
    indices.iK = components.K1[input.iK];
    if (indices.iK < 0) return 0.;
    if (indices.conjugate) return conj(interpolateK1(indices, *(this))); // conjugation only in t-channel --> no flip necessary
    if (indices.channel != channel) return interpolateK1(indices, vertex_in);
    return interpolateK1(indices, *(this));
}
#endif

#if DIAG_CLASS >= 2
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
    return K2[iK*nw2*nv2*n_in + iw*nv2*n_in + iv*n_in + i_in];
}

template <typename Q> auto rvert<Q>::K2_valsmooth(VertexInput input) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, input.v1, 0., input.i_in, channel);

    Ti(indices, transformations.K2[input.spin][input.iK]);
    indices.iK = components.K2[input.iK];
    if (indices.iK < 0) return 0.;
    if (indices.conjugate) return conj(interpolateK2(indices, *(this)));
    return interpolateK2(indices, *(this));
}
template <typename Q> auto rvert<Q>::K2_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, input.v1, 0., input.i_in, channel);

    Ti(indices, transformations.K2[input.spin][input.iK]);
    indices.iK = components.K2[input.iK];
    if (indices.iK < 0) return 0.;

    Q valueK2;

    if (indices.channel != channel)  // Applied trafo changes channel a -> t
        valueK2 = interpolateK2(indices, vertex_in);
    else
        valueK2 = interpolateK2(indices, *(this));

    if (indices.conjugate) return conj(valueK2);
    return valueK2;
}
template <typename Q> auto rvert<Q>::K2b_valsmooth(VertexInput input) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, 0., input.v2, input.i_in, channel);

    Ti(indices, transformations.K2b[input.spin][input.iK]);
    indices.iK = components.K2b[input.iK];
    if (indices.iK < 0) return 0.;
    if (indices.conjugate) return conj(interpolateK2(indices, *(this)));
    return interpolateK2(indices, *(this));
}
template <typename Q> auto rvert<Q>::K2b_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, 0., input.v2, input.i_in, channel);

    Ti(indices, transformations.K2b[input.spin][input.iK]);
    indices.iK = components.K2b[input.iK];
    if (indices.iK < 0) return 0.;

    Q valueK2;

    if (indices.channel != channel)  //Applied trafo changes channel a -> t
        valueK2 = interpolateK2(indices, vertex_in);
    else
        valueK2 = interpolateK2(indices, *(this));

    if (indices.conjugate) return conj(valueK2);
    return valueK2;
}
#endif

#if DIAG_CLASS >= 3
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

template <typename Q> auto rvert<Q>::K3_valsmooth(VertexInput input) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, input.v1, input.v2, input.i_in, channel);

    Ti(indices, transformations.K3[input.spin][input.iK]);
    indices.iK = components.K3[input.iK];
    if (indices.iK < 0) return 0.;
    if (indices.conjugate) return conj(interpolateK3(indices, *(this)));
    return interpolateK3(indices, *(this));
}
template <typename Q> auto rvert<Q>::K3_valsmooth(VertexInput input, const rvert<Q>& vertex_in) const -> Q {

    IndicesSymmetryTransformations indices(input.iK, input.w, input.v1, input.v2, input.i_in, channel);

    Ti(indices, transformations.K3[input.spin][input.iK]);
    indices.iK = components.K3[input.iK];
    if (indices.iK < 0) return 0.;

    Q valueK3;

    if (indices.channel != channel)  //Applied trafo changes channel a -> t
        valueK3 = interpolateK3(indices, vertex_in);
    else
        valueK3 = interpolateK3(indices, *(this));

    if (indices.conjugate) return conj(valueK3);
    return valueK3;
}
#endif

#endif //KELDYSH_MFRG_R_VERTEX_H