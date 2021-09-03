#ifndef FPP_MFRG_INTERPOLATION_FUNCTIONS_H
#define FPP_MFRG_INTERPOLATION_FUNCTIONS_H

#include <cmath>
#include <functional>
#include "../grids/frequency_grid.h"

/**
 * Interpolates linearly in 1D (on linear, auxiliary frequency grid)
 * ATTENTION!: all
 * @tparam Q            double or comp
 * @param x
 * @param frequencies   frequencyGrid with the functions fconv and with x-values in vector ws
 * @param val           any function that takes one integer and returns a value of type Q
 * @return
 */
template <typename Q>
inline auto interpolate1D(const double x, const FrequencyGrid& frequencies, const std::function<Q(int)> val) -> Q {

    return interpolate_sloppycubic1D(x, frequencies, val);
}

/**
 * Interpolates linearly in 2D (on linear, auxiliary frequency grid)
 * @tparam Q            double or comp
 * @param x
 * @param y
 * @param xfrequencies  frequencyGrid with the functions fconv and with x-values in vector ws
 * @param yfrequencies  frequencyGrid with the functions fconv and with y-values in vector ws
 * @param val           any function f(i,j) that takes two integers and returns a value of type Q
 *                      where integer i belongs to x
 *                        and integer j belongs to y
 * @return
 */
template <typename Q>
inline auto interpolate2D(const double x, const double y,
                          const FrequencyGrid& xfrequencies, const FrequencyGrid& yfrequencies,
                          const std::function<Q(int, int)> val) -> Q {

    return interpolate_lin2D(x, y , xfrequencies, yfrequencies, val);
}

/**
 * Interpolates linearly in 3D (on linear, auxiliary frequency grid)
 * @tparam Q            double or comp
 * @param x
 * @param y
 * @param z
 * @param xfrequencies  frequencyGrid with the functions fconv and with x-values in vector ws
 * @param yfrequencies  frequencyGrid with the functions fconv and with y-values in vector ws
 * @param zfrequencies  frequencyGrid with the functions fconv and with z-values in vector ws
 * @param val           any function f(i,j,k) that takes three integers and returns a value of type Q
 *                      where integer i belongs to x
 *                        and integer j belongs to y
 *                        and integer k belongs to z
 * @return
 */
template <typename Q>
inline auto interpolate3D(const double x, const double y, const double z,
                          const FrequencyGrid& xfrequencies, const FrequencyGrid& yfrequencies, const FrequencyGrid& zfrequencies,
                          const std::function<Q(int, int, int)> val) -> Q {

    return interpolate_lin3D(x, y, z, xfrequencies, yfrequencies, zfrequencies, val);

}



/**
 * Interpolates linearly in 1D (on linear, auxiliary frequency grid)
 * ATTENTION!: all
 * @tparam Q            double or comp
 * @param x
 * @param frequencies   frequencyGrid with the functions fconv and with x-values in vector ws
 * @param val           any function that takes one integer and returns a value of type Q
 * @return
 */
template <typename Q>
inline auto interpolate_lin1D(const double x, const FrequencyGrid& frequencies, const std::function<Q(int)> val) -> Q {

    double t;
    int index = frequencies.fconv(t, x);

    double x1 = frequencies.ts[index];
    double x2 = frequencies.ts[index + 1];
    double xd = (t - x1) / (x2 - x1);

    Q f1 = val(index);
    Q f2 = val(index + 1);

    Q result = ((1. - xd) * f1 + xd * f2);
    assert(isfinite(result));
    return result;
}

/**
 * Interpolates linearly in 2D (on linear, auxiliary frequency grid)
 * @tparam Q            double or comp
 * @param x
 * @param y
 * @param xfrequencies  frequencyGrid with the functions fconv and with x-values in vector ws
 * @param yfrequencies  frequencyGrid with the functions fconv and with y-values in vector ws
 * @param val           any function f(i,j) that takes two integers and returns a value of type Q
 *                      where integer i belongs to x
 *                        and integer j belongs to y
 * @return
 */
template <typename Q>
inline auto interpolate_lin2D(const double x, const double y,
                          const FrequencyGrid& xfrequencies, const FrequencyGrid& yfrequencies,
                          const std::function<Q(int, int)> val) -> Q {

    double t;
    int index = xfrequencies.fconv(t, x);

    double x1 = xfrequencies.ts[index];
    double x2 = xfrequencies.ts[index + 1];
    double xd = (t - x1) / (x2 - x1);

    Q f1 = interpolate1D<Q>(y, yfrequencies, [&index, &val](int i) -> Q {return val(index  , i);});
    Q f2 = interpolate1D<Q>(y, yfrequencies, [&index, &val](int i) -> Q {return val(index+1, i);});

    Q result = ((1. - xd) * f1 + xd * f2);
    assert(isfinite(result));
    return result;
}

/**
 * Interpolates linearly in 3D (on linear, auxiliary frequency grid)
 * @tparam Q            double or comp
 * @param x
 * @param y
 * @param z
 * @param xfrequencies  frequencyGrid with the functions fconv and with x-values in vector ws
 * @param yfrequencies  frequencyGrid with the functions fconv and with y-values in vector ws
 * @param zfrequencies  frequencyGrid with the functions fconv and with z-values in vector ws
 * @param val           any function f(i,j,k) that takes three integers and returns a value of type Q
 *                      where integer i belongs to x
 *                        and integer j belongs to y
 *                        and integer k belongs to z
 * @return
 */
template <typename Q>
inline auto interpolate_lin3D(const double x, const double y, const double z,
                          const FrequencyGrid& xfrequencies, const FrequencyGrid& yfrequencies, const FrequencyGrid& zfrequencies,
                          const std::function<Q(int, int, int)> val) -> Q {

    double t;
    int index = xfrequencies.fconv(t, x);

    double x1 = xfrequencies.ts[index];
    double x2 = xfrequencies.ts[index + 1];
    double xd = (t - x1) / (x2 - x1);

    Q f1 = interpolate2D<Q>(y, z, yfrequencies, zfrequencies, [&index, &val](int i, int j) -> Q {return val(index  , i, j);});
    Q f2 = interpolate2D<Q>(y, z, yfrequencies, zfrequencies, [&index, &val](int i, int j) -> Q {return val(index+1, i, j);});

    Q result = ((1. - xd) * f1 + xd * f2);
    assert(isfinite(result));
    return result;
}


/**
 * Interpolates cubically in 1D (on linear, auxiliary frequency grid)
 * ATTENTION!: all
 * @tparam Q            double or comp
 * @param x
 * @param frequencies   frequencyGrid with the functions fconv and with x-values in vector ws
 * @param val           any function that takes one integer and returns a value of type Q
 * @return
 */
template <typename Q>
inline auto interpolate_sloppycubic1D(const double x, const FrequencyGrid& frequencies, const std::function<Q(int)> val) -> Q {

    double t;
    int index = frequencies.fconv(t, x);

    //double xs [4] = {frequencies.ts[index - 1]
    //                ,frequencies.ts[index    ]
    //                ,frequencies.ts[index + 1]
    //                ,frequencies.ts[index + 2]};
    //double ys [4] = {val(index - 1)
    //                ,val(index    )
    //                ,val(index + 1)
    //                ,val(index + 2)};
    //Q result =  lagrangePoly<4>(x, xs, ys);

    double x0 = frequencies.ts[index - 1];
    double x1 = frequencies.ts[index    ];
    double x2 = frequencies.ts[index + 1];
    double x3 = frequencies.ts[index + 2];

    auto f0 = val(index - 1);
    auto f1 = val(index    );
    auto f2 = val(index + 1);
    auto f3 = val(index + 2);

    Q result = ((t - x1)*(t - x2)*(t - x3)/((x0-x1)*(x0-x2)*(x0-x3)) * f0
               +(t - x0)*(t - x2)*(t - x3)/((x1-x0)*(x1-x2)*(x1-x3)) * f1
               +(t - x0)*(t - x1)*(t - x3)/((x2-x0)*(x2-x1)*(x2-x3)) * f2
               +(t - x0)*(t - x1)*(t - x2)/((x3-x0)*(x3-x1)*(x3-x2)) * f3);

    assert(isfinite(result));
    return result;


}


#endif //FPP_MFRG_INTERPOLATION_FUNCTIONS_H