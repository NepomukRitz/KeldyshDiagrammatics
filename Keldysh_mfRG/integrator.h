#ifndef KELDYSH_MFRG_INTEGRATOR_H
#define KELDYSH_MFRG_INTEGRATOR_H

#include <numeric>
#include "data_structures.h"                // real and complex vectors
#include "parameters.h"                     // system parameters
#include <gsl/gsl_integration.h>            // for GSL integrator
#include <gsl/gsl_errno.h>                  // for GSL integrator
#include "Integrator_NR/integrator_NR.h"    // adaptive Gauss-Lobatto integrator with Kronrod extension
#include "util.h"                           // for rounding functions

// temporarily necessary to make use of frequency grid bfreqs
#include "frequency_grid.h"

/* compute real part of integrand (for GSL/PAID) */
template <typename Integrand>
auto f_real(double x, void* params) -> double {
    Integrand integrand = *(Integrand*) params;
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    double f_real = integrand(x).real();
#else
    double f_real = integrand(x);
#endif
    return f_real;
}

/* compute imaginary part of integrand (for GSL/PAID) */
template <typename Integrand>
auto f_imag(double x, void* params) -> double {
    Integrand integrand = *(Integrand*) params;
    double f = integrand(x).imag();
    return f;
}

/* compute the dot product of two vectors (integrand values and weights) */
auto dotproduct(const cvec& x, const rvec& y) -> comp {
    comp res;
//#pragma acc parallel loop private(i) reduction(+:resp)
    for(int i=0; i<x.size(); ++i)
        res += x[i] * y[i];
    return res;
}

//TODO implement the intergration with pragma omp simd for vetorization.
// Think about the compatibility of the execution flow with declaration of data_structures.h also with omp (simd ) and
// wether or not it'd make sense to only MPI-parallelize at bubble_function and allow omp to take over the vector or
// scalar (reduction) operations e.g. integration, vector sums, products and such

/// --- DIFFERENT INTEGRATION ROUTINES --- ///

/* Integration using Riemann sum -- deprecated, not recommended */
template <typename Integrand> auto integrator_riemann(const Integrand& integrand, int N) -> comp {
    rvec spacings(N);
    cvec integrand_values(N);
    double w0, w1, w2;

    spacings[0] = bfreqs[1] - bfreqs[0];
    integrand_values[0] = integrand(bfreqs[0]);

    for (int i=1; i<N-1; ++i) {
        w0 = bfreqs[i-1];   // now specifically relies on bfreqs, only works if bfreqs = ffreqs...
        w1 = bfreqs[i];
        w2 = bfreqs[i+1];
        spacings[i] = w2 - w0;
        integrand_values[i] = integrand(w1);
    }

    spacings[N-1] = bfreqs[N-1] - bfreqs[N-2];
    integrand_values[N-1] = integrand(bfreqs[N-1]);

    return 1/2.*dotproduct(integrand_values, spacings);
}

/**
 * Perform an integration using Simpson's rule.
 * @tparam Integrand : arbitrary integrand class, only requires a const ()-operator that returns a comp
 * @param integrand  : integrand of type Integrand
 * @param a          : lower boundary
 * @param b          : upper boundary
 * @param N          : number of integration points
 * @return           : result of the integration (comp)
 */
template <typename Integrand> auto integrator_simpson(const Integrand& integrand, double a, double b, int N) -> comp {
    if (N < 3) N = 3;
    double dx = (b-a)/((double)(N-1));
    rvec simpson(N);
    cvec integrand_values(N);

//#pragma acc parallel loop private (i)
    for (int i=0; i<N; ++i)
    {
        integrand_values[i] = integrand(a+i*dx);
        simpson[i] = 2. +2*(i%2);
    }
    simpson[0] = 1.;
    simpson[N-1]=1.;

    return dx/3.*dotproduct(integrand_values, simpson);
}

/**
 * Wrapper for integrator_simpson(const Integrand& integrand, double a, double b, int N).
 * Optimized version for loop that has a sharp features within the integration domain
 * at frequency w (typically w=0, where w is the loop external frequency).
 *   --> Split up the integration domain into different regions, integrate with higher resolution around the
 *       sharp feature.
 */
template <typename Integrand> auto integrator_simpson(const Integrand& integrand, double a, double b, double w, int N) -> comp {
    comp result = 0.;   // initialize result

    double Delta = 2*glb_Gamma;  // half-width of the region around the sharp feature which should get better resolution
    int Nw = N/5;                // number of additional points around the sharp feature
    if (!(Nw % 2)) Nw += 1;      // number of points needs to be odd


    int Na = (int)(N * (w - a) / (b - a));  // number of points in the first interval (fraction of N)
    if (!(Na % 2)) Na += 1;                 // number of points needs to be odd
    int Nb = (int)(N * (b - w) / (b - a));  // number of points in the first interval (fraction of N)
    if (!(Nb % 2)) Nb += 1;                 // number of points needs to be odd

    // Compute different contributions
    result += integrator_simpson(integrand, a, w-Delta, Na);        // interval of frequencies < w
    result += integrator_simpson(integrand, w-Delta, w+Delta, Nw);  // interval around w
    result += integrator_simpson(integrand, w+Delta, b, Nb);        // interval of frequencies > w
    return result;
}

/**
 * Wrapper for integrator_simpson(const Integrand& integrand, double a, double b, int N).
 * Optimized version for bubbles that have two distinct sharp features within the integration domain
 * at frequencies w1 and w2 (typically w1,2 = +-w/2, where w is the bubble external frequency).
 *   --> Split up the integration domain into different regions, integrate with higher resolution around the
 *       sharp features.
 */
template <typename Integrand> auto integrator_simpson(const Integrand& integrand, double a, double b, double w1_in, double w2_in, int N) -> comp {

    comp result = 0.;   // initialize result
    int Na, Nb, Nc;     // number of points in different intervals

    double Delta = 5*glb_T;  // half-width of the region around the sharp features which should get better resolution
    int Nw = N/10;           // number of additional points around each of the sharp features
    if (!(Nw % 2)) Nw += 1;  // number of points needs to be odd

    // Order the frequencies w1/w2 such that always w1 < w2.
    double w1, w2;
    if (w1_in < w2_in) {
        w1 = w1_in; w2 = w2_in;
    }
    else {
        w1 = w2_in; w2 = w1_in;
    }

    // First case: w1/w2 are away from boundary (assume that b-w2 == w1-a)
    if (b-w2 > Delta) {
        // First compute contributions of intervals [a,w1-Delta] and [w2+Delta,b]
        Na = (int)(N * (w1 - a) / (b - a));  // number of points in the first interval (fraction of N)
        if (!(Na % 2)) Na += 1;              // number of points needs to be odd
        Nb = (int)(N * (b - w2) / (b - a));  // number of points in the last interval
        if (!(Nb % 2)) Nb += 1;              // number of points needs to be odd

        result += integrator_simpson(integrand, a, w1-Delta, Na);
        result += integrator_simpson(integrand, w2+Delta, b, Nb);

        // Check if w1 and w2 are too close to consider them separately
        if (w2-w1 > 10*Delta) {   // w1 and w2 are far enough away from each other to consider separate regions around them
            Nc = (int)(N * (w2 - w1)/(b - a)); // number of points in the central interval between w1/w2
            if (!(Nc % 2)) Nc += 1;            // number of points needs to be odd

            // Compute contributions of the intervals around w1/w2 and between them
            result += integrator_simpson(integrand, w1-Delta, w1+Delta, Nw);
            result += integrator_simpson(integrand, w1+Delta, w2-Delta, Nc);
            result += integrator_simpson(integrand, w2-Delta, w2+Delta, Nw);
        }
        else { // w1/w2 are close to each other only consider single contribution
            Nc = (int)(Nw * (w2-w1+2*Delta)/(2*Delta)); // number of points in the interval enclosing w1/w2
            // (depends on the distance between w1/w2)
            if (!(Nc % 2)) Nc += 1;                     // number of points needs to be odd

            result += integrator_simpson(integrand, w1-Delta, w2+Delta, Nc);
        }
    }

    // Second case: w1/w2 are close to boundaries a/b // currently not necessary since w1/2 = +-w/2
    else {
        result += integrator_simpson(integrand, a, w1+Delta, Nw);       // interval around w1/a
        result += integrator_simpson(integrand, w1+Delta, w2-Delta, N); // interval between w1/w2
        result += integrator_simpson(integrand, w2-Delta, b, Nw);       // interval around w2/b
    }

    return result;
}


// compute Simpson rule for 3 given values and given step size dx
auto simpson_rule_3(comp& val0, comp& val1, comp& val2, double& dx) -> comp {
    return dx / 3. * (val0 + 4.*val1 + val2);
}

// compute Simpson rule for vector of 3 given values and given step size dx
auto simpson_rule_3(cvec& values, double& dx) -> comp {
    return dx / 3. * (values[0] + 4.*values[1] + values[2]);
}

/**
 * Adaptive Simpson integrator.
 * Idea: Start with just 3-point Simpson (integration boundaries + point in the center), then split it up into
 * two sub-intervals by adding one point in the center of each of the two sub-intervals.
 * Then split those up again to obtain four sub-intervals, then eight, etc. For each sub-interval check convergence
 * separately by comparing to the previous result, split it up only if desired accuracy is not yet reached.
 * Continue successive splitting until either
 *  - all sub-intervals of the preceding step are converged, or
 *  - desired accuracy for the total result is reached, or
 *  - max. number of allowed points (integrand accesses) is reached.
 * During this iterative process, results from the previous step are stored to minimize the number of integrand accesses.
 */
template <typename Integrand> auto adaptive_simpson_integrator(const Integrand& integrand, double a, double b, int Nmax) -> comp {
    // accuracy: relative error between successive results for the same interval
    // at which to stop splitting the interval into sub-intervals
    double accuracy = 1e-3; //1e-3;
    // total_accuracy: relative error between successive results for the full integral
    // at which to stop the integration procedure and return result
    double total_accuracy = 1e-4; //1e-9;

    /// --- initial step: start with 3-point Simpson --- ///

    // Create vectors containing information about each sub-interval. Initially: only one sub-interval.
    rvec points (3);             // vector containing all grid points at which integrand is evaluated
    cvec values (3);             // vector containing the integrand values for each grid point
    cvec result (1);             // vector containing the integration result for each sub-interval
    vec<bool> converged {false}; // vector of bools containing the information if each sub-interval is converged

    double dx = (b-a)/2.;                    // initial step size: half the full interval
    for (int i=0; i<3; ++i) {
        points[i] = a+i*dx;                  // choose 3 equidistant grid points
        values[i] = integrand(points[i]);    // get the integrand value at those grid points
    }
    result[0] = simpson_rule_3(values[0], values[1], values[2], dx); // compute the integral using 3-point Simpson
    comp res = result[0];                                            // initialize result

    /// --- loop: successively split intervals into two sub-intervals until converged --- ///

    for (int it=0; true; ++it) {    // Infinite loop: only quit by reaching convergence or max. number of evaluations.
        dx /= 2.;                   // in each step, halve the step size

        rvec points_next;           // new vector for grid points
        cvec values_next;           // new vector for integrand values
        cvec result_next;           // new vector for results of sub-intervals
        vec<bool> converged_next;   // new vector for convergence flags of sub-intervals

        // add the left boundary and the corresponding integrand value to the new vectors
        points_next.push_back(points[0]);
        values_next.push_back(values[0]);

        for (int interval=0; interval<result.size(); ++interval) { // Go through all existing sub-intervals:
            if (converged[interval] && it>3) {                     // If the interval is converged, don't do anything
                                                                   // (only after having performed the first few splits)
                // add existing points, values, ... for this interval
                // to the new list
                points_next.push_back(points[2*interval+1]);
                points_next.push_back(points[2*interval+2]);
                values_next.push_back(values[2*interval+1]);
                values_next.push_back(values[2*interval+2]);

                result_next.push_back(result[interval]);
                converged_next.push_back(true);
            }
            else {                          // If the interval is not converged, split it up into 2 new sub-intervals:

                // add center and right points for first sub-interval (left known from previous interval)
                points_next.push_back(points[2*interval] + dx);
                points_next.push_back(points[2*interval+1]);
                // add integrand values for first sub-interval
                values_next.push_back(integrand(points_next.end()[-2]));
                values_next.push_back(values[2*interval+1]);

                // compute integral over the first sub-interval
                auto val = values_next.end();
                comp result1 = simpson_rule_3(val[-3], val[-2], val[-1], dx);

                // add center and right points for second sub-interval (left known from first sub-interval)
                points_next.push_back(points[2*interval+1] + dx);
                points_next.push_back(points[2*interval+2]);
                // add integrand values for second sub-interval
                values_next.push_back(integrand(points_next.end()[-2]));
                values_next.push_back(values[2*interval+2]);

                // compute integral over the second sub-interval
                val = values_next.end();
                comp result2 = simpson_rule_3(val[-3], val[-2], val[-1], dx);

                // add the results to the vector of all intervals, for later checking convergence
                result_next.push_back(result1);
                result_next.push_back(result2);

                // Check convergence: if difference between the sum of the results of the 2 sub-intervals and the
                // previous result only has a relative error smaller than predefined accuracy,
                // set convergence flag for both sub-intervals to true, such that they are not split in the next iteration

                //if ((abs(real(result1+result2-result[interval])/real(result1+result2+1e-16)) < accuracy)
                //    && (abs(imag(result1+result2-result[interval])/imag(result1+result2+glb_i*1e-16)) < accuracy)) {   // add 1e-16 to avoid errors if result is zero
                if (abs((result1+result2-result[interval])/(result1+result2+1e-16)) < accuracy) {   // add 1e-16 to avoid errors if result is zero
                    converged_next.push_back(true);
                    converged_next.push_back(true);
                }
                else {
                    converged_next.push_back(false);
                    converged_next.push_back(false);
                }
            }
        }

        // compute the total result of the integration after the current splitting step
        comp res_next = accumulate(result_next.begin(), result_next.end(), (comp)0.);

        // First stop condition: check convergence of the total result w.r.t. predefined accuracy

        //if ((abs(real(res-res_next)/real(res+1e-16)) < total_accuracy) && (abs(imag(res-res_next)/imag(res+glb_i*1e-16)) < total_accuracy) && it>3) {
        if (abs((res-res_next)/res) < total_accuracy) {
            res = res_next;
            break;
        }

        res = res_next; // update the total result for convergence check in the next iteration

        // update points, values, results, converged flags
        points = points_next;
        values = values_next;
        result = result_next;
        converged = converged_next;

        // Second stop condition: check if all sub-intervals are converged
        if (count(converged.begin(), converged.end(), true) == converged.size()) break;

        // Third stop condition: check if the total number of evaluation points exceeds the predefined maximum
        if (points.size() > Nmax) break;
    }

    return res;  // return the result
}

/* error handler for GSL integrator */
void handler (const char * reason,
              const char * file,
              int line,
              int gsl_errno) {
    //print(reason, true);
}

/* Integration using routines from the GSL library (many different routines available, would need more testing) */
// TODO: code does currently not compile when this integrator is used!
template <typename Q, typename Integrand> auto integrator_gsl(Integrand& integrand, double a, double b, double w1_in, double w2_in, int Nmax) -> Q {
    gsl_integration_workspace* W_real = gsl_integration_workspace_alloc(Nmax);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_workspace* W_imag = gsl_integration_workspace_alloc(Nmax);
#endif

    //gsl_integration_cquad_workspace* W_real = gsl_integration_cquad_workspace_alloc(Nmax);
    //gsl_integration_cquad_workspace* W_imag = gsl_integration_cquad_workspace_alloc(Nmax);

    gsl_function F_real;
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_function F_imag;
#endif

    F_real.function = &f_real<Integrand>;
    F_real.params = &integrand;

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    F_imag.function = &f_imag<Integrand>;
    F_imag.params = &integrand;
#endif

    double result_real, error_real;
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    double result_imag, error_imag;
#endif

    gsl_set_error_handler(handler);

    gsl_integration_qag(&F_real, a, b, 0, integrator_tol, Nmax, 1, W_real, &result_real, &error_real);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_qag(&F_imag, a, b, 0, integrator_tol, Nmax, 1, W_imag, &result_imag, &error_imag);
#endif

    //double w1, w2;
    //if (w1_in < w2_in) {
    //    w1 = w1_in; w2 = w2_in;
    //}
    //else {
    //    w1 = w2_in; w2 = w1_in;
    //}
    //
    //double pts[4] = {a, w1, w2, b};
    //int npts = 4;
    //
    //gsl_integration_qagp(&F_real, pts, npts, 1e-4, 1e-4, Nmax, W_real, &result_real, &error_real);
    //gsl_integration_qagp(&F_imag, pts, npts, 1e-4, 1e-4, Nmax, W_imag, &result_imag, &error_imag);

    //gsl_integration_qagi(&F_real, 0, 1e-4, Nmax, W_real, &result_real, &error_real);
    //gsl_integration_qagi(&F_imag, 0, 1e-4, Nmax, W_imag, &result_imag, &error_imag);

    //size_t neval = Nmax;
    //gsl_integration_cquad(&F_real, a, b, 1e-4, 1e-4, W_real, &result_real, &result_imag, &neval);

    gsl_integration_workspace_free(W_real);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_workspace_free(W_imag);
#endif

    //gsl_integration_cquad_workspace_free(W_real);
    //gsl_integration_cquad_workspace_free(W_imag);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    return result_real + glb_i*result_imag;
#else
    return result_real;
#endif
}

/* Integration using routines from the GSL library (many different routines available, would need more testing) */
//
template <typename Q, typename Integrand> auto integrator_gsl(Integrand& integrand, vec<vec<double>> intervals, size_t num_intervals, int Nmax) -> Q {
    //gsl_integration_cquad_workspace* W_real = gsl_integration_cquad_workspace_alloc(Nmax);
    //gsl_integration_cquad_workspace* W_imag = gsl_integration_cquad_workspace_alloc(Nmax);
    gsl_integration_workspace* W_real = gsl_integration_workspace_alloc(Nmax);
    gsl_function F_real;
    F_real.function = &f_real<Integrand>;
    F_real.params = &integrand;
    double result_real {}, error_real {};

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_workspace* W_imag = gsl_integration_workspace_alloc(Nmax);
    gsl_function F_imag;
    F_imag.function = &f_imag<Integrand>;
    F_imag.params = &integrand;
    double result_imag, error_imag;
#endif

    gsl_set_error_handler(handler);

    //gsl_integration_qag(&F_real, a, b, 0, integrator_tol, Nmax, 1, W_real, &result_real, &error_real);
    gsl_integration_qagil(&F_real, intervals[0][0], 0, integrator_tol, Nmax, W_real, &result_real, &error_real);
    double result_real_temp{}, error_real_temp{};
    gsl_integration_qagiu(&F_real, intervals[num_intervals-1][1], 0, integrator_tol, Nmax, W_real, &result_real_temp, &error_real_temp);
    result_real += result_real_temp;
    error_real += error_real_temp;
    for (int i = 0; i < num_intervals; i++){
        result_real_temp = 0.;
        error_real_temp = 0.;
        if (intervals[i][0] < intervals[i][1]) gsl_integration_qag(&F_real, intervals[i][0], intervals[i][1], 10e-8, integrator_tol, Nmax, 1, W_real, &result_real_temp, &error_real_temp);
        result_real += result_real_temp;
        error_real += error_real_temp;
    }

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_qagil(&F_imag, intervals[0][0], 0, integrator_tol, Nmax, W_imag, &result_imag, &error_imag);
    double result_imag_temp, error_imag_temp;
    gsl_integration_qagiu(&F_imag, intervals[num_intervals-1][1], 0, integrator_tol, Nmax, W_imag, &result_imag_temp, &error_imag_temp);
    result_imag += result_imag_temp;
    error_imag += error_imag_temp;
    for (int i = 0; i < num_intervals; i++){
        result_imag_temp = 0.;
        error_imag_temp = 0.;
        gsl_integration_qag(&F_imag, intervals[i][0], intervals[i][1], 0, integrator_tol, Nmax, 1, W_imag, &result_imag_temp, &error_imag_temp);
        result_imag += result_imag_temp;
        error_imag += error_imag_temp;
    }
#endif

    //double w1, w2;
    //if (w1_in < w2_in) {
    //    w1 = w1_in; w2 = w2_in;
    //}
    //else {
    //    w1 = w2_in; w2 = w1_in;
    //}
    //
    //double pts[4] = {a, w1, w2, b};
    //int npts = 4;
    //
    //gsl_integration_qagp(&F_real, pts, npts, 1e-4, 1e-4, Nmax, W_real, &result_real, &error_real);
    //gsl_integration_qagp(&F_imag, pts, npts, 1e-4, 1e-4, Nmax, W_imag, &result_imag, &error_imag);

    //gsl_integration_qagi(&F_real, 0, 1e-4, Nmax, W_real, &result_real, &error_real);
    //gsl_integration_qagi(&F_imag, 0, 1e-4, Nmax, W_imag, &result_imag, &error_imag);

    //size_t neval = Nmax;
    //gsl_integration_cquad(&F_real, a, b, 1e-4, 1e-4, W_real, &result_real, &result_imag, &neval);

    gsl_integration_workspace_free(W_real);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    gsl_integration_workspace_free(W_imag);
#endif

    //gsl_integration_cquad_workspace_free(W_real);
    //gsl_integration_cquad_workspace_free(W_imag);
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
    return result_real + glb_i*result_imag;
#else
    return result_real;
#endif
}



/* Integration using the PAID algorithm (not yet properly implemented) */
template <typename Integrand> auto integrator_PAID(Integrand& integrand, double a, double b) -> comp {
    //PAID
    //TODO Solve issue with the first vertex not being passed completely
//    Domain1D<comp> D (grid[0], grid[grid.size()-1]);
//    vector<PAIDInput> inputs;
//    inputs.emplace_back(D, integrand, 1);
//    PAID p(inputs);
//    auto result = p.solve();
//
//    return result[1];
}



/// --- WRAPPER FUNCTIONS: INTERFACE FOR ACCESSING THE INTEGRATOR IN BUBBLES/LOOP --- ///

// old wrapper function
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b) -> Q {
#if INTEGRATOR_TYPE == 0 // Riemann sum
    return integrator_riemann(integrand, nINT);
#elif INTEGRATOR_TYPE == 1 // Simpson
    return integrator_simpson(integrand, a, b, nINT);
#elif INTEGRATOR_TYPE == 2 // Simpson + additional points
    return integrator_simpson(integrand, a, b, 0., nINT);      // use standard Simpson plus additional points around w = 0
#elif INTEGRATOR_TYPE == 3 // adaptive Simpson
    return adaptive_simpson_integrator(integrand, a, b, nINT);          // use adaptive Simpson integrator
#elif INTEGRATOR_TYPE == 4 // GSL
    return integrator_gsl<Q>(integrand, a, b, 0., 0., nINT);
#elif INTEGRATOR_TYPE == 5 // adaptive Gauss-Lobatto with Kronrod extension
    Adapt<Q, Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a, b);
#endif
}

// wrapper function, used for loop
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w) -> Q {
#if INTEGRATOR_TYPE == 0 // Riemann sum
    return integrator_riemann(integrand, nINT);
#elif INTEGRATOR_TYPE == 1 // Simpson
    return integrator_simpson(integrand, a, b, nINT);       // only use standard Simpson
#elif INTEGRATOR_TYPE == 2 // Simpson + additional points
    return integrator_simpson(integrand, a, b, w, nINT);      // use standard Simpson plus additional points around w = 0
#elif INTEGRATOR_TYPE == 3 // adaptive Simpson
    return adaptive_simpson_integrator(integrand, a, b, nINT);      // use adaptive Simpson integrator
#elif INTEGRATOR_TYPE == 4 // GSL
    return integrator_gsl<Q>(integrand, a, b, w, w, nINT);
#elif INTEGRATOR_TYPE == 5 // adaptive Gauss-Lobatto with Kronrod extension
    Adapt<Q, Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a, b);
#endif
}

/**
 * wrapper function, used for bubbles.
 * @param integrand
 * @param a         :   lower limit for integration
 * @param b         :   upper limit for integration
 */
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w1, double w2) -> Q {
#if INTEGRATOR_TYPE == 0 // Riemann sum
    return integrator_riemann(integrand, nINT);
#elif INTEGRATOR_TYPE == 1 // Simpson
    return integrator_simpson(integrand, a, b, nINT);           // only use standard Simpson
#elif INTEGRATOR_TYPE == 2 // Simpson + additional points
    return integrator_simpson(integrand, a, b, w1, w2, nINT);     // use standard Simpson plus additional points around +- w/2
#elif INTEGRATOR_TYPE == 3 // adaptive Simpson
    return adaptive_simpson_integrator(integrand, a, b, nINT);          // use adaptive Simpson integrator
#elif INTEGRATOR_TYPE == 4 // GSL
    return integrator_gsl<Q>(integrand, a, b, w1, w2, nINT);
#elif INTEGRATOR_TYPE == 5 // adaptive Gauss-Lobatto with Kronrod extension
    Adapt<Q, Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a,b);
#endif
}

/**
 * Wrapper function for bubbles and loops, splitting the integration domain along difficult features.
 * ATTENTION: splitting only done for INTEGRATOR_TYPE == 5.
 * @param a      : lower limit for integration
 * @param b      : upper limit for integration
 * @param w1     : first frequency where features occur
 * @param w2     : second frequency where features occur
 * @param Delta  : with of window around the features which should be integrated separately (to be set by hybridization strength)
 */
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w1, double w2, double Delta) -> comp {
#if INTEGRATOR_TYPE == 0 // Riemann sum
    return integrator_riemann(integrand, nINT);
#elif INTEGRATOR_TYPE == 1 // Simpson
    return integrator_simpson(integrand, a, b, nINT);           // only use standard Simpson
#elif INTEGRATOR_TYPE == 2 // Simpson + additional points
    return integrator_simpson(integrand, a, b, w1, w2, nINT);     // use standard Simpson plus additional points around +- w/2
#elif INTEGRATOR_TYPE == 3 // adaptive Simpson
    return adaptive_simpson_integrator(integrand, a, b, nINT);          // use adaptive Simpson integrator
#elif INTEGRATOR_TYPE == 4 // GSL
    return integrator_gsl<Q>(integrand, a, b, w1, w2, nINT);
#elif INTEGRATOR_TYPE == 5 // adaptive Gauss-Lobatto with Kronrod extension
    // define points at which to split the integrals (including lower and upper integration limits)
    rvec intersections {a, w1-Delta, w1+Delta, w2-Delta, w2+Delta, b};
    std::sort(intersections.begin(), intersections.end()); // sort the intersection points to get correct intervals

    comp result = 0.; // initialize results
    // integrate intervals of with 2*Delta around the features at w1, w2
    Adapt<Q, Integrand> adaptor_peaks(integrator_tol, integrand);
    result += adaptor_peaks.integrate(intersections[1], intersections[2]);
    result += adaptor_peaks.integrate(intersections[3], intersections[4]);

    // integrate the tails and the interval between the features, with increased tolerance
    Adapt<Q, Integrand> adaptor_tails(integrator_tol*10, integrand);
    result += adaptor_tails.integrate(intersections[0], intersections[1]);
    result += adaptor_tails.integrate(intersections[2], intersections[3]);
    result += adaptor_tails.integrate(intersections[4], intersections[5]);

    return result;
#endif
}

/**
 * wrapper function, used for bubbles.
 * @param integrand
 * @param intervals         :   list of intervals (lower and upper limit for integrations)
 * @param num_intervals     :   number of intervals
 */
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, vec<vec<double>>& intervals, const size_t num_intervals) -> Q {
#if INTEGRATOR_TYPE == 0 // Riemann sum
    Q result;
    for (int i = 0; i < num_intervals; i++){
        result += integrator_riemann(integrand, nINT);
    }
    return result;
#elif INTEGRATOR_TYPE == 1 // Simpson
    Q result;
    for (int i = 0; i < num_intervals; i++){
        result += integrator_simpson(integrand, intervals[i][0], intervals[i][1], nINT);       // only use standard Simpson
    }
    return result;
#elif INTEGRATOR_TYPE == 2 // Simpson + additional points
    Q result;
    for (int i = 0; i < num_intervals; i++){
        result += integrator_simpson(integrand, intervals[i][0], intervals[i][1], w1, w2, nINT);        // use standard Simpson plus additional points around +- w/2
    }
    return result;
#elif INTEGRATOR_TYPE == 3 // adaptive Simpson
    Q result;
    for (int i = 0; i < num_intervals; i++){
        result += adaptive_simpson_integrator(integrand, intervals[i][0], intervals[i][1], nINT);       // use adaptive Simpson integrator
    }
    return result;
#elif INTEGRATOR_TYPE == 4 // GSL
    return integrator_gsl<Q>(integrand, intervals, num_intervals, nINT);
#elif INTEGRATOR_TYPE == 5 // adaptive Gauss-Lobatto with Kronrod extension
    Adapt<Q, Integrand> adaptor(integrator_tol, integrand);
    vec<Q> result = vec<Q>(num_intervals);
    for (int i = 0; i < num_intervals; i++){
        result[i] = adaptor.integrate(intervals[i][0], intervals[i][1]);
    }
    return result.sum();
#endif
}
#if not defined(KELDYSH_FORMALISM) and defined(ZERO_TEMP)
/**
 * wrapper function, used for bubbles. Splits up integration interval in suitable pieces for Matsubara T=0
 * @param integrand
 * @param intervals         :   list of intervals (lower and upper limit for integrations)
 * @param num_intervals     :   number of intervals
 */
template <typename Q, int num_freqs, typename Integrand> auto integrator(Integrand& integrand, const double vmin, const double vmax, double w_half, const vec<double>& freqs, const double Delta) -> Q {
    double tol = inter_tol;

    // Doesn't work yet (errors accumulate with the current implementation)
    // The idea is to split up the interval and thereby make sure that the integrator recognizes all the relevant features of the integrand.
    vec<double> intersections;
    size_t num_intervals;
    if (w_half < tol) {
        w_half = 0.;
        intersections = {w_half, vmin, vmax};
        num_intervals = num_freqs*2 + 2;
    }
    else {
        intersections = {-w_half, w_half, vmin, vmax};
        num_intervals = num_freqs*2 + 3;
    }

    for (int i = 0; i<num_freqs; i++){
        for (int sign1:{-1,1}) {
            //for (int sign2:{-1,1}) {
            //    intersections.push_back(sign1 * freqs[i] + sign2 * Delta);
            //}
            intersections.push_back(sign1 * freqs[i]);
        }
    }

    std::sort(intersections.begin(), intersections.end());

    vec<vec<double>> intervals(num_freqs*2 + 3, {0.,0.});
    for (int i = 0; i < num_intervals; i++) {
        if (intersections[i] != intersections[i+1]) {
            intervals[i] = {intersections[i], intersections[i + 1]};
            if (abs(abs(intersections[i]) - w_half) < tol) {
                intervals[i][0] += tol;
                intervals[i - 1][1] -= tol;
            }
        }
    }


/*
    size_t num_intervals;
    vec<vec<double>> intervals;
    if( -w_half+tol < w_half-tol){
        intervals = {{vmin, -w_half-tol}, {-w_half+tol, w_half-tol}, {w_half+tol, vmax}};
        num_intervals = 3;
    }
    else {
        intervals = {{vmin, -w_half-tol}, {w_half+tol, vmax}};
        num_intervals = 2;
    }
*/

    return integrator<Q>(integrand, intervals, num_intervals);

}
#endif


template <typename Q, typename Integrand> auto matsubarasum(const Integrand& integrand, const int Nmin, const int Nmax, const int N_tresh = 60,
        int balance_fac = 2, double reltol = 1e-5, double abstol = 1e-7) -> Q {
    double freq_step = (2 * M_PI * glb_T);
    int N = Nmax - Nmin  + 1;

    //// Straightforward summation:
    //vec<Q> values(N);
    //double vpp;
    //for (int i = 0; i < N; i++) {
    //    vpp = vmin + i * (2 * M_PI * glb_T);
    //    values[i] = integrand(vpp);
    //}
    //return values.sum();

    //// Adaptive summator:
    if (N_tresh * balance_fac >= N) {

        //cout << "Direct SUMMATION on interval[" << Nmin << ", " << Nmax << "] of length " << N <<  "!!! \n";
        vec<Q> values(N);
        double vpp;
        for (int i = 0; i < N; i++) {
            vpp = (2*Nmin+1) * (M_PI * glb_T) + i * freq_step;
            values[i] = integrand(vpp);
        }
        return values.sum();
    }
    else {
        //cout << "Adapt on interval[" << vmin << ", " << vmax << "] of length " << N <<  "!!! \n";
        vec<Q> values(N_tresh);
        vec<Q> mfreqs(N_tresh);
        int intstep = (Nmax - Nmin) / (N_tresh-1);
        for (int i = 0; i < N_tresh-1; i++) {
            mfreqs[i] = ((Nmin + intstep * i ) * 2 + 1) * (M_PI * glb_T);
            values[i] = integrand(mfreqs[i]);
        }
        mfreqs[N_tresh-1] = (Nmax * 2 + 1) * (M_PI * glb_T);
        values[N_tresh-1] = integrand(mfreqs[N_tresh-1]);

        Q slope = (values[N_tresh-1] - values[0]) / (mfreqs[N_tresh-1] - mfreqs[0]);
        vec<Q> linrzd = values[0] + slope * (mfreqs - mfreqs[0]);
        vec<Q> intermediate = (linrzd - values).abs();
        Q error_estimate = (values[0] + slope * (mfreqs - mfreqs[0]) - values).abs().sum();
        //double vmaxn = vmax/(M_PI*glb_T);
        //double vminn = vmin/(M_PI*glb_T);
        Q resul_estimate = (values[0] - slope * freq_step/2) * N + freq_step*slope / 2 * N * N;
        //cout << "error_estimate = " << error_estimate << "\t < tol * sum = " << reltol * values.abs().sum() << " ? \n";
        if (error_estimate < reltol * abs(values.abs().sum()) or error_estimate < abstol) {
            //cout << "Accepted estimate! \n";
            return resul_estimate;
        }
        else {
            //cout << "Recursion step \n";
            vec<Q> result(N_tresh-1);
            //cout << "Recursion step with error " << error_estimate << "\t on the interval[" << Nmin << ", " << Nmax << "] of length " << N << "\n";
            for (int i = 0; i < N_tresh - 2; i++) {
                result[i] = matsubarasum<Q>(integrand, Nmin + i*intstep, Nmin + (i+1)*intstep - 1, N_tresh, balance_fac, reltol, abstol);
            }
            result[N_tresh-2] = matsubarasum<Q>(integrand, Nmin + (N_tresh - 2)*intstep, Nmax, N_tresh, balance_fac, reltol, abstol);
            return result.sum();
        }


    }
}

#endif //KELDYSH_MFRG_INTEGRATOR_H
