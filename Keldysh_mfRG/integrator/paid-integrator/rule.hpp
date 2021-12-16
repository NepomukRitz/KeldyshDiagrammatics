// This file is a part of PAID.
// Copyright (c) 2015-2021, Simulation and Data Laboratory Quantum Materials,
//   Forschungszentrum Juelich GmbH, Germany. All rights reserved.
// License is 3-clause BSD:
# ifndef  PAID_INTEGRATOR_RULE_HPP
# define PAID_INTEGRATOR_RULE_HPP

#pragma once

#include <assert.h>
#include <cmath>
#include <vector>

#include "invoker.hpp"
#include "types.hpp"

namespace paid {

// an integration rule
/*
class IntegrationRule {
 public:
};
*/

class ClenshawCurtis {
 public:
  explicit ClenshawCurtis(std::size_t N) noexcept // constructor calculates points t, t2 where to evaluate integrand with correpsonding weights w, w2
      : N_(N), w(N + 1), w2(2 * N + 1), t(N + 1), t2(2 * N + 1) {
    std::size_t j, k;

    std::vector<std::vector<double>> z(N, std::vector<double>(N + 1));
    std::vector<std::vector<double>> z2(2 * N, std::vector<double>(2 * N + 1));

    // z, z2, just notation to help to compute the weights
    for (j = 0; j < N_; ++j)
      for (k = 0; k <= N_; ++k) {
        z[j][k] = std::cos(k * j * M_PI / N_) / N_;
        if (k != 0 && k != N_) z[j][k] *= 2.0;
        if (j == 0)
          z[j][k] /= 2.0;
        else if (j % 2 == 0)
          z[j][k] /= ((j + 1) * (1 - j));
      }

    for (j = 0; j < 2 * N_; ++j)
      for (k = 0; k <= 2 * N_; ++k) {
        z2[j][k] = std::cos(k * j * M_PI / (2 * N_)) / (2 * N_);
        if (k != 0 && k != (2 * N_)) z2[j][k] *= 2.0;
        if (j == 0)
          z2[j][k] /= 2.0;
        else if (j % 2 == 0)
          z2[j][k] /= ((j + 1) * (1 - j));
      }

    // weights for N_ and 2N_
    for (k = 0; k <= N_; ++k) {
      w[k] = 0.0;
      for (j = 0; j < N_; j += 2) w[k] += z[j][k];
    }

    for (k = 0; k <= 2 * N_; ++k) {
      w2[k] = 0.0;
      for (j = 0; j < 2 * N_; j += 2) w2[k] += z2[j][k];
    }

    std::size_t npts = N_;

    for (k = 0; k <= npts; ++k) {
      t[k] = std::cos((M_PI * k) / static_cast<double>(npts));
      if (k == 0 || k == npts) {
        w[k] = 1.0 / (npts * npts - 1.0);
      } else {
        w[k] = 1.0 + std::cos(k * M_PI) / (1.0 - npts * npts);
        for (j = 1; j <= npts / 2 - 1; j++) {
          w[k] += (2.0 / (1.0 - 4.0 * j * j)) *
                  std::cos((2.0 * j) * (M_PI * k) / npts);
        }
        w[k] *= 2.0 / npts;
      }
    }

    npts = 2 * N_;

    for (k = 0; k <= npts; ++k) {
      t2[k] = std::cos((M_PI * k) / static_cast<double>(npts));
      if (k == 0 || k == npts) {
        w2[k] = 1.0 / (npts * npts - 1.0);
      } else {
        w2[k] = 1.0 + std::cos(k * M_PI) / (1.0 - npts * npts);
        for (j = 1; j <= npts / 2 - 1; j++) {
          w2[k] += (2.0 / (1.0 - 4.0 * j * j)) *
                   std::cos((2.0 * j) * (M_PI * k) / npts);
        }
        w2[k] *= 2.0 / npts;
      }
    }
  }

  // 1D-case
  /*
  template <std::size_t Dim, typename F, typename T, typename... Args>
  IntegrationResult<T> apply1d(const F& f,
                             const std::array<AffineTransform1D, Dim>& af) const
      noexcept {
    T val1;
    T val2;
    const std::size_t nEvals = 2 * N_ + 1;
    std::vector<T> cache_(N_ + 1);  // TODO

    val1 = 0;
    for (std::size_t k = 0; k < N_ + 1; ++k) {
      double node = af[0](t[k]);
      cache_[k] =
          Invoker<Dim, F, T, Args...>::apply(f, std::array<double, 1>{{node}});
      val1 += w[k] * cache_[k];
    }

    val2 = 0;
    for (std::size_t k = 0; k < 2 * N_ + 1; ++k) {
      if (k % 2 == 0) {
        val2 += w2[k] * cache_[k / 2];
        assert(t[k / 2] == t2[k]);
      } else {
        // val2 += w2[k] * f(af(t2[k]));
        double node = af[0](t2[k]);
        val2 += w2[k] * Invoker<Dim, F, T, Args...>::apply(
                            f, std::array<double, 1>{{node}});
      }
    }

    assert(!std::isnan(std::abs(val2)));
    assert(!std::isnan(std::abs(val1)));

    // due to affine transformation there is a factor (right-left)/2 for the integral value which corresponds to af[0](1)-af[0](0)
    return {nEvals, val2 * (af[0](1) - af[0](0)), std::abs(val2 - val1)};
  };
  */

  // 2D-case
  /*
  template <std::size_t 2, typename F, typename T, typename... Args>
  IntegrationResult<T> apply2D(const F& f,
                             const std::array<AffineTransform1D, Dim>& af) const
    noexcept {
      T val1;
      T val2;
      const std::size_t nEvals = pow(2 * N_ + 1,Dim); // was 2*(N_+1) before, but it should be 2*N_+1
      std::vector<std::vector<T>> cache_(N_ + 1, std::vector<T>(N_+1));

      for (std::size_t kx = 0; kx <= N_; ++kx) {
          for (std::size_t ky = 0; ky <= N_; ++ky){
              auto nodex = af[0](t[kx]);
              auto nodey = af[1](t[ky]);
              cache_[kx,ky] =
                      Invoker<Dim, F, T, Args...>::apply(f, std::array<double, 2>{{nodex,nodey}});
              val1 += w[kx] * w[ky] * cache_[kx,ky];
          }
      }

      val2 = 0;
      for (std::size_t kx = 0; kx <= 2 * N_; ++kx) {
          for (std::size_t ky = 0; ky <= 2 * N_; ++ky) {
              if ((kx % 2 == 0) and (ky % 2 == 0) {
                  val2 += w2[kx] * w2[ky] * cache_[kx / 2, ky / 2];
                  assert(t[kx / 2] == t2[kx]);
                  assert(t[ky / 2] == t2[ky]);
              } else {
                  // val2 += w2[k] * f(af(t2[k]));
                  auto nodex = af[0](t2[kx]);
                  auto nodex = af[1](t2[ky]);
                  val2 += w2[kx] w2[ky] * Invoker<Dim, F, T, Args...>::apply(
                          f, std::array<double, 2>{{nodex,nodey}});
              }
          }
      }

      assert(!std::isnan(std::abs(val2)));
      assert(!std::isnan(std::abs(val1)));

      // due to affine transformation there is a factor (right-left)/2 for the integral value which corresponds to af[0](1)-af[0](0)
      return {nEvals, val2 * (af[0](1) - af[0](0)) * (af[1](1) - af[1](0)), std::abs(val2 - val1)};
  };*/

  // general case
  template <std::size_t Dim, typename F, typename T, typename... Args>
  IntegrationResult<T> apply(const F& f,
                             const std::array<AffineTransform1D, Dim>& af) const
    noexcept {
    T val1;
    T val2;
    const std::size_t nEvals = pow(2 * N_ + 1, Dim); // was 2*(N_+1) before, but it should be 2*N_+1
    std::vector<T> cache_(pow(N_ + 1, Dim)); //static_cast<std::size_t>(pow(N_ + 1, Dim))

    std::array<std::size_t,Dim> k_vector, k_vector_half;
    std::array<double, Dim> node;
    double wtot, w2tot;

    val1 = 0;
    for (std::size_t k = 0; k < pow(N_ + 1, Dim); ++k) {
        wtot = 1;
        k_vector = get_each_index<Dim>(N_+1,k);
        for (std::size_t n = 0; n < Dim; ++n){
            node[n] = af[n](t[k_vector[n]]);
            wtot *= w[k_vector[n]];
        }
        //auto node = af[0](t[k]);
        cache_[k] =
            Invoker<Dim, F, T, Args...>::apply(f, node);
        val1 += wtot * cache_[k]; //val1 += w[k] * cache_[k];
    }

    val2 = 0;
    for (std::size_t k = 0; k < pow(2 * N_ + 1,Dim); ++k) {
        w2tot = 1;
        k_vector = get_each_index<Dim>(2*N_+1,k);
        std::size_t k_mod = 0;
        for (std::size_t n = 0; n < Dim; ++n){
            k_mod += k_vector[n] % 2;
        }
        if (k_mod == 0) {// (k % 2 == 0) {
            for (std::size_t n = 0; n < Dim; ++n){
                w2tot *= w2[k_vector[n]];
                k_vector_half[n] = k_vector[n]/2;
                assert(t[k_vector_half[n]] == t2[k_vector[n]]);
            }
            std::size_t k_half = get_composite_index<Dim>(N_+1,k_vector_half);
            val2 += w2tot * cache_[k_half]; //val2 += w2[k] * cache_[k / 2];
            // assert(t[k / 2] == t2[k]);
        } else {
            // val2 += w2[k] * f(af(t2[k]));
            for (std::size_t n = 0; n < Dim; ++n){
                node[n] = af[n](t2[k_vector[n]]);
                w2tot *= w2[k_vector[n]];
            }
            //auto node = af[0](t2[k]);
            val2 += w2tot * Invoker<Dim, F, T, Args...>::apply(f, node);
        }
    }

    assert(!std::isnan(std::abs(val2)));
    assert(!std::isnan(std::abs(val1)));

    double substitution_factor = 1;
    for (std::size_t n = 0; n < Dim; ++n){
        substitution_factor *= (af[n](1) - af[n](0));
    }

    // due to affine transformation there is a factor (right-left)/2 for the integral value which corresponds to af[0](1)-af[0](0)
    //T value_integral = val2*substitution_factor;
    //double error_integral = std::abs(val2-val1);
    return {nEvals, val2*substitution_factor, std::abs(val2-val1)};
  };

  /*
  template <typename F, typename T, typename... Args>
  IntegrationResult<T> apply<1, F, T, Args...>(const F& f,
                             const std::array<AffineTransform1D, 1>& af) const
                             noexcept {
      T val1;
      T val2;
      const std::size_t nEvals = 2 * (N_ + 1);
      std::vector<T> cache_(N_ + 1);  // TODO

      for (std::size_t k = 0; k <= N_; ++k) {
          auto node = af[0](t[k]);
          cache_[k] =
                  Invoker<1, F, T, Args...>::apply(f, std::array<double, 1>{{node}});
          val1 += w[k] * cache_[k];
      }

      val2 = 0;
      for (std::size_t k = 0; k <= 2 * N_; ++k) {
          if (k % 2 == 0) {
              val2 += w2[k] * cache_[k / 2];
              assert(t[k / 2] == t2[k]);
          } else {
              // val2 += w2[k] * f(af(t2[k]));
              auto node = af[0](t2[k]);
              val2 += w2[k] * Invoker<1, F, T, Args...>::apply(
                      f, std::array<double, 1>{{node}});
          }
      }

      assert(!std::isnan(std::abs(val2)));
      assert(!std::isnan(std::abs(val1)));

      // due to affine transformation there is a factor (right-left)/2 for the integral value which corresponds to af[0](1)-af[0](0)
      return {nEvals, val2 * (af[0](1) - af[0](0)), std::abs(val2 - val1)};
  };
     */


 private:
  std::size_t N_;
  std::vector<double> w;
  std::vector<double> w2;
  std::vector<double> t;
  std::vector<double> t2;
};

}  // namespace paid

#endif