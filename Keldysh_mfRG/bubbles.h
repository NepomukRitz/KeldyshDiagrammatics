/**
 * Classes / functions for computing bubbles.
 *
 * class Bubble            : Object containing two propagators, with call operator
 * class Integrand_Ki      : Object with call operator providing the integrand for bubble frequency integration
 *                           in diagrammatic class Ki:
 *                           Computing Vertex * Bubble * Vertex, and performing the internal Keldysh summation
 * class Integrand_Ki_diff : Same as Integrand_Ki, for differentiated bubble
 * bubble_function()       : Computing the bubble frequency integral, using the integrand classes and the integrator
 *                           from "integrator.h". Using MPI+OMP parallelization in the external arguments.
 */

#ifndef KELDYSH_MFRG_BUBBLES_H
#define KELDYSH_MFRG_BUBBLES_H

#include <cmath>                        // for using the macro M_PI as pi

#include "vertex.h"                     // vertex class
#include "selfenergy.h"                 // self-energy class
#include "propagator.h"                 // propagator class
#include "integrator.h"                 // integration routines
#include "util.h"                       // measuring time, printing text output
#include "mpi_setup.h"                  // mpi parallelization routines
#include "correctionFunctions.h"        // correction terms due to finite integration range
#include "write_data2file.h"            // write vectors into hdf5 file

/// Class combining two propagators, either GG or GS+SG
class Bubble{
    const Propagator& g;
    const Propagator& s;
    const bool dot;
public:

    /**
     * Constructor:
     * @param propagatorG : first propagator (always a standard one)
     * @param propagatorS : second propagator (standard or single-scale/differentiated, depending on "dot_in")
     * @param dot_in      : whether to compute standard (false) or differentiated (true) bubble
     */
    Bubble(const Propagator& propagatorG, const Propagator& propagatorS, const bool dot_in)
        :g(propagatorG), s(propagatorS), dot(dot_in) {};

    /**
     * Call operator:
     * @param iK    : Keldysh index of combined bubble object (0 <= iK <= 15)
     * @param v1    : frequency of first propagator
     * @param v2    : frequency of second propagator
     * @param i_in  : internal structure index
     * @return comp : value of the bubble evaluated at (iK, v1, v2)
     */
    auto value(int iK, double v1, double v2, int i_in) const -> comp{
        comp ans;
        if(dot){
#ifdef KELDYSH_FORMALISM
            switch (iK) {
                case 3: //AA
                    ans = conj(g.valsmooth(0, v1, i_in)) * conj(s.valsmooth(0, v2, i_in)) + conj(s.valsmooth(0, v1, i_in)) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 6: //AR
                    ans = conj(g.valsmooth(0, v1, i_in)) * s.valsmooth(0, v2, i_in) + conj(s.valsmooth(0, v1, i_in)) * g.valsmooth(0, v2, i_in);
                    break;
                case 7: //AK
                    ans = conj(g.valsmooth(0, v1, i_in)) * s.valsmooth(1, v2, i_in) + conj(s.valsmooth(0, v1, i_in)) * g.valsmooth(1, v2, i_in);
                    break;
                case 9: //RA
                    ans = g.valsmooth(0, v1, i_in) * conj(s.valsmooth(0, v2, i_in)) + s.valsmooth(0, v1, i_in) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 11://KA
                    ans = g.valsmooth(1, v1, i_in) * conj(s.valsmooth(0, v2, i_in)) + s.valsmooth(1, v1, i_in) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 12://RR
                    ans = g.valsmooth(0, v1, i_in) * s.valsmooth(0, v2, i_in) + s.valsmooth(0, v1, i_in) * g.valsmooth(0, v2, i_in);
                    break;
                case 13://RK
                    ans = g.valsmooth(0, v1, i_in) * s.valsmooth(1, v2, i_in) + s.valsmooth(0, v1, i_in) * g.valsmooth(1, v2, i_in);
                    break;
                case 14://KR
                    ans = g.valsmooth(1, v1, i_in) * s.valsmooth(0, v2, i_in) + s.valsmooth(1, v1, i_in) *  g.valsmooth(0, v2, i_in);
                    break;
                case 15://KK
                    ans = g.valsmooth(1, v1, i_in) * s.valsmooth(1, v2, i_in) + s.valsmooth(1, v1, i_in) * g.valsmooth(1, v2, i_in);
                    break;
                default:
                    return 0.;
            }
#else
            ans = g.valsmooth(0, v1, i_in) * s.valsmooth(0, v2, i_in) + s.valsmooth(0, v1, i_in) * g.valsmooth(0, v2, i_in);
#endif
        }
        else {
#ifdef KELDYSH_FORMALISM
            switch (iK){ // labelling propagators from top (t: left) to bottom (t: right); a,t: G(v+w/2)G(v-w/2), p: G(w/2-v)G(w/2+v)
                case 3: //AA
                    ans = conj(g.valsmooth(0, v1, i_in)) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 6: //AR
                    ans = conj(g.valsmooth(0, v1, i_in)) * g.valsmooth(0, v2, i_in);
                    break;
                case 7: //AK
                    ans = conj(g.valsmooth(0, v1, i_in)) * g.valsmooth(1, v2, i_in);
                    break;
                case 9: //RA
                    ans = g.valsmooth(0, v1, i_in) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 11://KA
                    ans = g.valsmooth(1, v1, i_in) * conj(g.valsmooth(0, v2, i_in));
                    break;
                case 12://RR
                    ans = g.valsmooth(0, v1, i_in) * g.valsmooth(0, v2, i_in);
                    break;
                case 13://RK
                    ans = g.valsmooth(0, v1, i_in) * g.valsmooth(1, v2, i_in);
                    break;
                case 14://KR
                    ans =  g.valsmooth(1, v1, i_in) *  g.valsmooth(0, v2, i_in);
                    break;
                case 15://KK
                    ans =  g.valsmooth(1, v1, i_in) *  g.valsmooth(1, v2, i_in);
                    break;
                default:
                    return 0.;
            }
#else
            ans = g.valsmooth(0, v1, i_in) * g.valsmooth(0, v2, i_in);
#endif
        }
        return ans;
    }

    /**
     * Wrapper for value function above, providing the natural arguments for evaluation of the bubble in each channel:
     * @param iK      : Keldysh index of combined bubble object (0 <= iK <= 15)
     * @param w       : bubble transfer frequency of the corresponding channel
     * @param vpp     : bubble integration frequency of the corresponding channel
     * @param i_in    : internal structure index
     * @param channel : channel to which the bubble belongs
     * @return comp   : value of the bubble evaluated at the arguments described above
     */
    auto value(int iK, double w, double vpp, int i_in, char channel) const -> comp {
        comp Pival;
        switch (channel) {
            case 'a':
                Pival = value(iK, vpp - w / 2., vpp + w / 2., i_in);    //vppa-1/2wa, vppa+1/2wa for the a-channel
                break;
            case 'p':
                Pival = value(iK, w / 2. + vpp, w / 2. - vpp, i_in);    //wp/2+vppp, wp/2-vppp for the p-channel
                break;
            case 't':
                Pival = value(iK, vpp - w / 2., vpp + w / 2., i_in);    //vppt-1/2wt, vppt+1/2wt for the t-channel
                break;
            default:;
        }
        return Pival;
    }
};

template <typename Q>
class PrecalculateBubble{
    const Propagator& g;
    const Propagator& s;
    const bool dot;
    char channel;

    FrequencyGrid bosonic_grid;
    FrequencyGrid fermionic_grid;

    Bubble Helper_Bubble;

#ifdef KELDYSH_FORMALISM
    int number_of_Keldysh_components = 9;
#else
    int number_of_Keldysh_components = 1;
#endif

    vec<Q> FermionicBubble = vec<Q> (number_of_Keldysh_components*nBOS*nFER*n_in); // 9 non-zero Keldysh components
public:
    PrecalculateBubble(const Propagator& G_in, const Propagator& S_in,
                       const bool dot_in, const char channel_in)
                       :g(G_in), s(S_in), dot(dot_in), channel(channel_in),
                       Helper_Bubble(g, s, dot){
        bosonic_grid = FrequencyGrid('b', 1, g.Lambda);
        fermionic_grid = FrequencyGrid('f', 1, g.Lambda);

        compute_FermionicBubble();
    }

    // TODO: Use "Interpolate" from "interpolations.h" for this.
    Q value(const int iK, const double w, const double vpp, const int i_in){
        if (    fabs(w) + inter_tol < bosonic_grid.w_upper
                && fabs(vpp) + inter_tol < fermionic_grid.w_upper) {

            int index_b = bosonic_grid.fconv(w);
            int index_f = fermionic_grid.fconv(vpp);

            double x1 = bosonic_grid.w[index_b];
            double x2 = bosonic_grid.w[index_b + 1];
            if (!(x1 < x2)) { // If not x1<x2, we run out of the box --> shift the index downwards.
                index_b -= 1;
                x1 = bosonic_grid.w[index_b];
                x2 = bosonic_grid.w[index_b + 1];
            }
            double y1 = fermionic_grid.w[index_f];
            double y2 = fermionic_grid.w[index_f + 1];
            if (!(y1 < y2)) { // If not y1<y2, we run out of the box --> shift the index downwards.
                index_f -= 1;
                y1 = fermionic_grid.w[index_f];
                y2 = fermionic_grid.w[index_f + 1];
            }

            double xd = (w - x1) / (x2 - x1);
            double yd = (vpp - y1) / (y2 - y1);
            auto f11 = FermionicBubble[composite_index(iK, index_b, index_f, i_in)];
            auto f12 = FermionicBubble[composite_index(iK, index_b, index_f + 1, i_in)];
            auto f21 = FermionicBubble[composite_index(iK, index_b + 1, index_f, i_in)];
            auto f22 = FermionicBubble[composite_index(iK, index_b + 1, index_f + 1, i_in)];

            return (1. - yd) * ((1. - xd) * f11 + xd * f21) + yd * ((1. - xd) * f12 + xd * f22);
        }
        else {
            return 0.;
        }
    };

    void compute_FermionicBubble(){
        for (int iK = 0; iK < number_of_Keldysh_components; ++iK) {
            for (int iw = 0; iw < nBOS; ++iw) {
                for (int ivpp = 0; ivpp < nFER; ++ivpp) {
                    perform_internal_sum(iK, iw, ivpp);
                }
            }
        }
    };

    void perform_internal_sum(const int iK, const int iw, const int ivpp){
        double w = bosonic_grid.w[iw];
        double vpp = fermionic_grid.w[ivpp];
        for (int i_in = 0; i_in < n_in; ++i_in) {
            FermionicBubble[composite_index(iK, iw, ivpp, i_in)] =
                    Helper_Bubble.value(iK, w, vpp, i_in, channel);
        }
    };

    int composite_index(const int iK, const int iw, const int ivpp, const int i_in){
        return iK*nBOS*nFER*n_in + iw*nFER*n_in + ivpp*n_in + i_in;
    };
};

//Class created for debugging of the Bubbles
class IntegrandBubble{
    const Propagator& g1;
    const Propagator& g2;
    bool diff;
    double w;
    int iK;
    char channel;

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
    IntegrandBubble(const Propagator& g1_in, const Propagator& g2_in, bool diff_in, double w_in, int iK_in, char channel_in)
            : g1(g1_in), g2(g2_in), diff(diff_in), w(w_in), iK(iK_in), channel(channel_in) {};

    /**
     * Call operator
     * @param vpp : v'', the frequency over which is being integrated
     * @return The value g1(v1)*g2(v2), where v1 and v2 are calculated according to the channel. The components of the
     * propagators taken depend on the Keldysh component
     */
    auto operator() (double vpp) const -> comp {
        comp ans;
        double v1, v2;
        Bubble Pi(g1, g2, diff);

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
                cout << "Error in IntegrandBubble";
        }
        //Make reference to the Bubble object of the actual code, making this into a useful test of code correctnes and compliance
        return Pi.value(iK, v1, v2, 0)/(2.*M_PI*glb_i);
    }
};

/// Integrand classes for bubble contributing to diagrammatic class K1, K2, K3
template <typename Q,
          template <typename> class symmetry_left,
          template <typename> class symmetry_right>
class Integrand_K1 {
    const GeneralVertex<Q, symmetry_left>& vertex1;
    const GeneralVertex<Q, symmetry_right>& vertex2;
    const Bubble& Pi;
    int i0;
    int i2;
#ifdef DEBUG_MODE
    int iK_select;
    int iK_select_bubble;
#endif
    const int i_in;
    const char channel;
    const double w;
    const bool diff;
#if DIAG_CLASS <= 1
    Q res_l_V, res_r_V, res_l_Vhat, res_r_Vhat;
#endif
public:
    /**
     * Constructor:
     * @param vertex1_in : left vertex
     * @param vertex2_in : right vertex
     * @param Pi_in      : Bubble object connecting the left and right vertex
     * @param i0_in      : index (0 or 1) specifying the (external) Keldysh component of integrand object
     *                     (converted into actual Keldysh index i0 within the constructor)
     * @param w_in       : external bosonic frequency \omega
     * @param i_in_in    : external index for internal structure
     * @param ch_in      : diagrammatic channel ('a', 'p', 't')
     * @param diff_in    : determines whether to compute differentiated or non-differentiated bubble
     */
    Integrand_K1(const GeneralVertex<Q, symmetry_left>& vertex1_in,
                 const GeneralVertex<Q, symmetry_right>& vertex2_in,
                 const Bubble& Pi_in,
                 int i0_in, int i2_in, const double w_in, const int i_in_in, const char ch_in, const bool diff_in
#ifdef DEBUG_MODE
                 , const int iK_select_in, const int iK_select_bubble_in
#endif
                 )
               : vertex1(vertex1_in),         vertex2(vertex2_in),           Pi(Pi_in),
                 i2(i2_in), w(w_in), i_in(i_in_in), channel(ch_in), diff(diff_in)
#ifdef DEBUG_MODE
                 , iK_select(iK_select_in), iK_select_bubble(iK_select_bubble_in)
#endif
    {
#ifdef KELDYSH_FORMALISM
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a': i0 = non_zero_Keldysh_K1a[i0_in]; break;
            case 'p': i0 = non_zero_Keldysh_K1p[i0_in]; break;
            case 't': i0 = non_zero_Keldysh_K1t[i0_in]; break;
            default: ;
        }
#else
        i0 = 0;
#endif

#if DIAG_CLASS <= 1
#ifdef KELDYSH_FORMALISM
        // For K1 class, left and right vertices do not depend on integration frequency -> precompute them to save time
        vector<int> indices = indices_sum(i0, i2, channel);

        VertexInput input_l (indices[0], w, 0., 0., i_in, 0, channel);
        VertexInput input_r (indices[1], w, 0., 0., i_in, 0, channel);
#else
        VertexInput input_l (0, w, 0., 0., i_in, 0, channel);
        VertexInput &input_r = input_l;
#endif
        res_l_V = vertex1[0].left_same_bare(input_l);
        res_r_V = vertex2[0].right_same_bare(input_r);
        if (channel == 't') {
            input_l.spin = 1;
            input_r.spin = 1;
            res_l_Vhat = vertex1[0].left_same_bare(input_l);
            res_r_Vhat = vertex2[0].right_same_bare(input_r);
        }
#endif
    };

    /**
     * Call operator:
     * @param vpp : frequency at which to evaluate integrand (to be integrated over)
     * @return Q  : value of the integrand object evaluated at frequency vpp (comp or double)
     */
    auto operator() (double vpp) const -> Q {
        Q res;
#if DIAG_CLASS >= 2
        Q res_l_V, res_r_V, res_l_Vhat, res_r_Vhat;
        vector<int> indices = indices_sum(i0, i2, channel);
#endif
#if DIAG_CLASS <= 1 && defined(KELDYSH_FORMALISM)
        if (!diff) {
            // directly return zero in cases that always have to be zero
            switch (channel) {
                case 'a':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 11 && i2 != 13)) return 0.;
                    if (i0 == 3 &&
                        (i2 != 6 && i2 != 7 && i2 != 9 && i2 != 11 && i2 != 13 && i2 != 14 && i2 != 15))
                        return 0.;
                    break;
                case 'p':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 7 && i2 != 11)) return 0.;
                    if (i0 == 5 &&
                        (i2 != 3 && i2 != 7 && i2 != 11 && i2 != 12 && i2 != 13 && i2 != 14 && i2 != 15))
                        return 0.;
                    break;
                case 't':
                    // only nonzero combinations of \int dvpp Gamma_0 Pi(vpp) Gamma_0
                    if (i0 == 1 && (i2 != 11 && i2 != 13)) return 0.;
                    if (i0 == 3 &&
                        (i2 != 6 && i2 != 7 && i2 != 9 && i2 != 11 && i2 != 13 && i2 != 14 && i2 != 15))
                        return 0.;
                    break;
                default:;
            }
        }
#endif
#ifdef DEBUG_MODE
        if (channel == 'a') {
            if (indices[1] != iK_select && iK_select < 16) return 0.;
            if (i2 != iK_select_bubble && iK_select_bubble < 16) return 0.;
        }
#endif
        Q Pival = Pi.value(i2, w, vpp, i_in, channel);

#if DIAG_CLASS >= 2
        VertexInput input_l (indices[0], w, 0., vpp, i_in, 0, channel);
        VertexInput input_r (indices[1], w, vpp, 0., i_in, 0, channel);

        res_l_V = vertex1[0].left_same_bare(input_l);
        res_r_V = vertex2[0].right_same_bare(input_r);

        if (channel == 't') {
            input_l.spin = 1;
            input_r.spin = 1;
            res_l_Vhat = vertex1[0].left_same_bare(input_l);
            res_r_Vhat = vertex2[0].right_same_bare(input_r);
        }
#endif
        if (channel != 't')
            res = res_l_V * Pival * res_r_V;
        else
            res = res_l_V * Pival * (res_r_V + res_r_Vhat) + (res_l_V + res_l_Vhat) * Pival * res_r_V;

        return res;
    }

    void save_integrand() {
        int npoints = 1000;
        rvec freqs (npoints);

        rvec integrand_re (npoints);
        rvec integrand_im (npoints);
        rvec Pival_re (npoints);
        rvec Pival_im (npoints);
        for (int i=0; i<npoints; ++i) {
            double wl = vertex1[0].avertex().frequencies.b_K1.w_lower/50.;
            double wu = vertex1[0].avertex().frequencies.b_K1.w_upper/50.;
            double vpp = wl + i * (wu-wl)/(npoints-1);
            Q integrand_value = (*this)(vpp);
            integrand_re[i] = integrand_value.real();
            integrand_im[i] = integrand_value.imag();
            freqs[i] = vpp;

            Q Pival = Pi.value(i2, w, vpp, i_in, channel);
            Pival_re[i] = Pival.real();
            Pival_im[i] = Pival.imag();
        }

        string filename = "../Data/integrand_K1";
        filename += channel;
        filename += "_i0=" + to_string(i0)
                  + "_i2=" + to_string(i2)
                  + "_w=" + to_string(w) + ".h5";
        write_h5_rvecs(filename,
                {"v", "integrand_re", "integrand_im", "Pival_re", "Pival_im"},
                {freqs, integrand_re, integrand_im, Pival_re, Pival_im});
    }

};
template <typename Q,
          template <typename> class symmetry_left,
          template <typename> class symmetry_right>
class Integrand_K2 {
    const GeneralVertex<Q, symmetry_left>& vertex1;
    const GeneralVertex<Q, symmetry_right>& vertex2;
    const Bubble& Pi;
    int i0;
    int i2;
#ifdef DEBUG_MODE
    int iK_select;
    int iK_select_bubble;
#endif
    const int i_in;
    const char channel;
    const double w, v;
    const bool diff;
public:
    /**
     * Constructor:
     * @param vertex1_in : left vertex
     * @param vertex2_in : right vertex
     * @param Pi_in      : Bubble object connecting the left and right vertex
     * @param i0_in      : index (0,...,4) specifying the (external) Keldysh component of integrand object
     *                     (converted into actual Keldysh index i0 within the constructor)
     * @param w_in       : external bosonic frequency \omega
     * @param v_in       : external fermionic frequency \nu
     * @param i_in_in    : external index for internal structure
     * @param ch_in      : diagrammatic channel ('a', 'p', 't')
     * @param pt_in      : For multi-loop calculation: specify if one computes left ('L') or right ('R')
     *                     multi-loop contribution.
     * @param diff_in    : determines whether to compute differentiated or non-differentiated bubble
     */
    Integrand_K2(const GeneralVertex<Q, symmetry_left>& vertex1_in,
                 const GeneralVertex<Q, symmetry_right>& vertex2_in,
                 const Bubble& Pi_in,
                 int i0_in, int i2_in, const double w_in, double v_in, const int i_in_in,
                 const char ch_in, const bool diff_in
#ifdef DEBUG_MODE
                 , const int iK_select_in, const int iK_select_bubble_in
#endif
                 )
               : vertex1(vertex1_in), vertex2(vertex2_in), Pi(Pi_in),
                 i2(i2_in), w(w_in), v(v_in), i_in(i_in_in), channel(ch_in), diff(diff_in)
#ifdef DEBUG_MODE
                 , iK_select(iK_select_in), iK_select_bubble(iK_select_bubble_in)
#endif
    {
#ifdef KELDYSH_FORMALISM
        // converting index i0_in (0,...,4) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a': i0 = non_zero_Keldysh_K2a[i0_in]; break;
            case 'p': i0 = non_zero_Keldysh_K2p[i0_in]; break;
            case 't': i0 = non_zero_Keldysh_K2t[i0_in]; break;
            default: ;
        }
#else
        i0=0;
#endif
    };

    /**
     * Call operator:
     * @param vpp : frequency at which to evaluate integrand (to be integrated over)
     * @return Q  : value of the integrand object evaluated at frequency vpp (comp or double)
     */
    auto operator() (double vpp) const -> Q {
        Q res, res_l_V, res_r_V, res_l_Vhat, res_r_Vhat;
        vector<int> indices = indices_sum(i0, i2, channel);

#ifdef DEBUG_MODE
        if (channel == 'a') {
            if (indices[0] != iK_select && iK_select < 16) return 0.;
            if (i2 != iK_select_bubble && iK_select_bubble < 16) return 0.;
        }
#endif
        Q Pival = Pi.value(i2, w, vpp, i_in, channel);

        VertexInput input_l (indices[0], w, v, vpp, i_in, 0, channel);
        VertexInput input_r (indices[1], w, vpp, 0., i_in, 0, channel);
        res_l_V = vertex1[0].left_diff_bare(input_l);
        res_r_V = vertex2[0].right_same_bare(input_r);

        if (channel != 't') {
            res = res_l_V * Pival * res_r_V;
        }
        else {
            input_l.spin = 1;
            input_r.spin = 1;
            res_l_Vhat = vertex1[0].left_diff_bare(input_l);
            res_r_Vhat = vertex2[0].right_same_bare(input_r);

            res = res_l_V * Pival * (res_r_V + res_r_Vhat) + (res_l_V + res_l_Vhat) * Pival * res_r_V;
        }
        return res;
    }

    void save_integrand() const {
        int npoints = 1000;
        rvec freqs (npoints);
        rvec integrand_re (npoints);
        rvec integrand_im (npoints);
        rvec Pival_re (npoints);
        rvec Pival_im (npoints);
        for (int i=0; i<npoints; ++i) {
            double wl = vertex1[0].avertex().frequencies.f_K2.w_lower;
            double wu = vertex1[0].avertex().frequencies.f_K2.w_upper;
            double vpp = wl + i * (wu-wl)/(npoints-1);
            freqs[i] = vpp;

            Q integrand_value = (*this)(vpp);
            integrand_re[i] = integrand_value.real();
            integrand_im[i] = integrand_value.imag();

            Q Pival = Pi.value(i2, w, vpp, i_in, channel);
            Pival_re[i] = Pival.real();
            Pival_im[i] = Pival.imag();
        }

        string filename = "../Data/integrand_K2";
        filename += channel;
        filename += "_i0=" + to_string(i0)
                    + "_i2=" + to_string(i2)
                    + "_w=" + to_string(w)
                    + "_v=" + to_string(v)
                    + ".h5";
        write_h5_rvecs(filename,
                       {"v", "integrand_re", "integrand_im", "Pival_re", "Pival_im"},
                       {freqs, integrand_re, integrand_im, Pival_re, Pival_im});
    }

};
template <typename Q,
          template <typename> class symmetry_left,
          template <typename> class symmetry_right>
class Integrand_K3 {
    const GeneralVertex<Q, symmetry_left>& vertex1;
    const GeneralVertex<Q, symmetry_right>& vertex2;
    const Bubble& Pi;
    int i0;
    const int i_in;
    const char channel;
    const double w, v, vp;
    const bool diff;
public:
    /**
     * Constructor:
     * @param vertex1_in : left vertex
     * @param vertex2_in : right vertex
     * @param Pi_in      : Bubble object connecting the left and right vertex
     * @param i0_in      : index (0,...,5) specifying the (external) Keldysh component of integrand object
     *                     (converted into actual Keldysh index i0 within the constructor)
     * @param w_in       : external bosonic frequency \omega
     * @param v_in       : external fermionic frequency \nu
     * @param vp_in      : external fermionic frequency \nu'
     * @param i_in_in    : external index for internal structure
     * @param ch_in      : diagrammatic channel ('a', 'p', 't')
     * @param pt_in      : For multi-loop calculation: specify if one computes left ('L') or right ('R')
     *                     multi-loop contribution.
     */
    Integrand_K3(const GeneralVertex<Q, symmetry_left>& vertex1_in,
                 const GeneralVertex<Q, symmetry_right>& vertex2_in,
                 const Bubble& Pi_in, int i0_in,
                 const double w_in, const double v_in, const double vp_in, const int i_in_in,
                 const char ch_in, const bool diff_in)
               : vertex1(vertex1_in), vertex2(vertex2_in), Pi(Pi_in), w(w_in), v(v_in), vp(vp_in), i_in(i_in_in),
                 channel(ch_in), diff(diff_in)
    {
#ifdef KELDYSH_FORMALISM
        i0 = non_zero_Keldysh_K3[i0_in]; // converting index i0_in (0,...,5) into actual Keldysh index i0 (0,...,15)
#else
        i0 = 0;
#endif
    };

    /**
     * Call operator:
     * @param vpp : frequency at which to evaluate integrand (to be integrated over)
     * @return Q  : value of the integrand object evaluated at frequency vpp (comp or double)
     */
    auto operator() (double vpp) const -> Q {
        Q res, res_l_V, res_r_V,  res_l_Vhat, res_r_Vhat;
        Q Pival;
        vector<int> indices(2);

        //Iterates over all Keldysh components of the bubble which are nonzero
#ifdef KELDYSH_FORMALISM
        for (auto i2:non_zero_Keldysh_bubble) {
#else
            int i2=0;
#endif
            indices = indices_sum(i0, i2, channel);
            Pival = Pi.value(i2, w, vpp, i_in, channel);

            VertexInput input_l (indices[0], w, v, vpp, i_in, 0, channel);
            VertexInput input_r (indices[1], w, vpp, vp, i_in, 0, channel);

            res_l_V = vertex1[0].left_diff_bare(input_l);
            res_r_V = vertex2[0].right_diff_bare(input_r);

            if (channel != 't') {
                res += res_l_V * Pival * res_r_V;
            }
            else {
                input_l.spin = 1;
                input_r.spin = 1;
                res_l_Vhat = vertex1[0].left_diff_bare(input_l);
                res_r_Vhat = vertex2[0].right_diff_bare(input_r);

                res += res_l_V * Pival * (res_r_V + res_r_Vhat) + (res_l_V + res_l_Vhat) * Pival * res_r_V;
            }
#ifdef KELDYSH_FORMALISM
        }
#endif
        return res;
    }

    void save_integrand() const {
        int npoints = 1000;
        rvec freqs (npoints);
        rvec integrand_re (npoints);
        rvec integrand_im (npoints);
        for (int i=0; i<npoints; ++i) {
            double wl = vertex1[0].avertex().frequencies.f_K3.w_lower;
            double wu = vertex1[0].avertex().frequencies.f_K3.w_upper;
            double vpp = wl + i * (wu-wl)/(npoints-1);
            freqs[i] = vpp;

            Q integrand_value = (*this)(vpp);
            integrand_re[i] = integrand_value.real();
            integrand_im[i] = integrand_value.imag();
        }

        string filename = "../Data/integrand_K3";
        filename += channel;
        filename += "_i0=" + to_string(i0)
                    + "_w=" + to_string(w)
                    + "_v=" + to_string(v)
                    + "_vp=" + to_string(vp)
                    + ".h5";
        write_h5_rvecs(filename,
                       {"v", "integrand_re", "integrand_im"},
                       {freqs, integrand_re, integrand_im});
    }
};


/**
 * Function that computes the bubble frequency integral for all external parameters,
 * including MPI and OMP parallelization. The first (reference) argument contains the result.
 * Distributing the tasks between MPI/OMP is done explicitly before each for-loop for each diagrammatic class
 * K1, K2, K3. (TODO: handle this globally / in parameters.h?)
 *
 * @tparam Q      : data type (comp or double)
 * @param dgamma  : result (full Vertex object)
 * @param vertex1 : left vertex in bubble
 * @param vertex2 : right vertex in bubble
 * @param G       : first propagator of bubble
 * @param S       : second propagator of bubble
 * @param channel : diagrammatic channel ('a', 'p', or 't')
 * @param diff    : whether or not the bubble is a differentiated one
 */
template <typename Q,
          template <typename> class symmetry_result,
          template <typename> class symmetry_left,
          template <typename> class symmetry_right>
void bubble_function(GeneralVertex<Q, symmetry_result>& dgamma,
                     const GeneralVertex<Q, symmetry_left>& vertex1,
                     const GeneralVertex<Q, symmetry_right>& vertex2,
                     const Propagator& G, const Propagator& S, const char channel, const bool diff
#ifdef DEBUG_MODE
                     , const int iK_select, const int iK_select_bubble, const int iK_select2, const int iK_select_bubble2
#endif
                     )
{
    Bubble Pi(G, S, diff); // initialize bubble object
    // Alternatively: PrecalculateBubble Pi(G, S, diff, channel);

    int nw1_w = 0, nw2_w = 0, nw2_v = 0, nw3_w = 0, nw3_v = 0, nw3_v_p = 0;
    Q prefactor = 1.;

    // set channel-specific frequency ranges and prefactor (1, 1, -1 for a, p, t) for sum over spins.
    switch (channel) {
        case 'a':
            nw1_w = nw1_a;
            nw2_w = nw2_a;
            nw2_v = nv2_a;
            nw3_w = nw3_a;
            nw3_v = nv3_a;
            nw3_v_p = nv3_a;
            prefactor *= 1.;
            break;
        case 'p':
            nw1_w = nw1_p;
            nw2_w = nw2_p;
            nw2_v = nv2_p;
            nw3_w = nw3_p;
            nw3_v = nv3_p;
            nw3_v_p = nv3_p;
            prefactor *= 1.;
            break;
        case 't':
            nw1_w = nw1_t;
            nw2_w = nw2_t;
            nw2_v = nv2_t;
            nw3_w = nw3_t;
            nw3_v = nv3_t;
            nw3_v_p = nv3_t;
            prefactor *= -1.;
            break;
        default: ;
    }

    int mpi_size = mpi_world_size(); // number of mpi processes
    int mpi_rank = mpi_world_rank(); // number of the current mpi process

#ifdef DIAG_CLASS
#if DIAG_CLASS >= 0
    // initialize frequency grid to be used for K1
    FrequencyGrid freqs_K1 = dgamma[0].avertex().frequencies.b_K1;
    // use min/max of selfenergy/K1 frequency grids as integration limits
    double vmin = min(freqs_K1.w_lower, G.selfenergy.frequencies.w_lower);
    double vmax = max(freqs_K1.w_upper, G.selfenergy.frequencies.w_upper);
#endif
#if DIAG_CLASS >= 2
    // initialize frequency grids to be used for K2
    FrequencyGrid bfreqs_K2 = dgamma[0].avertex().frequencies.b_K2;
    FrequencyGrid ffreqs_K2 = dgamma[0].avertex().frequencies.f_K2;
    // use min/max of selfenergy/K1/K2 frequency grids as integration limits
    vmin = min(vmin, ffreqs_K2.w_lower);
    vmax = max(vmax, ffreqs_K2.w_upper);
#endif
#if DIAG_CLASS >= 3
    // initialize frequency grids to be used for K3
    FrequencyGrid bfreqs_K3 = dgamma[0].avertex().frequencies.b_K3;
    FrequencyGrid ffreqs_K3 = dgamma[0].avertex().frequencies.f_K3;
    // use min/max of selfenergy/K1/K2/K3 frequency grids as integration limits
    vmin = min(vmin, ffreqs_K3.w_lower);
    vmax = max(vmax, ffreqs_K3.w_upper);
#endif

#if DIAG_CLASS >= 0
//    double tK1 = get_time();
    /*K1 contributions*/
    int n_mpi = 1;                      // set external arguments for MPI-parallelization (# of tasks distributed via MPI)
    int n_omp = nK_K1 * nw1_w * n_in;   // set external arguments for OMP-parallelization (# of tasks per MPI-task distributed via OMP)

    // initialize buffer into which each MPI process writes their results
    vec<Q> K1_buffer = mpi_initialize_buffer<Q>(n_mpi, n_omp);

    // start for-loop over external arguments, using MPI and OMP
    int iterator = 0;
    for (int i_mpi=0; i_mpi<n_mpi; ++i_mpi) {
        if (i_mpi % mpi_size == mpi_rank) {
#pragma omp parallel for schedule(static, 8)
            for (int i_omp=0; i_omp<n_omp; ++i_omp) {
                // converting external MPI/OMP indices to physical indices (TODO: put into extra function(s)?)
                int iK1 = i_mpi * n_omp + i_omp;
                int i0 = iK1/(nw1_w*n_in);      // exterior Keldysh indices of the bubble
                int iw = iK1/(n_in) - i0*nw1_w; // frequency index
                int i_in = iK1 - i0*nw1_w*n_in - iw*n_in; // internal index
                double w = freqs_K1.w[iw];      // frequency acc. to frequency index
                Q value;

                int trafo = 1;
                int sign_w = sign_index<double>(w);
                switch (channel) {
                    case 'a':
                        trafo = TransformaK1a[i0][sign_w];
                        //cout << "Ping!" << trafo << "\n";
                        break;
                    case 'p':
                        trafo = TransformaK1p[i0][sign_w];
                        break;
                    case 't':
                        trafo = TransformaK1t[i0][sign_w];
                        break;
                    default:
                        cout << "\n Uooooohhh, sth went wrong! \n \n";
                }

                if (trafo == 0) {

                    // initialize the integrand object and perform frequency integration
                    if (vertex1[0].Ir() && vertex2[0].Ir()) value = 0.; // bubbles with Ir do not contribute to K1
                    else {
#ifdef KELDYSH_FORMALISM
                        for (auto i2:non_zero_Keldysh_bubble) {
#else
                        int i2=0;
#endif
#ifdef DEBUG_MODE
                            Integrand_K1<Q, symmetry_left, symmetry_right>
                                    integrand_K1(vertex1, vertex2, Pi, i0, i2, w, i_in, channel, diff,
                                                 iK_select, iK_select_bubble);
#else
                        Integrand_K1<Q, symmetry_left, symmetry_right>
                                integrand_K1(vertex1, vertex2, Pi, i0, i2, w, i_in, channel, diff);
                        /* // save the integrand for manual checks:
                        if (i_omp == 100){
                            integrand_K1.save_integrand();
                        }
                        // */
#endif

#ifdef KELDYSH_FORMALISM
                           value += prefactor * (1. / (2. * M_PI * glb_i)) * integrator(integrand_K1, vmin, vmax, -w / 2., w / 2.);
#else
                           value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K1, vmin, -abs(w/2)-inter_tol, -w / 2., w / 2.);
                           if( -abs(w/2)+inter_tol < abs(w/2)-inter_tol){
                               value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K1, -abs(w/2)+inter_tol, abs(w/2)-inter_tol, -w / 2., w / 2.);
                           }
                           value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K1, abs(w/2)+inter_tol, vmax, -w / 2., w / 2.);
#endif


                           /* asymptotic corrections temporarily commented out --> TODO: fix
                           if (!diff) {
                               value += prefactor * (1. / (2. * M_PI * glb_i)) *
                                        asymp_corrections_K1(vertex1, vertex2, -vmin, vmax, w, i0, i2, i_in,
                                                             channel); //Correction needed for the K1 class
                           }
                           // */
#ifdef KELDYSH_FORMALISM
                      }
#endif
                    }
                }
                K1_buffer[iterator*n_omp + i_omp] = value; // write result of integration into MPI buffer
            }
            ++iterator;
        }
    }

    // collect+combine results from different MPI processes, reorder them appropriately
    vec<Q> K1_result = mpi_initialize_result<Q> (n_mpi, n_omp);
    mpi_collect(K1_buffer, K1_result, n_mpi, n_omp);
    vec<Q> K1_ordered_result = mpi_reorder_result(K1_result, n_mpi, n_omp);

    switch (channel) {
        case 'a':
            dgamma[0].avertex().K1 += K1_ordered_result;
            dgamma[0].avertex().enforce_freqsymmetriesK1();
            break;
        case 'p':
            dgamma[0].pvertex().K1 += K1_ordered_result;
            dgamma[0].pvertex().enforce_freqsymmetriesK1();
            break;
        case 't':
            dgamma[0].tvertex().K1 += K1_ordered_result;
            dgamma[0].tvertex().enforce_freqsymmetriesK1();
            break;
        default: ;
    }
//    print("K1", channel, " done: ");
//    get_time(tK1);
#endif

#if DIAG_CLASS >= 2
//    double tK2 = get_time();
    /*K2 contributions*/
    n_mpi = 1;
    n_omp = nK_K2 * nw2_w * nw2_v * n_in;

    // initialize buffer into which each MPI process writes their results
    vec<Q> K2_buffer = mpi_initialize_buffer<Q>(n_mpi, n_omp);

    // start for-loop over external arguments, using MPI and OMP
    iterator = 0;
    for (int i_mpi=0; i_mpi<n_mpi; ++i_mpi) {
        if (i_mpi % mpi_size == mpi_rank) {
#pragma omp parallel for schedule(static, 8)
            for (int i_omp=0; i_omp<n_omp; ++i_omp) {
                // converting external MPI/OMP indices to physical indices
                int iK2 = i_mpi * n_omp + i_omp;
                int i0 = iK2 / (nw2_w * nw2_v * n_in);
                int iw = iK2 / (nw2_v * n_in) - i0 * nw2_w;
                int iv = iK2 / n_in - iw * nw2_v - i0 * nw2_w * nw2_v;
                int i_in = iK2 - iv * n_in - iw * nw2_v * n_in - i0 * nw2_w * nw2_v * n_in;
                double w = bfreqs_K2.w[iw];
                double v = ffreqs_K2.w[iv];
                Q value;

                int trafo = 1;
                int sign_w = sign_index<double>(w);
                int sign_v = sign_index<double>(v);
                switch (channel) {
                    case 'a':
                        trafo = TransformaK2a[i0][sign_w * 2 + sign_v];
                        //cout << "Ping!" << trafo << "\n";
                        break;
                    case 'p':
                        trafo = TransformaK2p[i0][sign_w * 2 + sign_v];
                        break;
                    case 't':
                        trafo = TransformaK2t[i0][sign_w * 2 + sign_v];
                        break;
                    default:
                        cout << "\n Uooooohhh, sth went wrong! \n \n";
                }

                if (trafo == 0) {

                    // initialize the integrand object and perform frequency integration
                    if (vertex2[0].Ir()) value = 0.;  // right part of multi-loop contribution does not contribute to K2 class
                        // TODO: attention: central part does contribute, but we treat it as right part of previous loop --> fix this!! --> ?
                    else {
#ifdef KELDYSH_FORMALISM
                        for(auto i2:non_zero_Keldysh_bubble) {
#else
                        int i2 = 0;
#endif
#ifdef DEBUG_MODE
                        Integrand_K2<Q, symmetry_left, symmetry_right>
                                integrand_K2(vertex1, vertex2, Pi, i0, i2, w, v, i_in, channel, diff,
                                             iK_select2, iK_select_bubble2);
#else
                        Integrand_K2<Q, symmetry_left, symmetry_right>
                                integrand_K2(vertex1, vertex2, Pi, i0, i2, w, v, i_in, channel, diff);
                        /*if (i_omp == (int)n_omp-304){
                            integrand_K2.save_integrand();
                        }*/
#endif

#ifdef KELDYSH_FORMALISM
                        value += prefactor * (1. / (2. * M_PI * glb_i)) * integrator(integrand_K2, vmin, vmax, -w / 2., w / 2.);
#else
                        value += prefactor * (1. / (2. * M_PI)) *
                                 integrator(integrand_K2, vmin, -abs(w / 2) - inter_tol, -w / 2., w / 2.);
                        if (-abs(w / 2) + inter_tol < abs(w / 2) - inter_tol) {
                            value += prefactor * (1. / (2. * M_PI)) *
                                     integrator(integrand_K2, -abs(w / 2) + inter_tol, abs(w / 2) - inter_tol,
                                                -w / 2., w / 2.);
                        }
                        value += prefactor * (1. / (2. * M_PI)) *
                                 integrator(integrand_K2, abs(w / 2) + inter_tol, vmax, -w / 2., w / 2.);
#endif
                        /* asymptotic corrections temporarily commented out --> TODO: fix
                        if (!diff) {
                            value +=
#ifdef KELDYSH_FORMALISM
                                prefactor * (1. / (2. * M_PI * glb_i)) *
#else
                                prefactor * (1. / (-2. * M_PI)) *
#endif
                                    asymp_corrections_K2(vertex1, vertex2, -vmin, vmax, w, v, i0, i2,
                                                      i_in, channel); //Correction needed for the K2 class
                        }
                        // */
#ifdef KELDYSH_FORMALISM
                        }
#endif
                    }
                }
                K2_buffer[iterator*n_omp + i_omp] = value; // write result of integration into MPI buffer
            }
            ++iterator;
        }
    }

    // collect+combine results from different MPI processes, reorder them appropriately
    vec<Q> K2_result = mpi_initialize_result<Q> (n_mpi, n_omp);
    mpi_collect(K2_buffer, K2_result, n_mpi, n_omp);
    vec<Q> K2_ordered_result = mpi_reorder_result(K2_result, n_mpi, n_omp);

    switch (channel) {
        case 'a':
            dgamma[0].avertex().K2 += K2_ordered_result;
            dgamma[0].avertex().enforce_freqsymmetriesK2();
            break;
        case 'p':
            dgamma[0].pvertex().K2 += K2_ordered_result;
            dgamma[0].pvertex().enforce_freqsymmetriesK2();
            break;
        case 't':
            dgamma[0].tvertex().K2 += K2_ordered_result;
            dgamma[0].tvertex().enforce_freqsymmetriesK2();
            break;
        default: ;
    }

//    print("K2", channel, " done: ");
//    get_time(tK2);
#endif

#if DIAG_CLASS >= 3
    double tK3 = get_time();
    /*K3 contributions*/
    n_mpi = 1;
    n_omp = nK_K3 * nw3_w * nw3_v * nw3_v_p * n_in;

    // initialize buffer into which each MPI process writes their results
    vec<Q> K3_buffer = mpi_initialize_buffer<Q>(n_mpi, n_omp);

    // start for-loop over external arguments, using MPI and OMP
    iterator = 0;
    for (int i_mpi=0; i_mpi<n_mpi; ++i_mpi) {
        if (i_mpi % mpi_size == mpi_rank) {
#pragma omp parallel for schedule(static, 8)
            for (int i_omp=0; i_omp<n_omp; ++i_omp) {
                // converting external MPI/OMP indices to physical indices
                int iK3 = i_mpi * n_omp + i_omp;
                int i0 = iK3/(nw3_w * nw3_v * nw3_v_p * n_in);
                int iw = iK3/(nw3_v * nw3_v_p * n_in) - i0*nw3_w;
                int iv = iK3/(nw3_v * n_in) - i0*nw3_w*nw3_v - iw*nw3_v;
                int ivp =iK3/(n_in) - i0*nw3_w*nw3_v*nw3_v_p - iw*nw3_v*nw3_v_p - iv*nw3_v_p;
                int i_in = iK3 - i0*nw3_w*nw3_v*nw3_v_p*n_in - iw*nw3_v*nw3_v_p*n_in - iv*nw3_v_p*n_in - ivp*n_in;
                double w = bfreqs_K3.w[iw];
                double v = ffreqs_K3.w[iv];
                double vp = ffreqs_K3.w[ivp];
                Q value;

                int trafo = 1;
                int sign_w = sign_index<double>(w);
                int sign_f = sign_index<double>(v+vp);
                int sign_fp = sign_index<double>(v-vp);
                switch (channel) {
                    case 'a':
                        trafo = TransformaK3a[i0][sign_w * 4 + sign_f * 2 + sign_fp];
                        //cout << "Ping!" << trafo << "\n";
                        break;
                    case 'p':
                        trafo = TransformaK3p[i0][sign_w * 4 + sign_f * 2 + sign_fp];
                        break;
                    case 't':
                        trafo = TransformaK3t[i0][sign_w * 4 + sign_f * 2 + sign_fp];
                        break;
                    default:
                        cout << "\n Uooooohhh, sth went wrong! \n \n";
                }

                if (trafo == 0) {

                    // initialize the integrand object and perform frequency integration
                    Integrand_K3<Q, symmetry_left, symmetry_right>
                            integrand_K3(vertex1, vertex2, Pi, i0, w, v, vp, i_in, channel, diff);
//                    if (i_omp == 4189){
//                        integrand_K3.save_integrand();
//                    }

#ifdef KELDYSH_FORMALISM
                    value += prefactor * (1. / (2. * M_PI * glb_i)) *
                             integrator(integrand_K3, vmin, vmax, -w / 2., w / 2.);
#else
                    value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K3, vmin, -abs(w/2)-inter_tol, -w / 2., w / 2.);
                    if( -abs(w/2)+inter_tol < abs(w/2)-inter_tol){
                        value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K3, -abs(w/2)+inter_tol, abs(w/2)-inter_tol, -w / 2., w / 2.);
                    }
                    value += prefactor * (1. / (2. * M_PI)) * integrator(integrand_K3, abs(w/2)+inter_tol, vmax, -w / 2., w / 2.);
#endif


                    /* asymptotic corrections temporarily commented out --> TODO: fix
                    if (!diff) {
#ifdef KELDYSH_FORMALISM
                        for(auto i2:non_zero_Keldysh_bubble) {
#else
                            int i2=0;
#endif
                            value += prefactor * (1. / (2. * M_PI * glb_i)) *
                                     asymp_corrections_K3(vertex1, vertex2, -vmin, vmax, w, v, vp, i0, i2,
                                                          i_in, channel); //Correction needed for the K3 class
#ifdef KELDYSH_FORMALISM
                        }
#endif
                    }
                    // */
                }
                K3_buffer[iterator*n_omp + i_omp] = value; // write result of integration into MPI buffer
            }
            ++iterator;
        }
    }

    // collect+combine results from different MPI processes, reorder them appropriately
    vec<Q> K3_result = mpi_initialize_result<Q> (n_mpi, n_omp);
    mpi_collect(K3_buffer, K3_result, n_mpi, n_omp);
    vec<Q> K3_ordered_result = mpi_reorder_result(K3_result, n_mpi, n_omp);

    switch (channel) {
        case 'a':
            dgamma[0].avertex().K3 += K3_ordered_result;
            dgamma[0].avertex().enforce_freqsymmetriesK3();
            break;
        case 'p':
            dgamma[0].pvertex().K3 += K3_ordered_result;
            dgamma[0].pvertex().enforce_freqsymmetriesK3();
            break;
        case 't':
            dgamma[0].tvertex().K3 += K3_ordered_result;
            dgamma[0].tvertex().enforce_freqsymmetriesK3();
            break;
        default: ;
    }

    print("K3", channel, " done: ");
    get_time(tK3);
#endif

#if DIAG_CLASS>=4
    print("Damn son, this is a bad error");
#endif
#endif
}

#ifdef DEBUG_MODE
template <typename Q, template <typename> class container_type>
void bubble_function(GeneralVertex<Q, container_type>& dgamma,
                     const GeneralVertex<Q, container_type>& vertex1, const GeneralVertex<Q, container_type>& vertex2,
                     const Propagator& G, const Propagator& S, const char channel, const bool diff)
{
    bubble_function(dgamma, vertex1, vertex2, G, S, channel, diff, 16, 16, 16, 16);
}
#endif

#endif //KELDYSH_MFRG_BUBBLES_H
