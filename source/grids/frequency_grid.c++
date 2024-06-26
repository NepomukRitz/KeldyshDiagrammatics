#include "frequency_grid.hpp"


/**
 * This function initializes the frequency grid according to the grid parameters
 * w_upper(=-w_lower):  upper limit of frequency box
 * number_of_gridpoints:                 number of frequency points
 * W_scale:             non-linearity of the t_from_frequency function
 */
void FrequencyGrid<eliasGrid>::initialize_grid() {
    derive_auxiliary_parameters();
    freqType W;
    for(int i=0; i<number_of_gridpoints; ++i) {
        W = t_lower + i * spacing_auxiliary_gridpoint;
        all_frequencies[i] = frequency_from_t(W);
        assert(my_isfinite(all_frequencies[i]));
        auxiliary_grid[i]= W;
    }

    if (purely_positive) {
        all_frequencies[0] = 0.;
        auxiliary_grid[0] = 0.;
    }

    if (!KELDYSH && !ZERO_T) assert (is_doubleOccurencies(all_frequencies) == 0);
}


/// derive auxiliary parameters from essential parameters
void FrequencyGrid<eliasGrid>::derive_auxiliary_parameters() {
    t_upper = t_from_frequency(w_upper);
    t_lower = t_from_frequency(w_lower);
    spacing_auxiliary_gridpoint = (t_upper - t_lower) / ((double) (number_of_gridpoints-1));
}
void FrequencyGrid<eliasGrid>::guess_essential_parameters(double Lambda, const fRG_config& config) {

    U_factor = 0.;
    switch (diag_class) {
        case 1:
            switch (type) {
                case 'b':
                    number_of_gridpoints = nBOS;
                    Delta_factor = Delta_factor_K1;
                    break;
                case 'f':
                    number_of_gridpoints = nFER;
                    Delta_factor = Delta_factor_SE;
                    break;
                default:
                    break;
                    }
            break;
        case 2:
            switch (type) {
                case 'b':
                    number_of_gridpoints = nBOS2;
                    Delta_factor = Delta_factor_K2_w;
                    break;
                case 'f':
                    number_of_gridpoints = nFER2;
                    Delta_factor = Delta_factor_K2_v;
                    break;
                default:
                    break;
                }
            break;
        case 3:
            switch (type) {
                case 'b':
                    number_of_gridpoints = nBOS3;
                    Delta_factor = Delta_factor_K3_w;
                    break;
                case 'f':
                    number_of_gridpoints = nFER3;
                    Delta_factor = Delta_factor_K3_v;
                    break;
                default:;
                }
        default:;
    }


    freqType scale;
    if constexpr(REG == 2) {
        // scale the grid with Delta until Delta = T_K (Kondo temperature), then scale with T_K
        double Delta = (Lambda + config.Gamma) / 2.;
        scale = Delta_factor * Delta;
    }
    else if constexpr(REG == 3) {
        scale = std::max(U_factor * config.U, Delta_factor * (config.Gamma) / 2. + Lambda * (Lambda + 1));
    }
    else {
        //scale = std::max(U_factor * glb_U, Delta_factor * (config.Gamma) / 2.);
        scale = Delta_factor * (config.Gamma) / 2.;
    }
    freqType wmax_guess = scale * 100.;

#if not KELDYSH_FORMALISM and not ZERO_TEMP
    scale = 2;
    wmax_guess = (number_of_gridpoints-1);
#endif

    all_frequencies = vec<freqType>(number_of_gridpoints);
    auxiliary_grid = vec<freqType>(number_of_gridpoints);

    set_essential_parameters(wmax_guess, scale);
    initialize_grid();
}

void FrequencyGrid<eliasGrid>::update_Wscale(freqType Wscale) {
    W_scale = Wscale;
    initialize_grid();
}

/** This function returns the index corresponding to the frequency w_in.
 *  It rounds down due to the narrowing conversion from double to int.
 *  This is only used for (linear) interpolations. Hence the narrowing conversion is harmless.
 */
auto FrequencyGrid<eliasGrid>::get_grid_index(const freqType w_in) const -> int {
#ifdef PARAMETRIZED_GRID
    const freqType t = (t_from_frequency(w_in) - t_lower) / spacing_auxiliary_gridpoint;
#ifdef DENSEGRID
    auto index = ((int) (t + 0.1 )) ;
#else
    int index = ((int) t) ;
    if (INTERPOLATION==linear) {
        index = std::max(0, index);
        index = std::min(number_of_gridpoints - 2, index);
    }
    else {
        if (all_frequencies[index+1] < w_in) index++;
        index = std::max(0, index);
        index = std::min(number_of_gridpoints - 2, index);
    }
#endif
    return index;

#else
    int j;
    if (INTERPOLATION==linear) {locate(all_frequencies, number_of_gridpoints, w_in, j, 0, number_of_gridpoints-1);} // we cannot interpolate with infinity
    else {locate(all_frequencies, number_of_gridpoints, w_in, j, 0, number_of_gridpoints-1); }
    int index = j;
        assert(all_frequencies[index] - w_in  <= 1e-5 or index == 0);
        assert(w_in - all_frequencies[index+1] < 1e-5 or index == number_of_gridpoints-1);
    return index;
#endif
}

/** This function returns the index corresponding to the frequency w_in.
 *  It rounds down due to the narrowing conversion from double to int.
 *  This is only used for (linear) interpolations. Hence the narrowing conversion is harmless.
 */
auto FrequencyGrid<eliasGrid>::get_grid_index(freqType& t, const freqType w_in) const -> int {
    t = t_from_frequency(w_in);
    assert(isfinite(t));
#ifdef PARAMETRIZED_GRID

    freqType t_rescaled = (t - t_lower) / spacing_auxiliary_gridpoint;
    auto index = ((int) t_rescaled) ;  // round down
    if constexpr(INTERPOLATION==linear) {
        index = std::max(0, index);
        index = std::min(number_of_gridpoints - 2, index);
    }
    else {
        if (all_frequencies[index+1] < w_in and index < number_of_gridpoints-1)
            index++;
        if (all_frequencies[index] > w_in and index > 0)
            index--;
        index = std::max(0, index);
        index = std::min(number_of_gridpoints - 2, index);
        assert(all_frequencies[index] - w_in  <= 1e-5*std::abs(w_in) or index == 0); /// TODO: If this is not satisfied -> use locate
        assert(w_in - all_frequencies[index+1] < 1e-5 or index == number_of_gridpoints-2);
    }
    return index;

#else
    int j;
    if (INTERPOLATION==linear) {locate(all_frequencies, number_of_gridpoints, w_in, j, 0, number_of_gridpoints-1);} // we cannot interpolate with infinity
    else {locate(auxiliary_grid, number_of_gridpoints, t, j, 0, number_of_gridpoints-1); }
    int index = j;
        assert(all_frequencies[index] - w_in  <= 1e-5 or index == 0);
        assert(w_in - all_frequencies[index+1] < 1e-5 or index == number_of_gridpoints-1);
    return index;
#endif

}

/**
 * The used t_from_frequency function is determined here
 * @param w     frequency
 * @return
 */
auto FrequencyGrid<eliasGrid>::t_from_frequency(freqType w) const -> freqType {
    if constexpr (KELDYSH) return grid_transf_v2(w, this->W_scale, w_center);
    else if (this->type == 'f' and this->diag_class == 1) {
        if constexpr (ZERO_T) return grid_transf_v3(w, this->W_scale, w_center);
        else return grid_transf_lin(w, this->W_scale);

    }
    else {
        if constexpr (ZERO_T) return grid_transf_v4(w, this->W_scale, w_center);
        else                   return grid_transf_lin(w, this->W_scale);
    }
}

/**
 * The used frequency_from_t function is determined here
 * @param t     point on auxiliary grid
 * @return
 */
auto FrequencyGrid<eliasGrid>::frequency_from_t(freqType t) const -> freqType {
    if constexpr (KELDYSH) return grid_transf_inv_v2(t, this->W_scale, w_center);
    else if (this->type == 'f' and this->diag_class == 1) {
        if (ZERO_T) return grid_transf_inv_v3(t, this->W_scale, w_center);
        else return grid_transf_inv_lin(t, this->W_scale) + (std::is_same_v<freqType,int> and type=='f' ? 1 : 0);
    }
    else {
        if constexpr (KELDYSH || ZERO_T) return grid_transf_inv_v4(t, this->W_scale, w_center);
        else return grid_transf_inv_lin(t, this->W_scale) + (std::is_same_v<freqType,int> and type=='f' ? 1 : 0);
    }
}

/**
 * This function picks a suitable W_scale for a given wmax for Matsubara T>0
 * @return Wscale    non-linearity of t_from_frequency()
 * @param w1        first positive Matsubara frequency
 * @param wmax      upper bound of frequency grid
 * @param N         relates tmax to t1 by t1=tmax/N (for bosons: (nBOS-1)/2; for fermions: nFER-1)
 */
auto FrequencyGrid<eliasGrid>::wscale_from_wmax(freqType & Wscale, const freqType w1, const freqType wmax, const int N) -> double {
    if (!KELDYSH && !ZERO_T){
        if (this->type == 'f' and this->diag_class == 1) {
            return wscale_from_wmax_v3(Wscale, w1, wmax, N);
        }
        else {
            return wscale_from_wmax_v1(Wscale, w1, wmax, N);
        }
    }
    else {
        assert(false);
        return 0;
    }
}


/*******************************************    FREQUENCY GRID    *****************************************************/


/**
 * Here are several functions which map the real axis to the compact interval [-1,1]
 * W_scale: sets the scale separating small frequencies (containing interesting structures) and tails
 */

double grid_transf_v1(const double w, const double W_scale, const double w_center) {
    // Version 1: linear around w=0, good for w^(-2) tails
    const double w_dev = w - w_center;
    return w_dev/sqrt(W_scale*W_scale + w_dev*w_dev);
}
double grid_transf_inv_v1(const double t, const double W_scale, const double w_center) {
    // Version 1: linear around w=0, good for w^(-2) tails
    return w_center + W_scale*t/sqrt(1.-t*t);
}
double integration_measure_v1(const double t, const double W_scale) {
    double temp = sqrt(1 - t*t);
    return W_scale / (temp*temp*temp);
}


double grid_transf_v2(const double w, const double W_scale, const double w_center) {
    // Version 2: quadratic around w=0, good for w^(-2) tails
    const double w_dev = w - w_center;
    const double w2 = w_dev * w_dev;
    const double res = sgn(w_dev) * sqrt((sqrt(w2*w2 + 4 * w2 * W_scale * W_scale) - w2) / 2.) / W_scale;
    assert(isfinite(res));
    return res;
}
double grid_transf_inv_v2(const double t, const double W_scale, const double w_center) {
    // Version 2: quadratic around w=0, good for w^(-2) tails
    return w_center + W_scale * t * std::abs(t) / sqrt(1. - t * t);
}
double integration_measure_v2(const double t, const double W_scale) {
    double temp = sqrt(1 - t*t);
    return W_scale * sgn(t) * t * (2 - t*t) / (temp*temp*temp);
}


double grid_transf_v3(const double w, const double W_scale, const double w_center) {
    // Version 3: linear around w=0, good for w^(-1) tails
    const double almost_zero = 1e-12;
    const double w_dev = w - w_center;
    return (std::abs(w_dev) < almost_zero) ? 0. : (-W_scale + sqrt(4*w_dev*w_dev + W_scale*W_scale))/2/w_dev;
}
double grid_transf_inv_v3(const double t, const double W_scale, const double w_center) {
    // Version 3: linear around w=0, good for w^(-1) tails
    return w_center + W_scale * t / (1.-t*t);
}
double integration_measure_v3(const double t, const double W_scale) {
    double temp = t*t;
    return W_scale * (1 + temp) / (1 - temp) / (1 - temp);
}

double grid_transf_v4(const double w, const double W_scale, const double w_center) {
    // Version 4: quadratic around w=0, good for w^(-1) tails
    const double w_dev = w - w_center;
    const double abs_w = std::abs(w_dev);
    return sgn(w_dev) * sqrt(abs_w/(abs_w + W_scale));
}
double grid_transf_inv_v4(const double t, const double W_scale, const double w_center) {
    // Version 4: quadratic around w=0, good for w^(-1) tails
    return w_center + W_scale * sgn(t) * t*t /(1.-t*t);
}
double integration_measure_v4(const double t, const double W_scale) {
    double temp = 1 - t*t;
    return 2 * W_scale * sgn(t) * t / temp / temp;
}



////    linear grid
double grid_transf_lin(double w, double W_scale) {
    return w / W_scale;
}

freqType grid_transf_inv_lin(freqType W, freqType W_scale) {
    return W * W_scale;
}

/**
 * Makes sure that lower bound for W_scale fulfilled
 * Wscale:  current value for W_scale
 * w1:      smallest positive Matsubara frequency (for bosons: 2*M_PI*T, for fermions: M_PI*T)
 * wmax:    maximal frequency
 * N:       relates tmax to t1 by t1=tmax/N (for bosons: (nBOS-1)/2; for fermions: nFER-1)
*/
freqType wscale_from_wmax_v1(freqType & Wscale, const freqType w1, const freqType wmax, const int N) {
    freqType Wscale_candidate;

    // Version 1: linear around w=0, good for w^(-2) tails
    Wscale_candidate = wmax * sqrt( (N*N -1*2) / (pow(wmax/w1, 2) - N*N));
    return std::max(Wscale, Wscale_candidate);
}
freqType wscale_from_wmax_v2(freqType & Wscale, const freqType w1, const freqType wmax, const int N) {
    freqType Wscale_candidate;

    // Version 2: quaadratic around w=0, good for w^(-2) tails
    assert(w1 / wmax < 1./(N*N));       // if this fails, then change wmax or use Version 1
    Wscale_candidate = w1 * wmax * N * sqrt(N*N - 1) *sqrt(wmax*wmax - N*N*w1*w1) / std::abs(wmax*wmax - w1*w1*N*N*N*N);

    return std::max(Wscale, Wscale_candidate);
}
freqType wscale_from_wmax_v3(freqType & Wscale, const freqType w1, const freqType wmax, const int N) {
    freqType Wscale_candidate;

    // Version 3: quadratic around w=0, good for w^(-1) tails
    Wscale_candidate = w1 * wmax * (N*N - 1*2.) / sqrt(N * (N * (wmax*wmax + w1*w1) - N*N*w1*wmax - w1*wmax));

    return std::max(Wscale, Wscale_candidate);
}






/*******************************************    HYBRID GRID    *****************************************************/


void FrequencyGrid<hybridGrid>::derive_auxiliary_parameters() {
#if HYBRID_GRID_OPTION==0
    const double temp = (pos_section_boundaries[0] + 2.*pos_section_boundaries[1]);
    const double temp_quad = temp * w_upper - pos_section_boundaries[1]*pos_section_boundaries[1];
    aux_pos_section_boundaries[0] = (2.*t_upper*pos_section_boundaries[0]*w_upper)                              / temp_quad;
    aux_pos_section_boundaries[1] = (t_upper*(pos_section_boundaries[0] + pos_section_boundaries[1]) * w_upper) / temp_quad;
    recip_curvature_quad = (4.*t_upper*t_upper*pos_section_boundaries[0]*w_upper*w_upper) / (temp_quad*temp_quad );
    recip_slope_lin = t_upper*w_upper / (temp_quad);
    factor_rat = pos_section_boundaries[1]*pos_section_boundaries[1] / (temp);
    rescale_rat = (t_upper * w_upper * temp) / temp_quad;
#else
    const double log_wmax_w2 = log(w_upper / pos_section_boundaries[1]);
    const double factor = (pos_section_boundaries[1] - pos_section_boundaries[0]) / (1 - 2 * pos_section_boundaries[0] / (pos_section_boundaries[0] + pos_section_boundaries[1])) / pos_section_boundaries[1];
    //aux_pos_section_boundaries[1] = t_upper / (1 + log(w_upper/pos_section_boundaries[1]) * (1 - 2. * pos_section_boundaries[0] / (pos_section_boundaries[0] + pos_section_boundaries[1]) / (pos_section_boundaries[1] - pos_section_boundaries[0])));
    aux_pos_section_boundaries[1] = t_upper / (log_wmax_w2 / factor + 1);
    aux_pos_section_boundaries[0] = 2. * pos_section_boundaries[0] * aux_pos_section_boundaries[1] / (pos_section_boundaries[0] + pos_section_boundaries[1]);
    recip_slope_lin = (aux_pos_section_boundaries[1] - aux_pos_section_boundaries[0]) / (pos_section_boundaries[1] - pos_section_boundaries[0]);
    recip_curvature_quad = aux_pos_section_boundaries[0]*aux_pos_section_boundaries[0] / pos_section_boundaries[0];
#endif
    spacing_auxiliary_gridpoint = (t_upper - t_lower) / ((double)number_of_gridpoints - 1.);
}

void FrequencyGrid<hybridGrid>::guess_essential_parameters(const double Lambda, const fRG_config& config) {
    const double Delta = (config.Gamma + Lambda) * 0.5;

    switch (diag_class) {
        case 1:
            if (type == 'b') {
                number_of_gridpoints = nBOS;
                pos_section_boundaries[0] = 20.*Delta;
                pos_section_boundaries[1] = 30.*Delta;
                w_upper = 5000.*15.*Delta;
            }
            else {
                assert(type == 'f');
                number_of_gridpoints = nFER;
                pos_section_boundaries[0] = 25.;
                pos_section_boundaries[1] = 27.*Delta;
                w_upper = 40.*15.*Delta;
            }
            break;
        case 2:
            if (type == 'b') {
                number_of_gridpoints = nBOS2;
                pos_section_boundaries[0] = 6.*Delta;
                pos_section_boundaries[1] = 20.*Delta;
                w_upper = 80.*15.*Delta;
            }
            else {
                assert(type == 'f');
                number_of_gridpoints = nFER2;
                pos_section_boundaries[0] = 3.*Delta;
                pos_section_boundaries[1] = 10.*Delta;
                w_upper = 40.*15.*Delta;
            }
            break;
        case 3:
            if (type == 'b') {
                number_of_gridpoints = nBOS3;
                pos_section_boundaries[0] = Delta;
                pos_section_boundaries[1] = 3.*Delta;
                w_upper = 30.*15.*Delta;
            }
            else {
                assert(type == 'f');
                number_of_gridpoints = nFER3;
                pos_section_boundaries[0] = Delta;
                pos_section_boundaries[1] = 3.*Delta;
                w_upper = 15*15.*Delta;
            }
            break;
        default:
            break;
    }

    t_upper = ((double) number_of_gridpoints - 1) * (purely_positive ? 1. : 0.5);

    if (purely_positive) {
        w_lower = 0;
        t_lower = 0;
    }
    else {
        w_lower = -w_upper;
        t_lower =-((double) number_of_gridpoints - 1) * 0.5;
    }
    all_frequencies = vec<freqType>(number_of_gridpoints);
    auxiliary_grid = vec<freqType>(number_of_gridpoints);
    initialize_grid();
}

freqType FrequencyGrid<hybridGrid>::frequency_from_t(const freqType t) const {
    const double t_abs = std::abs(t);
    if (t_abs < aux_pos_section_boundaries[0]) {
        // quaddratic part
        const double result = t_abs*t_abs / recip_curvature_quad * sgn(t);
        assert(isfinite(result));
        return result;
    }
    else if (t_abs < aux_pos_section_boundaries[1]) {
        // linear part
        const double result = (pos_section_boundaries[0] + (t_abs - aux_pos_section_boundaries[0]) / recip_slope_lin)* sgn(t);
        assert(isfinite(result));
        return result;
    }
    else {
#if HYBRID_GRID_OPTION==0
        // rational part
        const double result = factor_rat / (1. - t_abs / rescale_rat) * sgn(t);
        assert(isfinite(result));
        return result;
#else
        // exponential part
        const double result = (pos_section_boundaries[1] * exp((t_abs - aux_pos_section_boundaries[1]) / (recip_slope_lin * pos_section_boundaries[1]))) * sgn(t);
        assert(isfinite(result));
        return result;
#endif
    }

}

freqType FrequencyGrid<hybridGrid>::t_from_frequency(const freqType w) const {
    const double w_abs = std::abs(w);
    if (w_abs < pos_section_boundaries[0]) {
        // quadratic part
        const double result = sqrt(w_abs * recip_curvature_quad) * sgn(w);
        assert(isfinite(result));
        return result;
    }
    else if (w_abs < pos_section_boundaries[1]) {
        // linear part
        const double result = ((w_abs - pos_section_boundaries[0]) * recip_slope_lin + aux_pos_section_boundaries[0]) * sgn(w);
        assert(isfinite(result));
        return result;
    }
    else {
#if HYBRID_GRID_OPTION==0
        // rational part
        const double result = (1. - factor_rat / w_abs) * rescale_rat * sgn(w);
        assert(isfinite(result));
        return result;
#else
        // exponential part
        const double result = sgn(w) * (aux_pos_section_boundaries[1] + log(w_abs / pos_section_boundaries[1]) * pos_section_boundaries[1] * recip_slope_lin);
        assert(isfinite(result));
        return result;
#endif
    }
}

void FrequencyGrid<hybridGrid>::initialize_grid() {
    derive_auxiliary_parameters();
    for (int i = 0; i < number_of_gridpoints; i++) {
        const double t = t_lower + spacing_auxiliary_gridpoint * (double)i;
        auxiliary_grid[i] = t;
        all_frequencies[i] = frequency_from_t(t);
        assert(std::isfinite(all_frequencies[i]));
    }
    if (purely_positive) {
        all_frequencies[0] = 0.;
        auxiliary_grid[0] = 0.;
    }
    if (number_of_gridpoints % 2 == 1 and not purely_positive) {
        all_frequencies[(int) number_of_gridpoints / 2] = 0.;  // make sure that the center of the grid is exactly zero (and not ~10^{-30})
        auxiliary_grid[(int) number_of_gridpoints / 2] = 0.;
    }
    if (!KELDYSH && !ZERO_T) assert (is_doubleOccurencies(all_frequencies) == 0);
}

int FrequencyGrid<hybridGrid>::get_grid_index(const freqType frequency)const {
    /// Do I need this? The next function basically does the same
    const double t = t_from_frequency(frequency);
#ifdef DENSEGRID
    auto index = ((int) ((t - t_lower) / spacing_auxiliary_gridpoint + 0.1 )) ;
#else
    assert(std::abs(spacing_auxiliary_gridpoint - 1.) < 1e-10); // By making sure that spacing_auxiliary_gridpoint = 1, there is no need to divide through spacing_auxiliary_gridpoint
    int index = int((t - t_lower) + 1e-12);
    index = std::max(0, index);
    index = std::min(number_of_gridpoints - 2, index);
#endif
    assert(auxiliary_grid[index] <= t+inter_tol);
    assert(auxiliary_grid[index+1] >= t-inter_tol or index == number_of_gridpoints-2);
    return index;
}

int FrequencyGrid<hybridGrid>::get_grid_index(freqType& t, const freqType frequency) const{
    t = t_from_frequency(frequency);
#ifdef DENSEGRID
    auto index = ((int) ((t - t_lower) / spacing_auxiliary_gridpoint + 0.1 )) ;
#else
    assert(std::abs(spacing_auxiliary_gridpoint - 1.) < 1e-10); // By making sure that spacing_auxiliary_gridpoint = 1, there is no need to divide through spacing_auxiliary_gridpoint
    int index = int((t - t_lower) + 1e-12);
    index = std::max(0, index);
    index = std::min(number_of_gridpoints - 2, index);
#endif
    assert(auxiliary_grid[index] <= t + inter_tol);
    assert(auxiliary_grid[index+1] >= t - inter_tol or index == number_of_gridpoints-2);
    return index;
}



/*******************************************    ANGULAR GRID    *****************************************************/


void FrequencyGrid<angularGrid>::derive_auxiliary_parameters() {
    recip_power = 1./power;
    lin_fac_to_power = pow(lin_fac, power);
    spacing_auxiliary_gridpoint = (t_upper - t_lower) / (number_of_gridpoints - 1);
    half_of_interval_length_for_t = (t_upper - t_lower) * 0.5 / number_of_intervals;
    half_of_interval_length_for_w = (w_upper - w_lower) * 0.5 / number_of_intervals;
    half_of_interval_length_for_w_recip = number_of_intervals / ((w_upper - w_lower) * 0.5);
    interval_length_for_w_recip = (double)number_of_intervals / (w_upper - w_lower);
    quad_fac_recip = (pow(1 + lin_fac, power) - lin_fac_to_power);
}

void FrequencyGrid<angularGrid>::guess_essential_parameters(const double Lambda, const fRG_config& config) {

    switch (diag_class) {
        case 1:
            // nonsensical for 1D objects
            number_of_gridpoints = nBOS;
            w_upper = 2*M_PI;
            number_of_intervals = 1;
            break;
        case 2:
            if (type == 'b') {
                assert(false);
            }
            else {
                assert(type == 'f');
                number_of_gridpoints = nFER2;
                w_upper = M_PI;
                number_of_intervals = 8;
                assert( (number_of_gridpoints - 1) % number_of_intervals == 0);
            }
            break;
        case 3:
            /// TODO: investigate structure in K3 and implement suitable grid
            if (type == 'b') {
                number_of_gridpoints = nBOS3;
                number_of_intervals = 8;
                w_upper = M_PI;
            }
            else {
                assert(type == 'f');
                number_of_gridpoints = K3_config.dims[purely_positive ? my_defs::K3::nup : my_defs::K3::nu];
                number_of_intervals = purely_positive ? 4 : 8;
                w_upper = M_PI;
                assert( (number_of_gridpoints - 1) % number_of_intervals == 0);
            }
            break;
        default:
            break;
    }

    t_upper = ((double) number_of_gridpoints - 1) * (purely_positive ? 1. : 0.5);

    if (purely_positive) {
        w_lower = 0;
        t_lower = 0;
    }
    else {
        w_lower = -w_upper;
        t_lower = -t_upper;
    }
    all_frequencies = rvec(number_of_gridpoints);
    auxiliary_grid = rvec(number_of_gridpoints);
    power = 2;
    initialize_grid();
}

double FrequencyGrid<angularGrid>::frequency_from_t(const double t) const {
    const double i_interval = floor( (t + half_of_interval_length_for_t) * number_of_intervals / (t_upper - t_lower) );
    const double remainder  = t / half_of_interval_length_for_t - i_interval * 2; // remainder in [-1, 1]
    //const double result = ( i_interval + 0.5 / quad_fac_recip * remainder * (std::abs(remainder) + 2.*lin_fac)) * half_of_interval_length_for_w * 2.;
    const double result = ( i_interval + 0.5 / quad_fac_recip * sgn(remainder) * (pow(std::abs(remainder) + lin_fac, power) - lin_fac_to_power)) * half_of_interval_length_for_w * 2.;
    assert(isfinite(result));
    return result;

}

double FrequencyGrid<angularGrid>::t_from_frequency(const double w) const {
    const double i_interval = floor( (w + half_of_interval_length_for_w) * interval_length_for_w_recip);
    const double remainder  = w / half_of_interval_length_for_w - i_interval * 2; // remainder in [-1, 1]
    const double result = ( i_interval + 0.5 * (pow(std::abs(remainder) * quad_fac_recip + lin_fac_to_power, recip_power) - lin_fac) * sgn(remainder)) * half_of_interval_length_for_t * 2.;
    assert(isfinite(result));
    return result;
}

void FrequencyGrid<angularGrid>::initialize_grid() {
    derive_auxiliary_parameters();

    for (int i = 0; i < number_of_gridpoints; i++) {
        const double t = t_lower + spacing_auxiliary_gridpoint * (double)i;
        auxiliary_grid[i] = t;
        all_frequencies[i] = frequency_from_t(t);
    }
}

int FrequencyGrid<angularGrid>::get_grid_index(const double frequency)const {
    /// Do I need this? The next function basically does the same
    const double t = t_from_frequency(frequency);
#ifdef DENSEGRID
    assert(false);
    int index = 0;
    return index;
#else
    assert(std::abs(spacing_auxiliary_gridpoint - 1.) < 1e-10); // By making sure that spacing_auxiliary_gridpoint = 1, there is no need to divide through spacing_auxiliary_gridpoint
    int index = int((t - t_lower) + 1e-12);
    index = std::max(0, index);
    index = std::min(number_of_gridpoints - 2, index);
    assert(auxiliary_grid[index] <= t+inter_tol);
    assert(auxiliary_grid[index+1] >= t-inter_tol or index == number_of_gridpoints-2);
    return index;
#endif
}


int FrequencyGrid<angularGrid>::get_grid_index(double& t, const double frequency) const{
    t = t_from_frequency(frequency);
#ifdef DENSEGRID
    assert(false);
    int index = 0;
    return index;
#else
    assert(std::abs(spacing_auxiliary_gridpoint - 1) < 1e-15);
    int index = int((t - t_lower) + 1e-12);
    index = std::max(0, index);
    index = std::min(number_of_gridpoints - 2, index);
    assert(auxiliary_grid[index] <= t + inter_tol);
    assert(auxiliary_grid[index+1] >= t - inter_tol or index == number_of_gridpoints-2);
    return index;
#endif
}