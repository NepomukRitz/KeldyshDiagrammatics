//
// Created by Marcel on 04.08.2021.
//

#include <numeric>
#include <string>
#include "../data_structures.h"                // real and complex vectors
#include "../utilities/write_data2file.h"             // write vectors into hdf5 file
#include <gsl/gsl_integration.h>            // for GSL integrator
#include <gsl/gsl_errno.h>                  // for GSL integrator
#include <complex>          // for usage of complex numbers
#include <cmath>            // for math. operations (real, imag, abs etc.)
#include <vector>           // vec class is derived from vector class
#include <initializer_list> // to initialize vec class with initializer list
#include <gsl/gsl_math.h>
#include <gsl/gsl_monte.h>
#include <gsl/gsl_monte_plain.h>
#include <gsl/gsl_monte_miser.h>
#include <gsl/gsl_monte_vegas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_deriv.h>      // numerical derivative
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv2.h>     // ordinary differential equations
//#include "../integrator.h"

#include "Momentum-integral-Bubble.h"
#include "Ladder-approximation.h"
#include "fRG-T-matrix-approach.h"

#ifndef MAIN_CPP_ZEROS_H
#define MAIN_CPP_ZEROS_H

template <typename F>
double find_root_newton (F f, double start, double dx, int imax, double prec) {
    double xnew = start;
    double fold, fold_plus, df;
    double xold;

    for (int i = 1; i < imax + 1; ++i){
        xold = xnew;
        fold = f(xold);
        while (std::abs(fold)>1/prec){ // avoid infinite f
            xold = xold + dx;
            fold = f(xold);
        }
        fold_plus = f(xold+dx);
        df = (fold_plus-fold)/dx;

        if (std::abs(df) < prec){ // stop if the denominator is too small
            break;
        }

        xnew = xold - fold/df; // Newton's computation
        if (xnew > 1) {
            xnew = xold + std::abs(xnew-xold)/10.;
        }

        if (std::abs(xnew-xold)<dx) { // stop when result is within tolerance
            break;
        }

    }
    return xnew;
}

template <typename F>
double find_root_divergence (F f, double start, double dxstart, int imax, double prec) {
    double xnew = start;
    double dxnew = dxstart;
    vec<double> xs(imax);
    vec<double> fs(imax);

    double xold, dxold;
    double fold, fnew;

    for (int i = 1; i < imax + 1; ++i){
        xold = xnew;
        dxold = dxnew;
        fold = f(xold);
        while (std::abs(fold)>1/prec){ // avoid infinite f
            fold = f(xold-dxold);
        }
        dxold = dxnew;
        xnew = xold + dxold;

        fnew = f(xnew);
        if ((abs(fnew)<prec)||(xnew > 1)||isnan(fnew)==true) {
            xnew = xold;
            dxnew = dxold/2.;
        }
        if (abs(dxnew)<prec){
            break;
        }

        // std::cout << "i = " << i << ": mu = " << xold << ", f_mu = " << fold << "\n";

        xs[i] = xold;
        fs[i] = fold;
    }
    /*
    for (int i = 0; i<imax; ++i){
        std::cout << "i = " << i << ": x = " << xs[i] << ", f(x) = " << fs[i] << "\n";
    }
     */

    return xold;
}


comp sigmoidal(double x){
    return 1./(1.+exp(x));
}

double fzeros1(double x){
    double f;
    f = 0.02*pow(x,4)+0.1*pow(x,3)-0.2*pow(x,2)+0.7*x-12;
    return f;
}

double fzeros2(double x){
    double f;
    double x0 = pow(M_PI,3)/log(2)/(10*M_EULER);
    if (x <= x0){
        f = sqrt(abs(x-x0));
    }
    else {
        f = -sqrt(abs(x-x0));
    }
    return f;
}

// MONTE-CARLO-INTEGRATION BUBBLE
// ==============================

struct bubble_params { double w; double vpp; double q; char i; char j; char r; bool complexity;};

double bubble_integrand_MC (double *k, size_t dim, void *params) {
    double prefactor;
    double v1;
    double v2;
    double q;
    char i;
    char j;
    char r;
    bool complexity;

    struct bubble_params *fp = (struct bubble_params *) params;

    (void) (dim);

    r = fp->r;

    if (r == 'a') {
        prefactor = 1.;
        v1 = fp->vpp + fp->w/2;
        v2 = fp->vpp - fp->w/2;
    }
    else if (r == 'p') {
        prefactor = 0.5;
        v1 = fp->w / 2 + fp->vpp;
        v2 = fp->w / 2 - fp->vpp;
    }
    else if (r == 't') {
        prefactor = -1.;
        v1 = fp->vpp + fp->w / 2;
        v2 = fp->vpp - fp->w / 2;
    }
    else {
        prefactor = 0.;
        v1 = 0.;
        v2 = 0.;
        std::cout << "wrong channel\n";
    }

    q = fp->q;
    i = fp->i;
    j = fp->j;
    complexity = fp->complexity;

    if (complexity == 0) {
        return prefactor * k[0] * k[0] * real(SimpleBubble(v1, v2, q, k[0], k[1], i, j)) / (4 * M_PI * M_PI);
    } else
        return prefactor * k[0] * k[0] * imag(SimpleBubble(v1, v2, q, k[0], k[1], i, j)) / (4 * M_PI * M_PI);
}

void integrate_bubble_full_monte_carlo (double w, double vpp, double q, char i, char j, char channel,
                                        double kmax, size_t calls, double vegas_chisq_precision) {

    double res, err;

    const gsl_rng_type *T;
    gsl_rng *r;

    double kl[2] = { 0., -1.};
    double ku[2] = { kmax, 1.};

    //size_t calls;

    gsl_rng_env_setup ();

    T = gsl_rng_default;
    r = gsl_rng_alloc (T);

    gsl_monte_function F_Re;
    gsl_monte_function F_Im;
    struct bubble_params Pi_Re_params = {w, vpp, q, i,j, channel, 0};
    struct bubble_params Pi_Im_params = {w, vpp, q, i,j, channel, 1};
    F_Re.f = &bubble_integrand_MC;
    F_Re.dim = 2;
    F_Re.params = &Pi_Re_params;
    F_Im.f = &bubble_integrand_MC;
    F_Im.dim = 2;
    F_Im.params = &Pi_Im_params;

    //gsl_monte_function F_Im = { &f_Im, 2, 0 };

    {
        gsl_monte_plain_state *s = gsl_monte_plain_alloc (2);
        gsl_monte_plain_integrate (&F_Re, kl, ku, 2, calls, r, s,
                                   &res, &err);

        gsl_monte_plain_free (s);

        std::cout << "Re plain result: " << res << ", error: " << err << "\n";

        gsl_monte_plain_state *ss = gsl_monte_plain_alloc (2);
        gsl_monte_plain_integrate (&F_Im, kl, ku, 2, calls, r, ss,
                                   &res, &err);

        gsl_monte_plain_free (ss);

        std::cout << "Im plain result: " << res << ", error: " << err << "\n";
    }

    {
        gsl_monte_miser_state *s = gsl_monte_miser_alloc (2);
        gsl_monte_miser_integrate (&F_Re, kl, ku, 2, calls, r, s,
                                   &res, &err);
        gsl_monte_miser_free (s);

        std::cout << "Re miser result: " << res << ", error: " << err << "\n";

        gsl_monte_miser_state *ss = gsl_monte_miser_alloc (2);
        gsl_monte_miser_integrate (&F_Im, kl, ku, 2, calls, r, ss,
                                   &res, &err);
        gsl_monte_miser_free (ss);

        std::cout << "Im miser result: " << res << ", error: " << err << "\n";
    }

    {
        gsl_monte_vegas_state *s = gsl_monte_vegas_alloc (2);

        gsl_monte_vegas_integrate (&F_Re, kl, ku, 2, calls/5, r, s,
                                   &res, &err);
        std::cout << "Re vegas result: " << res << ", error: " << err << "\n";

        printf ("converging...\n");

        do
        {
            gsl_monte_vegas_integrate (&F_Re, kl, ku, 2, calls, r, s,
                                       &res, &err);
            printf ("result = % .6f sigma = % .6f "
                    "chisq/dof = %.1f\n", res, err, gsl_monte_vegas_chisq (s));
        }
        while (std::abs (gsl_monte_vegas_chisq (s) - 1.0) > 0.1);

        std::cout << "Re vegas result: " << res << ", error: " << err << "\n";

        gsl_monte_vegas_free (s);

        gsl_monte_vegas_state *ss = gsl_monte_vegas_alloc (2);

        gsl_monte_vegas_integrate (&F_Im, kl, ku, 2, calls/5, r, ss,
                                   &res, &err);
        std::cout << "Im vegas result: " << res << ", error: " << err << "\n";

        printf ("converging...\n");

        do
        {
            gsl_monte_vegas_integrate (&F_Im, kl, ku, 2, calls, r, s,
                                       &res, &err);
            printf ("result = % .6f sigma = % .6f "
                    "chisq/dof = %.1f\n", res, err, gsl_monte_vegas_chisq (ss));
        }
        while (std::abs (gsl_monte_vegas_chisq (ss) - 1.0) > vegas_chisq_precision);

        std::cout << "Im vegas result: " << res << ", error: " << err << "\n";

        gsl_monte_vegas_free (ss);
    }
}

comp integrate_bubble_vegas (double w, double vpp, double q, char i, char j, char channel,
                             double kmax, size_t calls, double vegas_chisq_precision) {
    double res, err;
    comp res_comp;
    double res_Re, res_Im;

    const gsl_rng_type *T;
    gsl_rng *r;

    double kl[2] = { 0., -1.};
    double ku[2] = { kmax, 1.};

    gsl_rng_env_setup ();

    T = gsl_rng_default;
    r = gsl_rng_alloc (T);

    gsl_monte_function F_Re;
    gsl_monte_function F_Im;
    struct bubble_params Pi_Re_params = {w, vpp, q, i,j, channel, 0};
    struct bubble_params Pi_Im_params = {w, vpp, q, i,j, channel, 1};
    F_Re.f = &bubble_integrand_MC;
    F_Re.dim = 2;
    F_Re.params = &Pi_Re_params;
    F_Im.f = &bubble_integrand_MC;
    F_Im.dim = 2;
    F_Im.params = &Pi_Im_params;

    {
        gsl_monte_vegas_state *s = gsl_monte_vegas_alloc (2);

        gsl_monte_vegas_integrate (&F_Re, kl, ku, 2, calls/5, r, s,
                                   &res, &err);
        //std::cout << "Re vegas result: " << res << ", error: " << err << "\n";

        //printf ("converging...\n");

        do
        {
            gsl_monte_vegas_integrate (&F_Re, kl, ku, 2, calls, r, s,
                                       &res, &err);
            //printf ("result = % .6f sigma = % .6f "
            //        "chisq/dof = %.1f\n", res, err, gsl_monte_vegas_chisq (s));
        }
        while (std::abs (gsl_monte_vegas_chisq (s) - 1.0) > vegas_chisq_precision);

        //std::cout << "Re vegas result: " << res << ", error: " << err << "\n";

        res_Re = res;

        gsl_monte_vegas_free (s);

        gsl_monte_vegas_state *ss = gsl_monte_vegas_alloc (2);

        gsl_monte_vegas_integrate (&F_Im, kl, ku, 2, calls/5, r, ss,
                                   &res, &err);
        //std::cout << "Im vegas result: " << res << ", error: " << err << "\n";

        //printf ("converging...\n");

        if (res == 0. && err == 0.) {
            res_Im = 0.;
        } else {
            do
            {
                gsl_monte_vegas_integrate (&F_Im, kl, ku, 2, calls, r, s,
                                           &res, &err);
                //printf ("result = % .6f sigma = % .6f "
                //        "chisq/dof = %.1f\n", res, err, gsl_monte_vegas_chisq (ss));
            }
            while (std::abs (gsl_monte_vegas_chisq (ss) - 1.0) > vegas_chisq_precision);

            //std::cout << "Im vegas result: " << res << ", error: " << err << "\n";

            res_Im = res;
        }

        gsl_monte_vegas_free (ss);
    }

    res_comp = {res_Re, res_Im};

    return res_comp;
}

void integral_bubble_w_vpp_list_MC (char i, char j, char channel, double wmax, double vppmax, double qmax,
                                    double kmax, size_t calls, double vegas_chisq_precision, int nFER, int nBOS, int nq) {
    vec<double> vpps(nFER);
    vec<double> ws(nBOS);
    vec<double> qs(nq);
    vec<double> Pi_int_Re(nFER*nBOS*nq);
    vec<double> Pi_int_Im(nFER*nBOS*nq);
    //vector<comp> Pi_int(nFER*nBOS*nq);
    comp result_integral;
    double w;
    double vpp;
    double q;
    double output;

    for (int wi = 0; wi < nBOS; ++wi) {
        w = -wmax + 2*wi*wmax/(nBOS-1);
        ws[wi] = w;
        for (int vppi = 0; vppi < nFER; ++vppi) {
            vpp = -vppmax + 2*vppi*vppmax/(nFER-1);
            vpps[vppi] = vpp;
            for (int qi = 0; qi < nq; ++qi) {
                q = qi*qmax/(nq-1);
                qs[qi] = q;
                result_integral = integrate_bubble_vegas(w,vpp,q,i,j,channel,kmax,calls,vegas_chisq_precision);
                //Pi_int[composite_index_wvq(wi, vppi, nFER, qi, nq)] = result_integral;
                Pi_int_Re[composite_index_wvq(wi, vppi, nFER, qi, nq)] = real(result_integral);
                Pi_int_Im[composite_index_wvq(wi, vppi, nFER, qi, nq)] = imag(result_integral);
                std::cout << "w = " << w << ", vpp = " << vpp << ", q = " << q << ", result = " << result_integral << "\n";
            }
        }
    }

    std::string filename = "../Data/integrated_bubble_MC";
    filename += "_";
    filename += i;
    filename += j;
    filename += channel;
    filename += "_nBOS=" + std::to_string(nBOS)
                + "_nFER=" + std::to_string(nFER)
                + "_nq=" + std::to_string(nq)
                + ".h5";
    write_h5_rvecs(filename,
                   {"fermionic_frequencies", "bosonic_frequencies", "bosonic_momenta", "integrated_bubble_Re", "integrated_bubble_Im"},
                   {vpps, ws, qs, Pi_int_Re, Pi_int_Im});

}

/*void Runtime_comparison<Q>::test_runtimes(int max_number_of_iterations) {
    vec<double> times_usual (max_number_of_iterations);
    vec<double> times_precalculated (max_number_of_iterations);
    for (int t = 0; t < max_number_of_iterations; ++t) {
        times_usual[t] = run_iterations(t, 1);
        times_precalculated[t] = run_iterations(t, 0);
    }
    std::string filename = "../Data/runtime_comparisons.h5";
    write_h5_rvecs(filename,
                   {"runtimes_usual", "runtimes_precalculated"},
                   {times_usual, times_precalculated});
}*/

/*double f_Im (double *k, size_t dim, void *params)
{
    //struct f_struct * fparams = (struct f_struct *) params;
    (void)(dim); /* avoid unused parameter warnings */
/*(void)(params);

return k[0]*k[0]*imag(SimpleBubble(0.1, 0.1, 0.1, k[0], k[1], 'c', 'd'))/(4*M_PI*M_PI);
} */

// TRYING OUT GSL-ODE-SOLVER

struct params_K1{
    double w, q, g, Lambda_i;
    char r;
};

int K1cdcd(double t, const double y[], double f[], void *params) {
    struct params_K1 *p = (struct params_K1 *) params;
    double w = p->w;
    double q = p->q;
    double g = p->g;
    double Lambda_i = p->Lambda_i;
    char r = p->r;

    double Lambda = Lambda_i*exp(-t);

    double ReK1 = y[0];
    double ImK1 = y[1];


    double RePicd = real(sharp_frequency_bare_bubble ( w, Lambda, q, 'c', 'd', r, 0));
    double ImPicd = imag(sharp_frequency_bare_bubble ( w, Lambda, q, 'c', 'd', r, 0));
    double RePidc = real(sharp_frequency_bare_bubble ( w, Lambda, q, 'd', 'c', r, 0));
    double ImPidc = imag(sharp_frequency_bare_bubble ( w, Lambda, q, 'd', 'c', r, 0));
    double RePicc = real(sharp_frequency_bare_bubble ( w, Lambda, q, 'c', 'c', r, 0));
    double ImPicc = imag(sharp_frequency_bare_bubble ( w, Lambda, q, 'c', 'c', r, 0));
    double RePidd = real(sharp_frequency_bare_bubble ( w, Lambda, q, 'd', 'd', r, 0));
    double ImPidd = imag(sharp_frequency_bare_bubble ( w, Lambda, q, 'd', 'd', r, 0));

    double RegK1squared = g*g - 2*g*ReK1 + ReK1*ReK1 - ImK1*ImK1;
    double ImgK1squared = -2*g*ImK1 +2*ReK1*ImK1;

    if (r == 'p') {

        f[0] = -Lambda*(RegK1squared*(RePicd+RePidc)-ImgK1squared*(ImPicd+ImPidc));
        f[1] = -Lambda*(ImgK1squared*(RePicd+RePidc)+RegK1squared*(ImPicd+ImPidc));
    }
    else if (r == 'a') {
        f[0] = -Lambda*(RegK1squared*RePidc-ImgK1squared*ImPidc);
        f[1] = -Lambda*(ImgK1squared*RePidc+RegK1squared*ImPidc);
    }
    else if (r == 't') {
        double ReK1cccc = y[2];
        double ImK1cccc = y[3];
        double ReK1dddd = y[4];
        double ImK1dddd = y[5];
        double ReK1ccccsquared =  ReK1cccc*ReK1cccc-ImK1cccc*ImK1cccc;
        double ImK1ccccsquared = 2*ReK1cccc*ImK1cccc;
        double ReK1ddddsquared =  ReK1dddd*ReK1dddd-ImK1dddd*ImK1dddd;
        double ImK1ddddsquared = 2*ReK1dddd*ImK1dddd;
        double RegK1cccc =  (-g + ReK1)*ReK1cccc - ImK1*ImK1cccc;
        double ImgK1cccc = (-g + ReK1)*ImK1cccc + ImK1*ReK1cccc;
        double RegK1dddd =  (-g + ReK1)*ReK1dddd - ImK1*ImK1dddd;
        double ImgK1dddd = (-g + ReK1)*ImK1dddd + ImK1*ReK1dddd;
        f[0] = -Lambda*(RegK1cccc*RePicc-ImgK1cccc*ImPicc+RegK1dddd*RePidd-ImgK1dddd*ImPidd);
        f[1] = -Lambda*(RegK1cccc*ImPicc+ImgK1cccc*RePicc+RegK1dddd*ImPidd+ImgK1dddd*RePidd);
        f[2] = -Lambda*(ReK1ccccsquared*RePicc-ImK1ccccsquared*ImPicc+RegK1squared*RePidd-ImgK1squared*ImPidd);
        f[3] = -Lambda*(ReK1ccccsquared*ImPicc+ImK1ccccsquared*RePicc+RegK1squared*ImPidd+ImgK1squared*RePidd);
        f[4] = -Lambda*(ReK1ddddsquared*RePidd-ImK1ddddsquared*ImPidd+RegK1squared*RePicc-ImgK1squared*ImPicc);
        f[5] = -Lambda*(ReK1ddddsquared*ImPidd+ImK1ddddsquared*RePidd+RegK1squared*ImPicc+ImgK1squared*RePicc);
    }
    else {
        f[0] = 0.0;
        f[1] = 0.0;
        std::cout << "wrong channel\n";
    }

    return GSL_SUCCESS;
}

int solve_K1cdcd(double w, double q, double g, double Lambda_i, char r, double Lambda_f, double h, double epsabs, double epsrel) {
    struct params_K1 params_ode = {w,q,g,Lambda_i,r};

    if ( (r == 'p') || (r == 'a')) {
        gsl_odeiv2_system sys = {K1cdcd, nullptr, 2, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);
        double y[2] = {0,0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i/Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
            double Lambda = Lambda_i*exp(-t);
            printf("%.5e %.5e %.5e \n", Lambda, y[0], y[1]);
        }
    }
    else if (r == 't'){
        gsl_odeiv2_system sys = {K1cdcd, nullptr, 6, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);
        double y[6] = {0,0,0,0,0,0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i/Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
            double Lambda = Lambda_i*exp(-t);
            printf("%.5e %.5e %.5e \n", Lambda, y[0], y[1]);
        }
    }
    else {
        std::cout << "wrong channel\n";
    }

    return 0;
}

comp K1cdcd_solution(double w, double q, double g, double Lambda_i, char r, double Lambda_f, double h, double epsabs, double epsrel) {
    double ReK1, ImK1;
    comp result;
    struct params_K1 params_ode = {w,q,g,Lambda_i,r};

    if ( (r == 'p') || (r == 'a')) {
        gsl_odeiv2_system sys = {K1cdcd, nullptr, 2, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);

        double y[2] = {0, 0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i / Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
        }
        ReK1 = y[0];
        ImK1 = y[1];
        result = y[0]+glb_i*y[1];
    }
    else if (r == 't') {
        gsl_odeiv2_system sys = {K1cdcd, nullptr, 6, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);

        double y[6] = {0, 0, 0, 0, 0, 0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i / Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
        }
        ReK1 = y[0];
        ImK1 = y[1];
        result = y[0]+glb_i*y[1];
    }
    else {
        std::cout << "wrong channel\n";
    }

    return result;
}

void K1Lambda (double w, double q, double g, char channel, double Lambda_i, double Lambda_f, int nLambda, double h, double epsabs, double epsrel) {
    vec<double> Lambdas(nLambda);
    vec<double> K1Lambda_Re(nLambda);
    vec<double> K1Lambda_Im(nLambda);
    comp result_K1;

    double Lambda;
    double t = 0;

    for (int i = 0; i < nLambda; i++) {
        t = i * (log(Lambda_i / Lambda_f)) / (nLambda-1);
        Lambda = Lambda_i*exp(-t);
        result_K1 = K1cdcd_solution(w, q, g, Lambda_i, channel, Lambda, h, epsabs, epsrel);

        Lambdas[i] = Lambda;
        K1Lambda_Re[i] = real(result_K1);
        K1Lambda_Im[i] = imag(result_K1);

    }

    std::string filename = "../Data/K1Lambda";
    filename += "_";
    filename += channel;
    filename += "_Li=" + std::to_string(Lambda_i)
                + "_Lf=" + std::to_string(Lambda_f)
                + "_nL=" + std::to_string(nLambda)
                + ".h5";
    write_h5_rvecs(filename,
                   {"Lambdas", "K1Lambda_Re", "K1Lambda_Im"},
                   {Lambdas, K1Lambda_Re, K1Lambda_Im});

}

void K1Lambdag (double w, double q, double gmin, double gmax, char channel, double Lambda_i, double Lambda_f, int ng, double h, double epsabs, double epsrel) {
    vec<double> gs(ng);
    vec<double> K1Lambda_Re(ng);
    vec<double> K1Lambda_Im(ng);
    comp result_K1;

    double g;

    for (int i = 0; i < ng; i++) {
        g = gmin + std::abs(gmax-gmin) / (ng-1);
        result_K1 = K1cdcd_solution(w, q, g, Lambda_i, channel, Lambda_f, h, epsabs, epsrel);

        gs[i] = g;
        K1Lambda_Re[i] = real(result_K1);
        K1Lambda_Im[i] = imag(result_K1);

    }

    std::string filename = "../Data/K1Lambdag";
    filename += "_";
    filename += channel;
    filename += "_gmin=" + std::to_string(gmin)
                + "_gmax=" + std::to_string(gmax)
                + "_ng=" + std::to_string(ng)
                + ".h5";
    write_h5_rvecs(filename,
                   {"gs", "K1Lambda_Re", "K1Lambda_Im"},
                   {gs, K1Lambda_Re, K1Lambda_Im});

}

/* struct params_sfebb{
    double w, q;
    char i, j, r;
    bool complexity;
};

double sfebb (double Lambda, void *params) {
    struct params_sfebb *p = (struct params_sfebb *) params;
    double w = p->w;
    double q = p->q;
    char i = p->i;
    char j = p->j;
    char r = p->r;
    bool complexity = p->complexity;

    if (complexity == 0) {
        return real(sharp_frequency_exact_bare_bubble (w,Lambda,q,i,j,r));
    } else
        return imag(sharp_frequency_exact_bare_bubble (w,Lambda,q,i,j,r));
}

double dLsfebb ( double w, double Lambda, double q, char i, char j, char r, bool complexity, double h){
    double result, error;
    gsl_function F;
    F.function = &sfebb;
    struct params_sfebb params_dL = {w, q, i, j, r, complexity};
    F.params = &params_dL;
    gsl_deriv_central (&F, Lambda, h, &result, &error);
    return result;
}

comp dLsfeebb_comp( double w, double Lambda, double q, char i, char j, char r, double h){
    double realpart, imagpart;
    comp result;
    realpart = dLsfebb ( w, Lambda, q, i, j, r, 0, h);
    imagpart = dLsfebb ( w, Lambda, q, i, j, r, 1, h);
    result = realpart + glb_i*imagpart;
    return result;
}

struct params_K1r{
    double w, q;
    char ip, jp, i, j, r;
    bool complexity;
};

struct params_K1r_rhs{
    double w, q, g;
    /*char ip, jp, i, j, r;
    bool complexity;*/
//};

/*int K1r_rhs (double Lambda, const double K1[], double dK1dL[],
              void *params)
{
    double h = 1e-10;
    struct params_K1r_rhs *p = (struct params_K1r_rhs *) params;
    double w = p->w;
    double q = p->q;
    double g = p->g;
    /*char ip = p->ip;
    char jp = p->jp;
    char i = p->i;
    char j = p->j;
    char r = p->r;
    bool complexity = p->complexity;*/
/*double ReK1 = K1[0];
double ImK1 = K1[1];
comp pi_integral_rhs; //dL_pi_integral_rhs;
pi_integral_rhs = sharp_frequency_exact_bare_bubble (w, Lambda, q, 'c', 'd', 'p')+sharp_frequency_exact_bare_bubble (w, Lambda, q, 'd', 'c', 'p');
//dL_pi_integral_rhs = dLsfeebb_comp(w, Lambda, q, 'c', 'd', 'p',h)+dLsfeebb_comp(w, Lambda, q, 'd', 'c', 'p',h);
dK1dL[0] = (g*g - 2*g*ReK1+ReK1*ReK1-ImK1*ImK1)*real(pi_integral_rhs)-(-2*g*ImK1+2*ReK1*ImK1)*imag(pi_integral_rhs);
dK1dL[1] = (g*g - 2*g*ReK1+ReK1*ReK1-ImK1*ImK1)*imag(pi_integral_rhs)+(-2*g*ImK1+2*ReK1*ImK1)*real(pi_integral_rhs);
return GSL_SUCCESS;
}

int jacobian_K1r_rhs (double Lambda, const double K1[], double *dfdK1,
                  double dfdL[], void *params) {
double h = 1e-10;
struct params_K1r_rhs *p = (struct params_K1r_rhs *) params;
double w = p->w;
double q = p->q;
double g = p->g;
/*char ip = p->ip;
char jp = p->jp;
char i = p->i;
char j = p->j;
char r = p->r;
bool complexity = p->complexity;*/
/*double ReK1 = K1[0];
double ImK1 = K1[1];
gsl_matrix_view dfdy_mat
        = gsl_matrix_view_array (dfdK1, 2, 2);
gsl_matrix * m = &dfdy_mat.matrix;

comp pi_integral_rhs, dL_pi_integral_rhs;
pi_integral_rhs = sharp_frequency_exact_bare_bubble (w, Lambda, q, 'c', 'd', 'p')+sharp_frequency_exact_bare_bubble (w, Lambda, q, 'd', 'c', 'p');
dL_pi_integral_rhs = dLsfeebb_comp(w, Lambda, q, 'c', 'd', 'p',h)+dLsfeebb_comp(w, Lambda, q, 'd', 'c', 'p',h);

gsl_matrix_set (m, 0, 0, (-2*g+2*ReK1)*real(pi_integral_rhs)-2*ImK1*imag(pi_integral_rhs));
gsl_matrix_set (m, 0, 1, -2*ImK1*real(pi_integral_rhs)-(-2*g+2*ReK1)*imag(pi_integral_rhs));
gsl_matrix_set (m, 1, 0, (-2*g+2*ReK1)*imag(pi_integral_rhs)-2*ImK1*real(pi_integral_rhs));
gsl_matrix_set (m, 1, 1, -2*ImK1*imag(pi_integral_rhs)-(-2*g+2*ReK1)*real(pi_integral_rhs));
dfdL[0] = (g*g - 2*g*ReK1+ReK1*ReK1-ImK1*ImK1)*real(dL_pi_integral_rhs)-(-2*g*ImK1+2*ReK1*ImK1)*imag(dL_pi_integral_rhs);
dfdL[1] = (g*g - 2*g*ReK1+ReK1*ReK1-ImK1*ImK1)*imag(dL_pi_integral_rhs)+(-2*g*ImK1+2*ReK1*ImK1)*real(dL_pi_integral_rhs);
return GSL_SUCCESS;
}

void ODE_solver_K1p(double w, double q, double Lambdai, double Lambdaf, double g, int N) {

/*gsl_odeiv2_system sys = {ode_double_pendulum, nullptr, 4, nullptr};
gsl_odeiv2_driver *d =
        gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, 1e-6, 1e-6, 0.0);



double y[4] = {S1_ANGLE,V1_INIT,S2_ANGLE,V2_INITT}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
double t = T_START;
for (int i = 1; i <= 100; i++) {
    double ti = i * (T_END - T_START) / 100.0;
    int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
    printf("%.5e %.5e %.5e %.5e %.5e \n", t, y[0], y[1],y[2],y[3]);
}*/

/*struct params_K1r_rhs params_ode = {w, q, g};
gsl_odeiv2_system sys = {K1r_rhs, nullptr, 2, &params_ode};

gsl_odeiv2_driver * d =
        gsl_odeiv2_driver_alloc_y_new (&sys, gsl_odeiv2_step_rk8pd,
                                       1e-6, 1e-6, 0.0);
int iL;
double K1[2] = { 0.0, 0.0};
double Lambda = Lambdai;
for (iL = 0; iL < N; iL++)
{
    double Lambda_i = (N-iL) * (Lambdai - Lambdaf)/N; //(N-iL) * (Lambdai-Lambdaf) / N;
    int status;
    status = gsl_odeiv2_driver_apply(d, &Lambda, Lambda_i, K1);

    if (status != GSL_SUCCESS)
    {
        printf ("error, return value=%d\n", status);
        break;
    }

    printf ("%.5e %.5e %.5e\n", Lambda, K1[0], K1[1]);
}

gsl_odeiv2_driver_free (d);
}*/

// INTEGRATE BUBBLE IN MOMENTUM SPACE FROM GSL

struct params_theta_integration{
    double v1, v2, q, kpp;
    char i, j;
    bool complexity;
};

double bubble_integrand_theta (double x, void *params) {
    struct params_theta_integration *p = (struct params_theta_integration *) params;
    double v1 = p->v1;
    double v2 = p->v2;
    double q = p->q;
    double kpp = p->kpp;
    char i = p->i;
    char j = p->j;
    bool complexity = p->complexity;

    if (complexity == 0) {
        return real(SimpleBubble(v1, v2, q, kpp, x, i, j)) / (2 * M_PI);
    } else
        return imag(SimpleBubble(v1, v2, q, kpp, x, i, j)) / (2 * M_PI);
}

double bubble_integrate_theta (double v1, double v2, double q, double kpp, char i, char j, bool complexity, double epsabstheta){
    gsl_integration_workspace * w
            = gsl_integration_workspace_alloc (1000);

    double result, error;
    gsl_function F;
    F.function = &bubble_integrand_theta;
    struct params_theta_integration params_int = {v1, v2, q, kpp, i, j, complexity};
    F.params = &params_int;

    gsl_integration_qag (&F, -1, 1, epsabstheta, 1e-10, 1000, 2,
                         w, &result, &error);

    gsl_integration_workspace_free (w);
    return result;
}

struct params_kpp_integration{
    double v1, v2, q;
    char i, j;
    bool complexity;
    double epsabstheta;
};

double bubble_integrand_kpp (double kpp, void *params) {
    struct params_kpp_integration *p = (struct params_kpp_integration *) params;
    double v1 = p->v1;
    double v2 = p->v2;
    double q = p->q;
    char i = p->i;
    char j = p->j;
    bool complexity = p->complexity;
    double epsabstheta = p->epsabstheta;

    return kpp*kpp*bubble_integrate_theta (v1, v2, q, kpp, i, j, complexity, epsabstheta)/(2*M_PI);
}

double bubble_integrate_kpp (double v1, double v2, double q, char i, char j, bool complexity, double epsabstheta, double epsabskpp){
    gsl_integration_workspace * w
            = gsl_integration_workspace_alloc (1000);

    double result, error;
    gsl_function F;
    F.function = &bubble_integrand_kpp;
    struct params_kpp_integration params_int = {v1, v2, q, i, j, complexity, epsabstheta};
    F.params = &params_int;

    gsl_integration_qagiu (&F, 0, epsabskpp, 1e-10, 1000,
                           w, &result, &error);

    gsl_integration_workspace_free (w);
    return result;
}

comp bubble_k_2d_integrated (double w, double vpp, double q, char i, char j, char r, double epsabstheta, double epsabskpp){
    double real_part, imag_part;
    comp result;
    double prefactor, v1, v2;

    if (r == 'a') {
        prefactor = 1.;
        v1 = vpp + w/2;
        v2 = vpp - w/2;
    }
    else if (r == 'p') {
        prefactor = 0.5;
        v1 = w / 2 + vpp;
        v2 = w / 2 - vpp;
    }
    else if (r == 't') {
        prefactor = -1.;
        v1 = vpp + w / 2;
        v2 = vpp - w / 2;
    }
    else {
        prefactor = 0.;
        v1 = 0.;
        v2 = 0.;
        std::cout << "wrong channel\n";
    }

    real_part = prefactor*bubble_integrate_kpp (v1, v2, q, i, j, 0,epsabstheta,epsabskpp);
    imag_part = prefactor*bubble_integrate_kpp (v1, v2, q, i, j, 1,epsabstheta,epsabskpp);
    result = real_part + glb_i * imag_part;

    return result;
}

void print_numerical_bubble (double w, double vpp, double q, char i, char j, char r, double epsabstheta, double epsabskpp){
    comp output_value = bubble_k_2d_integrated(w,vpp,q,i,j,r, epsabstheta,epsabskpp);
    double real_output_value = real(output_value);
    double imag_output_value = imag(output_value);
    std::cout << "The numerical bubble is " << real_output_value << " + i " << imag_output_value << "\n";
}

void integral_bubble_w_vpp_list_2D (char i, char j, char channel, double wmax, double vppmax, double qmax, int nFER, int nBOS, int nq, double epsabstheta, double epsabskpp) {
    vec<double> vpps(nFER);
    vec<double> ws(nBOS);
    vec<double> qs(nq);
    vec<double> Pi_int_Re(nFER*nBOS*nq);
    vec<double> Pi_int_Im(nFER*nBOS*nq);
    comp result_integral;
    double w;
    double vpp;
    double q;

    for (int wi = 0; wi < nBOS; ++wi) {
        w = -wmax + 2*wi*wmax/(nBOS-1);
        ws[wi] = w;
        for (int vppi = 0; vppi < nFER; ++vppi) {
            vpp = -vppmax + 2*vppi*vppmax/(nFER-1);
            vpps[vppi] = vpp;
            for (int qi = 0; qi < nq; ++qi) {
                q = qi*qmax/(nq-1);
                qs[qi] = q;
                result_integral = bubble_k_2d_integrated(w,vpp,q,i,j,channel,epsabstheta,epsabskpp);
                Pi_int_Re[composite_index_wvq(wi, vppi, nFER, qi, nq)] = real(result_integral);
                Pi_int_Im[composite_index_wvq(wi, vppi, nFER, qi, nq)] = imag(result_integral);
                std::cout << "w = " << w << ", vpp = " << vpp << ", q = " << q << ", result = " << result_integral << "\n";
            }
        }
    }

    std::string filename = "../Data/integrated_bubble_2D";
    filename += "_";
    filename += i;
    filename += j;
    filename += channel;
    filename += "_nBOS=" + std::to_string(nBOS)
                + "_nFER=" + std::to_string(nFER)
                + "_nq=" + std::to_string(nq)
                + ".h5";
    write_h5_rvecs(filename,
                   {"fermionic_frequencies", "bosonic_frequencies", "bosonic_momenta", "integrated_bubble_Re", "integrated_bubble_Im"},
                   {vpps, ws, qs, Pi_int_Re, Pi_int_Im});

}

comp sharp_frequency_nint_bare_bubble ( double w, double Lambda, double q, char i, char j, char r, double epsabstheta, double epsabskpp){
    double v1, v2, v3, v4, Th1, Th2;
    comp result;

    v1 = Lambda - w/2;
    v2 = -Lambda -w/2;
    v3 = Lambda + w/2;
    v4 = -Lambda + w/2;

    Th1 = heaviside(std::abs(Lambda-w)-Lambda);
    Th2 = heaviside(std::abs(Lambda+w)-Lambda);

    result = -Th1*bubble_k_2d_integrated (w, v1, q, i, j, r, epsabstheta, epsabskpp) -Th2*bubble_k_2d_integrated (w, v2, q, i, j, r,epsabstheta, epsabskpp) -Th2*bubble_k_2d_integrated (w, v3, q, i, j, r,epsabstheta, epsabskpp) -Th1*bubble_k_2d_integrated (w, v4, q, i, j, r,epsabstheta, epsabskpp);
    return result;
}

// TEST GSL-ODE-SOLVER FOR NUMERICALLY INTEGRATED BUBBLE
/*
struct params_K1_nint{
    double w, q, g, Lambda_i, epsabstheta, epsabskpp;
    char r;
};

int K1cdcd_nint(double t, const double y[], double f[], void *params) {
    struct params_K1_nint *p = (struct params_K1_nint *) params;
    double w = p->w;
    double q = p->q;
    double g = p->g;
    double epsabstheta = p->epsabstheta;
    double epsabskpp = p->epsabskpp;
    double Lambda_i = p->Lambda_i;
    char r = p->r;

    double Lambda = Lambda_i*exp(-t);

    double ReK1 = y[0];
    double ImK1 = y[1];


    double RePicd = real(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'c', 'd', r,epsabstheta, epsabskpp));
    double ImPicd = imag(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'c', 'd', r,epsabstheta, epsabskpp));
    double RePidc = real(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'd', 'c', r,epsabstheta, epsabskpp));
    double ImPidc = imag(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'd', 'c', r,epsabstheta, epsabskpp));
    double RePicc = real(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'c', 'c', r,epsabstheta, epsabskpp));
    double ImPicc = imag(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'c', 'c', r,epsabstheta, epsabskpp));
    double RePidd = real(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'd', 'd', r,epsabstheta, epsabskpp));
    double ImPidd = imag(sharp_frequency_nint_bare_bubble ( w, Lambda, q, 'd', 'd', r,epsabstheta, epsabskpp));

    double RegK1squared = g*g - 2*g*ReK1 + ReK1*ReK1 - ImK1*ImK1;
    double ImgK1squared = -2*g*ImK1 +2*ReK1*ImK1;

    if (r == 'p') {

        f[0] = -Lambda*(RegK1squared*(RePicd+RePidc)-ImgK1squared*(ImPicd+ImPidc));
        f[1] = -Lambda*(ImgK1squared*(RePicd+RePidc)+RegK1squared*(ImPicd+ImPidc));
    }
    else if (r == 'a') {
        f[0] = -Lambda*(RegK1squared*RePidc-ImgK1squared*ImPidc);
        f[1] = -Lambda*(ImgK1squared*RePidc+RegK1squared*ImPidc);
    }
    else if (r == 't') {
        double ReK1cccc = y[2];
        double ImK1cccc = y[3];
        double ReK1dddd = y[4];
        double ImK1dddd = y[5];
        double ReK1ccccsquared =  ReK1cccc*ReK1cccc-ImK1cccc*ImK1cccc;
        double ImK1ccccsquared = 2*ReK1cccc*ImK1cccc;
        double ReK1ddddsquared =  ReK1dddd*ReK1dddd-ImK1dddd*ImK1dddd;
        double ImK1ddddsquared = 2*ReK1dddd*ImK1dddd;
        double RegK1cccc =  (-g + ReK1)*ReK1cccc - ImK1*ImK1cccc;
        double ImgK1cccc = (-g + ReK1)*ImK1cccc + ImK1*ReK1cccc;
        double RegK1dddd =  (-g + ReK1)*ReK1dddd - ImK1*ImK1dddd;
        double ImgK1dddd = (-g + ReK1)*ImK1dddd + ImK1*ReK1dddd;
        f[0] = -Lambda*(RegK1cccc*RePicc-ImgK1cccc*ImPicc+RegK1dddd*RePidd-ImgK1dddd*ImPidd);
        f[1] = -Lambda*(RegK1cccc*ImPicc+ImgK1cccc*RePicc+RegK1dddd*ImPidd+ImgK1dddd*RePidd);
        f[2] = -Lambda*(ReK1ccccsquared*RePicc-ImK1ccccsquared*ImPicc+RegK1squared*RePidd-ImgK1squared*ImPidd);
        f[3] = -Lambda*(ReK1ccccsquared*ImPicc+ImK1ccccsquared*RePicc+RegK1squared*ImPidd+ImgK1squared*RePidd);
        f[4] = -Lambda*(ReK1ddddsquared*RePidd-ImK1ddddsquared*ImPidd+RegK1squared*RePicc-ImgK1squared*ImPicc);
        f[5] = -Lambda*(ReK1ddddsquared*ImPidd+ImK1ddddsquared*RePidd+RegK1squared*ImPicc+ImgK1squared*RePicc);
    }
    else {
        f[0] = 0.0;
        f[1] = 0.0;
        std::cout << "wrong channel\n";
    }

    return GSL_SUCCESS;
}

int solve_K1cdcd_nint(double w, double q, double g, double Lambda_i, char r, double Lambda_f, double h, double epsabs, double epsrel,double epsabstheta,double epsabskpp) {
    struct params_K1_nint params_ode = {w,q,g,Lambda_i,r,epsabstheta,epsabskpp};

    if ( (r == 'p') || (r == 'a')) {
        gsl_odeiv2_system sys = {K1cdcd_nint, nullptr, 2, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);
        double y[2] = {0,0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i/Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
            double Lambda = Lambda_i*exp(-t);
            printf("%.5e %.5e %.5e \n", Lambda, y[0], y[1]);
        }
    }
    else if (r == 't'){
        gsl_odeiv2_system sys = {K1cdcd_nint, nullptr, 6, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);
        double y[6] = {0,0,0,0,0,0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i/Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
            double Lambda = Lambda_i*exp(-t);
            printf("%.5e %.5e %.5e \n", Lambda, y[0], y[1]);
        }
    }
    else {
        std::cout << "wrong channel\n";
    }

    return 0;
}

comp K1cdcd_solution_nint(double w, double q, double g, double Lambda_i, char r, double Lambda_f, double h, double epsabs, double epsrel,double epsabstheta,double epsabskpp) {
    double ReK1, ImK1;
    comp result;
    struct params_K1_nint params_ode = {w,q,g,Lambda_i,r,epsabstheta,epsabskpp};

    if ( (r == 'p') || (r == 'a')) {
        gsl_odeiv2_system sys = {K1cdcd_nint, nullptr, 2, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);

        double y[2] = {0, 0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i / Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
        }
        ReK1 = y[0];
        ImK1 = y[1];
        result = y[0]+glb_i*y[1];
    }
    else if (r == 't') {
        gsl_odeiv2_system sys = {K1cdcd_nint, nullptr, 6, &params_ode};
        gsl_odeiv2_driver *d =
                gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, h, epsabs, epsrel);

        double y[6] = {0, 0, 0, 0, 0, 0}; //y[4] = {S2_ANGLE,V1_INIT,S1_ANGLE,V2_INITT};
        double t = 0;
        for (int i = 1; i <= 100; i++) {
            double ti = i * (log(Lambda_i / Lambda_f)) / 100.0;
            int status = gsl_odeiv2_driver_apply(d, &t, ti, y);
        }
        ReK1 = y[0];
        ImK1 = y[1];
        result = y[0]+glb_i*y[1];
    }
    else {
        std::cout << "wrong channel\n";
    }

    return result;
}

void K1Lambda_nint (double w, double q, double g, char channel, double Lambda_i, double Lambda_f, int nLambda, double h, double epsabs, double epsrel, double epsabstheta, double epsabskpp) {
    vec<double> Lambdas(nLambda);
    vec<double> K1Lambda_Re(nLambda);
    vec<double> K1Lambda_Im(nLambda);
    comp result_K1;

    double Lambda;
    double t = 0;

    for (int i = 0; i < nLambda; i++) {
        t = i * (log(Lambda_i / Lambda_f)) / (nLambda-1);
        Lambda = Lambda_i*exp(-t);
        result_K1 = K1cdcd_solution_nint(w, q, g, Lambda_i, channel, Lambda, h, epsabs, epsrel, epsabstheta, epsabskpp);

        Lambdas[i] = Lambda;
        K1Lambda_Re[i] = real(result_K1);
        K1Lambda_Im[i] = imag(result_K1);

        }

    std::string filename = "../Data/K1Lambda";
    filename += "_";
    filename += channel;
    filename += "_Li=" + std::to_string(Lambda_i)
                + "_Lf=" + std::to_string(Lambda_f)
                + "_nL=" + std::to_string(nLambda)
                + ".h5";
    write_h5_rvecs(filename,
                   {"Lambdas", "K1Lambda_Re", "K1Lambda_Im"},
                   {Lambdas, K1Lambda_Re, K1Lambda_Im});

}
*/


#endif //MAIN_CPP_ZEROS_H