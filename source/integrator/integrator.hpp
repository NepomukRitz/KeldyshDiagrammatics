#ifndef KELDYSH_MFRG_INTEGRATOR_HPP
#define KELDYSH_MFRG_INTEGRATOR_HPP

#include <numeric>
#include <type_traits>
#include "../data_structures.hpp"                 // real and complex vectors
#include "../parameters/master_parameters.hpp"                      // system parameters
#include <gsl/gsl_integration.h>                // for GSL integrator
#include <gsl/gsl_errno.h>                      // for GSL integrator
#include "old_integrators.hpp"                    // Riemann, Simpson, PAID integrator (should not needed)
#include "integrator_NR.hpp"                      // adaptive Gauss-Lobatto integrator with Kronrod extension
#include "../utilities/util.hpp"                  // for rounding functions

/* compute real part of integrand (for GSL/PAID) */
template <typename Integrand>
auto f_real(double x, void* params) -> double {
    Integrand integrand = *(Integrand*) params;
    double f_real = myreal(integrand(x));

    return f_real;
}

/* compute imaginary part of integrand (for GSL/PAID) */
template <typename Integrand>
auto f_imag(double x, void* params) -> double {
    Integrand integrand = *(Integrand*) params;
    double f = myimag(integrand(x));
    return f;
}

/* error handler for GSL integrator */
void handler (const char * reason,
              const char * file,
              int line,
              int gsl_errno);

auto integrator_gsl_qag_helper(gsl_function& F, double a, double b, int Nmax) -> double;

auto integrator_gsl_qagp_helper(gsl_function& F, double* pts, size_t npts, int Nmax) -> double;


auto integrator_gsl_qagiu_helper(gsl_function& F, double a, int Nmax) -> double;

auto integrator_gsl_qagil_helper(gsl_function& F, double a, int Nmax) -> double;



/* Integration using routines from the GSL library (many different routines available, would need more testing) */
template <typename Q, typename Integrand> auto integrator_gsl_qag_tails(Integrand& integrand, double a, double b, int Nmax) -> Q {
    if constexpr (std::is_same<Q,comp>::value){

        gsl_function F_real;
        gsl_function F_imag;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        F_imag.function = &f_imag<Integrand>;
        F_imag.params = &integrand;

        double result_real = 0.;
        double result_imag = 0.;


        result_real += integrator_gsl_qagil_helper(F_real, a, Nmax);
        result_real += integrator_gsl_qagiu_helper(F_real, b, Nmax);
        result_imag += integrator_gsl_qagil_helper(F_imag, a, Nmax);
        result_imag += integrator_gsl_qagiu_helper(F_imag, b, Nmax);


        return result_real + glb_i*result_imag;
    }
    else{
        gsl_function F_real;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;

        double result_real = 0.;

            result_real += integrator_gsl_qagil_helper(F_real, a, Nmax);
            result_real += integrator_gsl_qagiu_helper(F_real, b, Nmax);


        return result_real;
    }
}

/* Integration using routines from the GSL library (many different routines available, would need more testing) */
template <typename Q, typename Integrand> auto integrator_gsl_qag_v2(Integrand& integrand, double a, double b, int Nmax, const bool isinf) -> Q {
    if constexpr (std::is_same<Q,comp>::value){

        gsl_function F_real;
        gsl_function F_imag;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        F_imag.function = &f_imag<Integrand>;
        F_imag.params = &integrand;

        double result_real = integrator_gsl_qag_helper(F_real, a, b, Nmax);
        double result_imag = integrator_gsl_qag_helper(F_imag, a, b, Nmax);

        if (isinf) {
            result_real += integrator_gsl_qagil_helper(F_real, a, Nmax);
            result_real += integrator_gsl_qagiu_helper(F_real, b, Nmax);
            result_imag += integrator_gsl_qagil_helper(F_imag, a, Nmax);
            result_imag += integrator_gsl_qagiu_helper(F_imag, b, Nmax);
        }

        return result_real + glb_i*result_imag;
    }
    else{
        gsl_function F_real;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;

        double result_real = integrator_gsl_qag_helper(F_real, a, b, Nmax);

        if (isinf) {
            result_real += integrator_gsl_qagil_helper(F_real, a, Nmax);
            result_real += integrator_gsl_qagiu_helper(F_real, b, Nmax);
        }

        return result_real;
    }
}

/* Integration using routines from the GSL library (many different routines available, would need more testing) */
template <typename Q, typename Integrand> auto integrator_gsl_qagp_v2(Integrand& integrand, double* pts, int npts, int Nmax, const bool isinf) -> Q {
    if constexpr (std::is_same<Q,comp>::value){

        gsl_function F_real;
        gsl_function F_imag;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        F_imag.function = &f_imag<Integrand>;
        F_imag.params = &integrand;

        double result_real = integrator_gsl_qagp_helper(F_real, pts, npts, Nmax);
        double result_imag = integrator_gsl_qagp_helper(F_imag, pts, npts, Nmax);


        if (isinf) {
            result_real += integrator_gsl_qagil_helper(F_real, pts[0]       , Nmax);
            result_real += integrator_gsl_qagiu_helper(F_real, pts[npts - 1], Nmax);
            result_imag += integrator_gsl_qagil_helper(F_imag, pts[0]       , Nmax);
            result_imag += integrator_gsl_qagiu_helper(F_imag, pts[npts - 1], Nmax);
        }

        return result_real + glb_i*result_imag;
    }
    else{
        gsl_function F_real;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;

        double result_real = integrator_gsl_qagp_helper(F_real, pts, npts, Nmax);


        if (isinf) {
            result_real += integrator_gsl_qagil_helper(F_real, pts[0]       , Nmax);
            result_real += integrator_gsl_qagiu_helper(F_real, pts[npts - 1], Nmax);
        }

        return result_real;
    }
}


/* Integration using routines from the GSL library (many different routines available, would need more testing) */
template <typename Q, typename Integrand> auto integrator_gsl(Integrand& integrand, double a, double b, double w1_in, double w2_in, int Nmax) -> Q {
    if constexpr (std::is_same<Q, std::complex<double>>::value){
        gsl_integration_workspace* W_real = gsl_integration_workspace_alloc(Nmax);
        gsl_integration_workspace* W_imag = gsl_integration_workspace_alloc(Nmax);

        gsl_function F_real;
        gsl_function F_imag;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        F_imag.function = &f_imag<Integrand>;
        F_imag.params = &integrand;

        double result_real, error_real;
        double result_imag, error_imag;

        gsl_set_error_handler(handler);

        gsl_integration_qag(&F_real, a, b, 0, integrator_tol, Nmax, 1, W_real, &result_real, &error_real);
        gsl_integration_qag(&F_imag, a, b, 0, integrator_tol, Nmax, 1, W_imag, &result_imag, &error_imag);

        gsl_integration_workspace_free(W_real);
        gsl_integration_workspace_free(W_imag);

        return result_real + glb_i*result_imag;
    }
    else{
        gsl_integration_workspace* W_real = gsl_integration_workspace_alloc(Nmax);

        gsl_function F_real;

        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;

        double result_real, error_real;

        gsl_set_error_handler(handler);

        gsl_integration_qag(&F_real, a, b, 0, integrator_tol, Nmax, 1, W_real, &result_real, &error_real);

        gsl_integration_workspace_free(W_real);

        return result_real;
    }
}

/* Integration using routines from the GSL library (many different routines available, would need more testing) */
//
template <typename Q, typename Integrand> auto integrator_gsl(Integrand& integrand, const vec<vec<double>>& intervals, const size_t num_intervals, const int Nmax, const bool isinf=false) -> Q {

    if constexpr (std::is_same<Q, std::complex<double>>::value) {
        gsl_integration_workspace *W_real = gsl_integration_workspace_alloc(Nmax);
        gsl_function F_real;
        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        double result_real{}, error_real{};

        gsl_set_error_handler(handler);

        double result_real_temp{}, error_real_temp{};
        if (isinf) {
            gsl_integration_qagil(&F_real, intervals[0][0], 0, integrator_tol, Nmax, W_real, &result_real, &error_real);
            gsl_integration_qagiu(&F_real, intervals[num_intervals - 1][1], 0, integrator_tol, Nmax, W_real,
                                  &result_real_temp, &error_real_temp);
            result_real += result_real_temp;
            error_real += error_real_temp;
        }
        for (int i = 0; i < num_intervals; i++) {
            result_real_temp = 0.;
            error_real_temp = 0.;
            if (intervals[i][0] < intervals[i][1])
                gsl_integration_qag(&F_real, intervals[i][0], intervals[i][1], 0, integrator_tol, Nmax, 1, W_real,
                                    &result_real_temp, &error_real_temp);
            result_real += result_real_temp;
            error_real += error_real_temp;
        }
        gsl_integration_workspace_free(W_real);


        gsl_integration_workspace *W_imag = gsl_integration_workspace_alloc(Nmax);
        gsl_function F_imag;
        F_imag.function = &f_imag<Integrand>;
        F_imag.params = &integrand;
        double result_imag{}, error_imag{};
        double result_imag_temp{}, error_imag_temp{};
        if (isinf) {
            gsl_integration_qagil(&F_imag, intervals[0][0], 0, integrator_tol, Nmax, W_imag, &result_imag, &error_imag);
            gsl_integration_qagiu(&F_imag, intervals[num_intervals - 1][1], 0, integrator_tol, Nmax, W_imag,
                                  &result_imag_temp, &error_imag_temp);
            result_imag += result_imag_temp;
            error_imag += error_imag_temp;
        }
        for (int i = 0; i < num_intervals; i++) {
            result_imag_temp = 0.;
            error_imag_temp = 0.;
            if (intervals[i][0] < intervals[i][1])
                gsl_integration_qag(&F_imag, intervals[i][0], intervals[i][1], 0, integrator_tol, Nmax, 1, W_imag,
                                    &result_imag_temp, &error_imag_temp);
            result_imag += result_imag_temp;
            error_imag += error_imag_temp;
        }
        gsl_integration_workspace_free(W_imag);

        return result_real + glb_i * result_imag;
    }
    else {
        gsl_integration_workspace *W_real = gsl_integration_workspace_alloc(Nmax);
        gsl_function F_real;
        F_real.function = &f_real<Integrand>;
        F_real.params = &integrand;
        double result_real = 0., error_real = 0.;

        gsl_set_error_handler(handler);

        double result_real_temp{}, error_real_temp{};
        if (isinf) {
            gsl_integration_qagil(&F_real, intervals[0][0], 0, integrator_tol, Nmax, W_real, &result_real, &error_real);
            gsl_integration_qagiu(&F_real, intervals[num_intervals - 1][1], 0, integrator_tol, Nmax, W_real,
                                  &result_real_temp, &error_real_temp);
            result_real += result_real_temp;
            error_real += error_real_temp;
        }

        vec<Q> result = vec<Q>(num_intervals);
        for (int i = 0; i < num_intervals; i++) {
            error_real_temp = 0.;
            if (intervals[i][0] < intervals[i][1])
                gsl_integration_qag(&F_real, intervals[i][0], intervals[i][1], 0., integrator_tol, Nmax, 1, W_real,
                                    &(result[i]), &error_real_temp);
            error_real += error_real_temp;
        }

        gsl_integration_workspace_free(W_real);

        result_real += result.sum();

        return result_real;
    }
}


/// --- WRAPPER FUNCTIONS: INTERFACE FOR ACCESSING THE INTEGRATOR IN BUBBLES/LOOP --- ///

// old wrapper function
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b) -> Q {
    Adapt<Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a, b);
}

// wrapper function, used for loop
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w) -> Q {
    Adapt<Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a, b);
}

/**
 * wrapper function, used for bubbles.
 * @param integrand
 * @param a         :   lower limit for integration
 * @param b         :   upper limit for integration
 * @param w1        :   unused
 * @param w2        :   unused
 */
template <typename Q, typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w1, double w2) -> Q {
    Adapt<Integrand> adaptor(integrator_tol, integrand);
    return adaptor.integrate(a, b);
}

/**
 * Wrapper function for bubbles and loops, splitting the integration domain along difficult features.
 * @param a      : lower limit for integration
 * @param b      : upper limit for integration
 * @param w1     : first frequency where features occur
 * @param w2     : second frequency where features occur
 * @param Delta  : with of window around the features which should be integrated separately (to be set by hybridization strength)
 */
template <typename Integrand> auto integrator(Integrand& integrand, double a, double b, double w1, double w2, double Delta, const bool isinf=false) -> std::result_of_t<Integrand(double)> {
    using return_type = std::result_of_t<Integrand(double)>;

    // define points at which to split the integrals (including lower and upper integration limits)
    rvec intersections{a, w1 - Delta, w1 + Delta, w2 - Delta, w2 + Delta, b};
    std::sort(intersections.begin(), intersections.end()); // sort the intersection points to get correct intervals

    return_type result = myzero<return_type>(); // initialize results
    // integrate intervals of with 2*Delta around the features at w1, w2
    Adapt<Integrand> adaptor_peaks(integrator_tol, integrand);
    result += adaptor_peaks.integrate(intersections[3], intersections[4]);
    result += adaptor_peaks.integrate(intersections[1], intersections[2]);

    // integrate the tails and the interval between the features, with increased tolerance
    Adapt<Integrand> adaptor_tails(integrator_tol * 10, integrand);
    result += adaptor_tails.integrate(intersections[0], intersections[1]);
    result += adaptor_tails.integrate(intersections[2], intersections[3]);
    result += adaptor_tails.integrate(intersections[4], intersections[5]);
    if (isinf) {
        Adapt_semiInfinitLower<Integrand> adapt_il(integrator_tol, integrand, intersections[0]);
        Adapt_semiInfinitUpper<Integrand> adapt_iu(integrator_tol, integrand, intersections[5]);
        result += adapt_il.integrate();
        result += adapt_iu.integrate();
    }
    return result;
}

/**
 * wrapper function, used for bubbles.
 * @param integrand
 * @param intervals         :   list of intervals (lower and upper limit for integrations)
 * @param num_intervals     :   number of intervals
 */
template <typename Integrand> auto integrator(Integrand& integrand, vec<vec<double>>& intervals, const my_index_t num_intervals, const bool isinf=false) -> std::result_of_t<Integrand(double)> {
    using return_type = std::result_of_t<Integrand(double)>;
    Adapt<Integrand> adaptor(integrator_tol, integrand);
    vec<return_type> result = vec<return_type>(num_intervals);
    assert(result.size() == num_intervals);
    for (my_index_t i = 0; i < num_intervals; i++){
        if (intervals[i][0] < intervals[i][1]) result[i] = adaptor.integrate(intervals[i][0], intervals[i][1]);
    }
    if (isinf) {
        Adapt_semiInfinitLower<Integrand> adapt_il(integrator_tol, integrand, intervals[0][0]);
        Adapt_semiInfinitUpper<Integrand> adapt_iu(integrator_tol, integrand, intervals[num_intervals-1][1]);
        result[0] += adapt_il.integrate();
        result[0] += adapt_iu.integrate();
    }
    assert(result.size() == num_intervals);
    const return_type val = result.sum();
    return val;
}

/**
 * wrapper function, used for bubbles.
 * @param integrand
 * @param intervals         :   list of intervals (lower and upper limit for integrations)
 * @param num_intervals     :   number of intervals
 */
template <typename Integrand> auto integrator_onlyTails(Integrand& integrand, const double vmin, const double vmax) -> std::result_of_t<Integrand(double)> {
    using return_type = std::result_of_t<Integrand(double)>;
    Adapt_semiInfinitLower<Integrand> adapt_il(integrator_tol, integrand, vmin);
    Adapt_semiInfinitUpper<Integrand> adapt_iu(integrator_tol, integrand, vmax);
    const return_type val = adapt_il.integrate() + adapt_iu.integrate();

    return val;
}


//#if not KELDYSH_FORMALISM and defined(ZERO_TEMP)
/**
 * wrapper function, used for bubbles. Splits up integration interval in suitable pieces for Matsubara T=0
 * @param integrand
 * @param intervals         :   list of intervals (lower and upper limit for integrations)
 * @param num_intervals     :   number of intervals
 */
template <typename Integrand> auto integrator_Matsubara_T0(Integrand& integrand, const double vmin, const double vmax, double w_half, const vec<double>& freqs, const double Delta, const bool isinf=false) -> std::result_of_t<Integrand(double)> {
    double tol = 1e-10;
    const int num_freqs = freqs.size();

    // The idea is to split up the interval and thereby make sure that the integrator recognizes all the relevant features of the integrand.
    vec<double> intersections;
    size_t num_intervals_max;
    if (w_half < tol) {
        w_half = 0.;
        intersections = {w_half, vmin, vmax};
        num_intervals_max = num_freqs * 2 + 2;
    }
    else {
        intersections = {-w_half, w_half, vmin, vmax};
        num_intervals_max = num_freqs * 2 + 3;
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
    int num_intervals = 0;
    vec<vec<double>> intervals(num_freqs*2 + 3, {1.,-1.});
    for (size_t i = 0; i < num_intervals_max; i++) {
        if (intersections[i] != intersections[i+1]) {
            intervals[num_intervals] = {intersections[i], intersections[i + 1]};
            if (std::abs(std::abs(intersections[i]) - w_half) < tol) {
                intervals[num_intervals][0] += tol;
                if (num_intervals > 0) intervals[num_intervals - 1][1] -= tol;
            }
            num_intervals++;
        }
    }
    if (std::abs(std::abs(intervals[num_intervals-1][1]) - w_half) < tol) {
        intervals[num_intervals-1][1] -= tol;
    }

    return integrator(integrand, intervals, num_intervals, isinf);



}
//#endif


template <typename Q, typename Integrand> auto matsubarasum(const Integrand& integrand, const int Nmin, const int Nmax, const int N_tresh = 60,
        int balance_fac = 2, double reltol = 1e-5, double abstol = 1e-7) -> Q {

    int N = Nmax - Nmin  + 1;

    //// Straightforward summation:
    vec<Q> values(N);
    Q result = 0;
    freqType vpp;
    for (int i = 0; i < N; i++) {
        vpp = ((Nmin + i)*2 + 1);
        //values[i] = integrand(vpp);
        result += integrand(vpp);
    }
    return result;// values.sum();
}

template <int spin, int channel, typename Integrand> auto matsubarasum_vectorized(const Integrand& integrand, const int Nmin_v, const int Nmax_v, const int Nmin_sum, const int Nmax_sum, const int Nmin_vp, const int Nmax_vp) -> Eigen::Matrix<std::result_of_t<Integrand(freqType)>, Eigen::Dynamic, Eigen::Dynamic>{
    using Q = std::result_of_t<Integrand(freqType)>;
    constexpr int n_spin_sum = (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1;

    Eigen::Matrix<Q, Eigen::Dynamic, Eigen::Dynamic> vertex_values_left ( Nmax_v -Nmin_v   +1              ,(Nmax_sum-Nmin_sum+1) * n_spin_sum);
    Eigen::Matrix<Q, Eigen::Dynamic, Eigen::Dynamic> vertex_values_right((Nmax_sum-Nmin_sum+1) * n_spin_sum, Nmax_vp-Nmin_vp  +1              );
    Eigen::Matrix<Q, Eigen::Dynamic, 1> Pi_values((Nmax_sum-Nmin_sum+1) * n_spin_sum);

//#pragma omp parallel for schedule(guided)
    for (int i = Nmin_sum; i <= Nmax_sum; i++) {
        const freqType vpp = (i*2 + 1);
        const Eigen::Matrix<Q, (channel == 't' and spin == 0) or (channel == 'a' and spin == 1) ? 2 : 1, 1> res_temp = integrand.load_Pi_keldysh_and_spin_Components_vectorized(vpp);
        //std::cout << Pi_values.template block<n_spin_sum,1>((-Nmin_sum+i)*n_spin_sum, 0) << std::endl;
        Pi_values.template block<n_spin_sum,1>((-Nmin_sum+i)*n_spin_sum, 0) = res_temp;
    }

//#pragma omp parallel for schedule(static, 500) collapse(2)
    for (int i = Nmin_v; i <= Nmax_v; i++) {
        for (int j = Nmin_sum; j <= Nmax_sum; j++) {
            const freqType v   = (i*2 + 1) ;
            const freqType vpp = (j*2 + 1) ;
            VertexInput input_l = integrand.input_external;
            input_l.v1 = v;
            input_l.v2 = vpp;
            Q value;
            vertex_values_left.template block<1,n_spin_sum>(-Nmin_v+i, (-Nmin_sum+j) * n_spin_sum) = integrand.load_vertex_keldysh_and_spin_Components_left_vectorized(input_l);
        }
    }

//#pragma omp parallel for schedule(static, 500) collapse(2)
    for (int i = Nmin_sum; i <= Nmax_sum; i++) {
        for (int j = Nmin_vp; j <= Nmax_vp; j++) {
            const freqType vpp= (i*2 + 1);
            const freqType vp = (j*2 + 1);
            VertexInput input_r = integrand.input_external;
            input_r.v1 = vpp;
            input_r.v2 = vp;
            Q value;
            vertex_values_right.template block<n_spin_sum,1>((-Nmin_sum+i) * n_spin_sum, -Nmin_vp+j) = integrand.load_vertex_keldysh_and_spin_Components_right_vectorized(input_r);
        }
    }

    Eigen::Matrix<std::result_of_t<Integrand(freqType)>, Eigen::Dynamic, Eigen::Dynamic> result = vertex_values_left * Pi_values.asDiagonal() * vertex_values_right;
    auto result_alt = Pi_values.sum() * 2.5*2.5;

    return vertex_values_left * Pi_values.asDiagonal() * vertex_values_right;

}

#endif //KELDYSH_MFRG_INTEGRATOR_HPP
