#ifndef FPP_MFRG_VERTEX_BUFFER_H
#define FPP_MFRG_VERTEX_BUFFER_H

#include <cassert>
//#include "interpolations/vertex_interpolations.h"
#include "../../interpolations/InterpolatorSpline1D.hpp"
#include "../../interpolations/InterpolatorSpline2D.hpp"
#include "../../interpolations/InterpolatorSpline3D.hpp"
#include "../../interpolations/InterpolatorLinOrSloppy.hpp"
#include "../../symmetries/symmetry_table.hpp"
#include "../../symmetries/symmetry_transformations.hpp"

/// Possible (unit-) tests for the vertex buffer:
/// [IMPLEMENTED in unit_tests/test_interpolations] reliability of interpolation --> fill vertex buffer with polynomial and interpolate
/*
enum vectypes {scalar, vec_left, vec_right};
*/
/**
 * Vertex buffers store the vertex data and frequency grids and interpolates data points on the grid.
 */
 /*
template<K_class k, typename Q, interpolMethod inter>
class vertexBuffer {
    explicit vertexBuffer(double Lambda) {
        assert(false);
    }
};




template <K_class k, typename Q>
class vertexBuffer<k, Q, linear> : public vertexDataContainer<k,Q> {
    using base_class = vertexDataContainer<k, Q>;
    static constexpr my_index_t numSamples() { /// TODO: adapt for non-trivial internal index
        if constexpr(k == k1) {return 2;}
        else if constexpr (k == k3) {return 8;}
        else {return 4;}
    }
    static constexpr my_index_t numSamples_half() {    /// TODO: adapt for non-trivial internal index
        if constexpr(k == k1) {return 1;}
        else if constexpr (k == k3) {return 4;}
        else {return 2;}
    }
    static constexpr my_index_t frequencydims() {
        if constexpr(k == k1) {return 1;}
        else if constexpr (k == k3) {return 3;}
        else {return 2;}
    }
    template <typename result_type>
    static constexpr my_index_t get_vecsize() {
        if constexpr(std::is_same_v<result_type, Q>) return 1;
        else { return 4; }  /// TODO: adapt for non-trivial internal index
    }
public:
    using index_type = typename base_class::index_type;
    using dimensions_type = typename base_class::dimensions_type;
    using frequency_arr = std::array<double, frequencydims()>;

    mutable bool initialized = false;
    vertexBuffer<k,Q,linear>() : initialized(false) {};
    explicit vertexBuffer<k, Q, linear>(double Lambda, dimensions_type dims) : base_class(Lambda, dims) {};
    void initInterpolator() const {initialized = true;};

    bool is_in_box(const VertexInput& indices) const {
        if constexpr(k == k1) return std::abs(indices.w) < base_class::frequencies.b.w_upper + inter_tol;
        else if constexpr(k == k2) return (std::abs(indices.w) < base_class::frequencies.b.w_upper + inter_tol) and (std::abs(indices.v1) < base_class::frequencies.f.w_upper + inter_tol);
        else if constexpr(k == k2b)return (std::abs(indices.w) < base_class::frequencies.b.w_upper + inter_tol) and (std::abs(indices.v2) < base_class::frequencies.f.w_upper + inter_tol);
        else return (std::abs(indices.w) < base_class::frequencies.b.w_upper + inter_tol) and (std::abs(indices.v1) < base_class::frequencies.f.w_upper + inter_tol) and (std::abs(indices.v2) < base_class::frequencies.f.w_upper + inter_tol);
    }


    Eigen::Matrix<double, numSamples(), 1> get_weights(const VertexInput& indices, index_type& idx_low) const {
        Eigen::Matrix<double, numSamples(), 1> weights;

        my_index_t iw = base_class::frequencies.b.fconv(indices.w);
        idx_low[0] = iw;
        double w_low = base_class::frequencies.b.get_ws(iw);
        double w_high= base_class::frequencies.b.get_ws(iw+1);
        const double weight_w = (indices.w - w_low) / (w_high - w_low);
        for (int j = 0; j < numSamples_half(); j++) {
            weights[j  ] = 1-weight_w;
            weights[numSamples_half()+j] = weight_w;
        }

        if constexpr (k == k1) {
            //idx_low = {indices.spin, iw, (my_index_t)indices.iK, indices.i_in};
            idx_low[my_defs::K1::spin] = indices.spin;
            idx_low[my_defs::K1::keldysh] = (my_index_t) indices.iK;
            idx_low[my_defs::K1::omega] = iw;
            idx_low[my_defs::K1::internal] = indices.i_in;
            return weights;
        }
        if constexpr (k == k2) {
            my_index_t iv = base_class::frequencies.f.fconv(indices.v1);
            idx_low[0] = iv;
            double v_low = base_class::frequencies.f.get_ws(iv);
            double v_high= base_class::frequencies.f.get_ws(iv+1);
            const double weight_v = (indices.v1 - v_low) / (v_high - v_low);
            for (int j = 0; j < numSamples_half(); j++) {
                weights[2*j  ] *= 1-weight_v;
                weights[2*j+1] *= weight_v;
            }
            //idx_low = {indices.spin, iw, iv, (my_index_t)indices.iK, indices.i_in};
            idx_low[my_defs::K2::spin] = indices.spin;
            idx_low[my_defs::K2::keldysh] = (my_index_t) indices.iK;
            idx_low[my_defs::K2::omega] = iw;
            idx_low[my_defs::K2::nu] = iv;
            idx_low[my_defs::K2::internal] = indices.i_in;

            return weights;
        }
        if constexpr(k == k2b) {
            my_index_t ivp = base_class::frequencies.f.fconv(indices.v2);
            double vp_low = base_class::frequencies.f.get_ws(ivp);
            double vp_high= base_class::frequencies.f.get_ws(ivp+1);
            const double weight_vp = (indices.v2 - vp_low) / (vp_high - vp_low);
            for (int j = 0; j < numSamples_half(); j++) {
                weights[2*j  ] *= 1-weight_vp;
                weights[2*j+1] *= weight_vp;
            }
            //idx_low = {indices.spin, iw, ivp, (my_index_t)indices.iK, indices.i_in};
            idx_low[my_defs::K2b::spin] = indices.spin;
            idx_low[my_defs::K2b::keldysh] = (my_index_t) indices.iK;
            idx_low[my_defs::K2b::omega] = iw;
            idx_low[my_defs::K2b::nup] = ivp;
            idx_low[my_defs::K2b::internal] = indices.i_in;

            return weights;
        }
        if constexpr(k == k3) {
            my_index_t iv = base_class::frequencies.f.fconv(indices.v1);
            double v_low = base_class::frequencies.f.get_ws(iv);
            double v_high= base_class::frequencies.f.get_ws(iv+1);
            const double weight_v = (indices.v1 - v_low) / (v_high - v_low);

            my_index_t ivp = base_class::frequencies.f.fconv(indices.v2);
            double vp_low = base_class::frequencies.f.get_ws(ivp);
            double vp_high= base_class::frequencies.f.get_ws(ivp+1);
            const double weight_vp= (indices.v2 - vp_low) / (vp_high - vp_low);

            const std::array<double, 2> single_weights = {weight_v, weight_vp};
            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 2; i++) {
                    weights[4*j+i]   *= 1-weight_v;
                    weights[4*j+i+2] *= weight_v;
                }
            }
            for (int j = 0; j < numSamples_half(); j++) {
                weights[j*2]   *= 1-weight_vp;
                weights[1+j*2] *= weight_vp;
            }

            //idx_low = {indices.spin, iw, iv, ivp, (my_index_t) indices.iK, indices.i_in};
            idx_low[my_defs::K3::spin] = indices.spin;
            idx_low[my_defs::K3::keldysh] = (my_index_t) indices.iK;
            idx_low[my_defs::K3::omega] = iw;
            idx_low[my_defs::K3::nu] = iv;
            idx_low[my_defs::K3::nup]= ivp;
            idx_low[my_defs::K3::internal] = indices.i_in;
            return weights;
        }



    }

    template <typename result_type, my_index_t vecsize>
    Eigen::Matrix<Q, vecsize, numSamples()> get_values(const index_type& vertex_index) const {
        Eigen::Matrix<Q, vecsize, numSamples()> result;
    if constexpr(std::is_same_v<result_type, Q>) {
        if constexpr (k == k1) {
            for (int i = 0; i < 2; i++) {
                result[i] = base_class::val(vertex_index[0], vertex_index[1] + i, vertex_index[2], vertex_index[3]);
            }
        } else if constexpr (k == k3) {
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int l = 0; l < 2; l++) {
                        result[i * 4 + j * 2 + l] = base_class::val(vertex_index[0], vertex_index[1] + i,
                                                                    vertex_index[2] + j, vertex_index[3] + l,
                                                                    vertex_index[4],
                                                                    vertex_index[5]);
                    }
                }
            }
        } else { // k2 and k2b
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    result[i * 2 + j] = base_class::val(vertex_index[0], vertex_index[1] + i, vertex_index[2] + j,
                                                        vertex_index[3],
                                                        vertex_index[4]);

                }
            }
        }

    }
    else { // typ == vec_left or vec_right

        if constexpr (k == k1) {
            for (int i = 0; i < 2; i++) {
                auto res = base_class::template val_vectorized<frequencydims(),vecsize>(vertex_index[0], vertex_index[1] + i, vertex_index[2], vertex_index[3]);
                result.col(i) = res;
            }
        } else if constexpr (k == k3) {
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int l = 0; l < 2; l++) {
                        result.col(i * 4 + j * 2 + l) = base_class::template val_vectorized<frequencydims(),vecsize>(vertex_index[0], vertex_index[1] + i,
                                                                                                                     vertex_index[2] + j, vertex_index[3] + l, vertex_index[4], vertex_index[5]);
                    }
                }
            }
        } else {
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    result.col(i * 2 + j) = base_class::template val_vectorized<frequencydims(),vecsize>(vertex_index[0], vertex_index[1] + i, vertex_index[2] + j, vertex_index[3], vertex_index[4]);

                }
            }
        }

    }



        return result;
    }




    template <typename result_type = Q>
    auto interpolate(const VertexInput &indices) const -> result_type {

        constexpr my_index_t vecsize = get_vecsize<result_type>();
        using weights_type = Eigen::Matrix<double, numSamples(), 1>;
        using values_type = Eigen::Matrix<Q, vecsize, numSamples()>;

        // Check if the frequency runs out of the box; if yes: return asymptotic value
        if (is_in_box(indices))
        {

#ifdef DENSEGRID
            Q result;
            if constexpr(k == k1)
            {
                result = interpolate_nearest1D<Q>(indices.w, base_class::get_VertexFreqGrid().b,
                                                  [&](int i) -> Q {
                                                      return base_class::val(indices.spin, i,
                                                                                             indices.iK,
                                                                                             indices.i_in);
                                                  });
            }
            else if constexpr(k==k2){
                result =  interpolate_nearest2D<Q>(indices.w, indices.v1,
                                                   base_class::get_VertexFreqGrid().b,
                                                   base_class::get_VertexFreqGrid().f,
                                                   [&](int i, int j) -> Q {
                                                       return base_class::val(indices.spin, i,
                                                                                              j, indices.iK,
                                                                                              indices.i_in);
                                                   });
            }
            else if constexpr(k==k2b){
                result =  interpolate_nearest2D<Q>(indices.w, indices.v2,
                                                   base_class::get_VertexFreqGrid().b,
                                                   base_class::get_VertexFreqGrid().f,
                                                   [&](int i, int j) -> Q {
                                                       return base_class::val(indices.spin, i,
                                                                                              j, indices.iK,
                                                                                              indices.i_in);
                                                   });
            }
            else if constexpr(k == k3)
            {
                result =  interpolate_nearest3D<Q>(indices.w, indices.v1, indices.v2,
                                                   base_class::get_VertexFreqGrid().b,
                                                   base_class::get_VertexFreqGrid().f,
                                                   base_class::get_VertexFreqGrid().f,
                                                   [&](int i, int j, int l) -> Q {
                                                       return base_class::val(indices.spin, i,
                                                                                              j, l, indices.iK,
                                                                                              indices.i_in);
                                                   });
            }

            return result;
#else
            // get weights from frequency Grid

            index_type vertex_index;
            weights_type weights = get_weights(indices, vertex_index);
            // fetch vertex values
            values_type values = get_values<result_type, vecsize>(vertex_index);
            assert(weights.allFinite());
            assert(values.allFinite());

            if constexpr(std::is_same_v<result_type, Q>) {
                Q result = values * weights;
                assert(isfinite(result));
                return result;
            }
            else {
                Eigen::Matrix<Q, vecsize,1> result = values * weights;
                assert(result.allFinite());
                return result;
            }


#endif

            //assert(isfinite(result));
            //return result;

        } else { //asymptotic value
            if constexpr (std::is_same_v<result_type,Q>) return result_type{};
            else return result_type::Zero();

        }

    };

    auto operator+= (const vertexBuffer<k,Q,linear>& rhs) -> vertexBuffer<k,Q,linear> {vertexDataContainer<k,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k,Q,linear>& rhs) -> vertexBuffer<k,Q,linear> {vertexDataContainer<k,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k,Q,linear>& operator+ (vertexBuffer<k,Q,linear>& lhs, const vertexBuffer<k,Q,linear>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k,Q,linear>& operator- (vertexBuffer<k,Q,linear>& lhs, const vertexBuffer<k,Q,linear>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};


*/
/*
/// Template specialization for K1 (linear interpolation on the auxiliary grid)
template<typename Q, interpolMethod inter>
class vertexBuffer<k1, Q, inter> : public vertexDataContainer<k1, Q> {
    using base_class = vertexDataContainer<k1, Q>;
public:
    using index_type = typename base_class::index_type;
    mutable bool initialized = false;

    vertexBuffer<k1,Q,inter>() : initialized(false) {};
    explicit vertexBuffer<k1, Q, inter>(double Lambda, index_type dims) : vertexDataContainer<k1, Q>(Lambda, dims) {};

    void initInterpolator() const {initialized = true;};

    auto interpolate(const VertexInput &indices) const -> Q {


        // Check if the frequency runs out of the box; if yes: return asymptotic value
        if (std::abs(indices.w) < vertexDataContainer<k1, Q>::frequencies.b.w_upper + inter_tol)
        {

            Q result;
#ifdef DENSEGRID
            result =  interpolate_nearest1D<Q>(indices.w, vertexDataContainer<k1, Q>::get_VertexFreqGrid().b,
                                                  [&](int i) -> Q {
                                                      return vertexDataContainer<k1, Q>::val(indices.spin, i, indices.iK,
                                                                                             indices.i_in);
                                                  });
#else
            if constexpr(inter == linear_on_aux) {
                result =  interpolate_lin_on_aux1D<Q>(indices.w, vertexDataContainer<k1, Q>::get_VertexFreqGrid().b,
                                                        [&](int i) -> Q {
                                                            return vertexDataContainer<k1, Q>::val(indices.spin, i, indices.iK,
                                                                                                   indices.i_in);
                                                        });
            }
            else if constexpr(inter == linear) {
                result =  interpolate_lin1D<Q>(indices.w, vertexDataContainer<k1, Q>::get_VertexFreqGrid().b,
                                                      [&](int i) -> Q {
                                                          return vertexDataContainer<k1, Q>::val(indices.spin, i, indices.iK,
                                                                                                 indices.i_in);
                                                      });
            }
            else if constexpr(inter == sloppycubic) {
                result =  interpolate_sloppycubic1D<Q>(indices.w, vertexDataContainer<k1, Q>::get_VertexFreqGrid().b,
                                                   [&](int i) -> Q {
                                                       return vertexDataContainer<k1, Q>::val(indices.spin, i, indices.iK,
                                                                                              indices.i_in);
                                                   });
            }
            else assert(false);
            // Lambda function (aka anonymous function) in last argument
#endif

            assert(isfinite(result));
            return result;

        } else {
            return 0.;  // asymptotic value
        }

    };

    auto operator+= (const vertexBuffer<k1,Q,inter>& rhs) -> vertexBuffer<k1,Q,inter> {vertexDataContainer<k1,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k1,Q,inter>& rhs) -> vertexBuffer<k1,Q,inter> {vertexDataContainer<k1,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k1,Q,inter>& operator+ (vertexBuffer<k1,Q,inter>& lhs, const vertexBuffer<k1,Q,inter>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k1,Q,inter>& operator- (vertexBuffer<k1,Q,inter>& lhs, const vertexBuffer<k1,Q,inter>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/// Template specialization for K2 (linear interpolation on the auxiliary grid)
template<typename Q, interpolMethod inter>
class vertexBuffer<k2, Q, inter> : public vertexDataContainer<k2, Q> {
    using base_class = vertexDataContainer<k2, Q>;
public:
    using index_type = typename base_class::index_type;
    vertexBuffer<k2,Q,inter>() : initialized(false) {};
    explicit vertexBuffer<k2, Q, inter>(double Lambda, index_type dims) : vertexDataContainer<k2, Q>(Lambda, dims) {};
    mutable bool initialized = false;

    void initInterpolator() const {initialized = true;};
    // Template class call operator: used for K2 and K2b. For K1 and K3: template specializations (below)
    auto interpolate(const VertexInput &indices) const -> Q {

        double w_temp = indices.w;
        double v_temp = indices.v1;
        K2_convert2internalFreqs(w_temp, v_temp); // convert natural frequency parametrization in channel r to internal parametrization


        // Check if the frequency runs out of the box; if yes: return asymptotic value
        if (    std::abs(indices.w ) < vertexDataContainer<k2, Q>::frequencies.b.w_upper + inter_tol
                && std::abs(indices.v1) < vertexDataContainer<k2, Q>::frequencies.f.w_upper + inter_tol )
        {

            Q result;
#ifdef DENSEGRID
            result =  interpolate_nearest2D<Q>(w_temp, v_temp,
                                               vertexDataContainer<k2, Q>::get_VertexFreqGrid().b,
                                               vertexDataContainer<k2, Q>::get_VertexFreqGrid().f,
                                               [&](int i, int j) -> Q {
                                                   return vertexDataContainer<k2, Q>::val(indices.spin, i,
                                                                                           j, indices.iK,
                                                                                           indices.i_in);
                                               });
#else
            if constexpr(inter == linear_on_aux) {
                result =  interpolate_lin_on_aux2D<Q>(w_temp, v_temp,
                                                      vertexDataContainer<k2, Q>::get_VertexFreqGrid().b,
                                                      vertexDataContainer<k2, Q>::get_VertexFreqGrid().f,
                                                        [&](int i, int j) -> Q {
                                                            return vertexDataContainer<k2, Q>::val(indices.spin, i,
                                                                                                   j, indices.iK,
                                                                                                   indices.i_in);
                                                        });
            }
            else if constexpr(inter == linear) {
                result =  interpolate_lin2D<Q>(w_temp, v_temp,
                                               vertexDataContainer<k2, Q>::get_VertexFreqGrid().b,
                                               vertexDataContainer<k2, Q>::get_VertexFreqGrid().f,
                                                        [&](int i, int j) -> Q {
                                                            return vertexDataContainer<k2, Q>::val(indices.spin, i,
                                                                                                   j, indices.iK,
                                                                                                   indices.i_in);
                                                        });
            }
            else if constexpr(inter == sloppycubic) {

                result =  interpolate_sloppycubic2D<Q>(w_temp, v_temp,
                                                       vertexDataContainer<k2, Q>::get_VertexFreqGrid().b,
                                                       vertexDataContainer<k2, Q>::get_VertexFreqGrid().f,
                                                     [&](int i, int j) -> Q {
                                                         return vertexDataContainer<k2, Q>::val(indices.spin, i,
                                                                                                j, indices.iK,
                                                                                                indices.i_in);
                                                     });
            }
            else assert(false);
#endif // DENSEGRID
            assert(isfinite(result));
            return result;

        }
        else {
            return 0.;      // asymptotic value
        }

    }

    auto operator+= (const vertexBuffer<k2,Q,inter>& rhs) -> vertexBuffer<k2,Q,inter> {vertexDataContainer<k2,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k2,Q,inter>& rhs) -> vertexBuffer<k2,Q,inter> {vertexDataContainer<k2,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k2,Q,inter>& operator+ (vertexBuffer<k2,Q,inter>& lhs, const vertexBuffer<k2,Q,inter>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k2,Q,inter>& operator- (vertexBuffer<k2,Q,inter>& lhs, const vertexBuffer<k2,Q,inter>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/// Template specialization for K2b(linear interpolation on the auxiliary grid)
template<typename Q, interpolMethod inter>
class vertexBuffer<k2b, Q, inter> : public vertexDataContainer<k2b, Q> {
    using base_class = vertexDataContainer<k2b, Q>;
public:
    using index_type = typename base_class::index_type;
    vertexBuffer<k2b,Q,inter>() : initialized(false) {};
    explicit vertexBuffer<k2b, Q, inter>(double Lambda, index_type dims) : base_class(Lambda, dims) {};
    mutable bool initialized = false;

    void initInterpolator() const {initialized = true;};
    // Template class call operator: used for K2 and K2b. For K1 and K3: template specializations (below)
    auto interpolate(const VertexInput &indices) const -> Q {

        double w_temp = indices.w;
        double v_temp = indices.v2;
        K2_convert2internalFreqs(w_temp, v_temp); // convert natural frequency parametrization in channel r to internal parametrization


        // Check if the frequency runs out of the box; if yes: return asymptotic value
        if (    std::abs(indices.w ) < vertexDataContainer<k2b, Q>::frequencies.b.w_upper + inter_tol
                && std::abs(indices.v2) < vertexDataContainer<k2b, Q>::frequencies.f.w_upper + inter_tol )
        {

            Q result;

#ifdef DENSEGRID
            result =  interpolate_nearest2D<Q>(w_temp, v_temp,
                                               vertexDataContainer<k2b, Q>::get_VertexFreqGrid().b,
                                               vertexDataContainer<k2b, Q>::get_VertexFreqGrid().f,
                                               [&](int i, int j) -> Q {
                                                   return vertexDataContainer<k2b, Q>::val(indices.spin, i,
                                                                                           j, indices.iK,
                                                                                           indices.i_in);
                                               });
#else
            if constexpr(inter == linear_on_aux){
                result =  interpolate_lin_on_aux2D<Q>(w_temp, v_temp,
                                                      vertexDataContainer<k2b, Q>::get_VertexFreqGrid().b,
                                                      vertexDataContainer<k2b, Q>::get_VertexFreqGrid().f,
                                                      [&](int i, int j) -> Q {
                                                          return vertexDataContainer<k2b, Q>::val(indices.spin, i,
                                                                                                  j, indices.iK,
                                                                                                  indices.i_in);
                                                      });
            }
            else if constexpr(inter == linear) {
                result =  interpolate_lin2D<Q>(w_temp, v_temp,
                                               vertexDataContainer<k2b, Q>::get_VertexFreqGrid().b,
                                               vertexDataContainer<k2b, Q>::get_VertexFreqGrid().f,
                                                      [&](int i, int j) -> Q {
                                                          return vertexDataContainer<k2b, Q>::val(indices.spin, i,
                                                                                                  j, indices.iK,
                                                                                                  indices.i_in);
                                                      });
            }
            else if constexpr(inter == sloppycubic) {
                result =  interpolate_sloppycubic2D<Q>(w_temp, v_temp,
                                                       vertexDataContainer<k2b, Q>::get_VertexFreqGrid().b,
                                                       vertexDataContainer<k2b, Q>::get_VertexFreqGrid().f,
                                               [&](int i, int j) -> Q {
                                                   return vertexDataContainer<k2b, Q>::val(indices.spin, i,
                                                                                           j, indices.iK,
                                                                                           indices.i_in);
                                               });
            }
            else assert(false);
#endif // DENSEGRID
            assert(isfinite(result));
            return result;

        }
        else {
            return 0.;      // asymptotic value
        }

    }

    auto operator+= (const vertexBuffer<k2b,Q,inter>& rhs) -> vertexBuffer<k2b,Q,inter> {vertexDataContainer<k2b,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k2b,Q,inter>& rhs) -> vertexBuffer<k2b,Q,inter> {vertexDataContainer<k2b,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k2b,Q,inter>& operator+ (vertexBuffer<k2b,Q,inter>& lhs, const vertexBuffer<k2b,Q,inter>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k2b,Q,inter>& operator- (vertexBuffer<k2b,Q,inter>& lhs, const vertexBuffer<k2b,Q,inter>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/// Template specialization for K3 (linear interpolation on the auxiliary grid)
template<typename Q, interpolMethod inter>
class vertexBuffer<k3, Q, inter> : public vertexDataContainer<k3, Q> {
    using base_class = vertexDataContainer<k3, Q>;
public:
    using index_type = typename base_class::index_type;
    mutable bool initialized = false;

    void initInterpolator() const {initialized = true;};
    vertexBuffer<k3,Q,inter>() : initialized(false), vertexDataContainer<k3, Q>() {};
    explicit vertexBuffer<k3, Q, inter>(double Lambda, index_type dims) : vertexDataContainer<k3, Q>(Lambda, dims) {};

    auto interpolate(const VertexInput &indices) const -> Q {
        double w_temp = indices.w;
        double v_temp = indices.v1;
        double vp_temp= indices.v2;
        if (BOSONIC_PARAM_FOR_K3) {
            assert(false); //
            // if (indices.channel_rvert == 'a') { switch2bosonicFreqs<'a'>(w_temp, v_temp, vp_temp); }
            // else if (indices.channel_rvert == 'p') { switch2bosonicFreqs<'p'>(w_temp, v_temp, vp_temp); }
            // else if (indices.channel_rvert == 't') { switch2bosonicFreqs<'t'>(w_temp, v_temp, vp_temp); }
        }

        // Check if the frequency runs out of the box; if yes: return asymptotic value
        if (std::abs(indices.w) < vertexDataContainer<k3, Q>::frequencies.b.w_upper + inter_tol
            && std::abs(indices.v1) < vertexDataContainer<k3, Q>::frequencies.f.w_upper + inter_tol
            && std::abs(indices.v2) < vertexDataContainer<k3, Q>::frequencies.f.w_upper + inter_tol)
        {

            Q result;


#ifdef DENSEGRID
            result =  interpolate_nearest3D<Q>(w_temp, v_temp, vp_temp,
                                               vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                               vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                               vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                               [&](int i, int j, int k) -> Q {
                                                   return vertexDataContainer<k3, Q>::val(indices.spin, i,
                                                                                          j, k, indices.iK,
                                                                                          indices.i_in);
                                               });
#else
            if (not INTERPOL2D_FOR_K3 or indices.kClass_aim != k3) {

                if constexpr(inter == linear_on_aux) {
                    result = interpolate_lin_on_aux3D<Q>(w_temp, v_temp, vp_temp,
                                                         vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                         vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                         vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                         [&](int i, int j, int k) -> Q {
                                                             return vertexDataContainer<k3, Q>::val(indices.spin, i,
                                                                                                    j, k, indices.iK,
                                                                                                    indices.i_in);
                                                         });
                }
                else if constexpr(inter == linear) {
                    result = interpolate_lin3D<Q>(w_temp, v_temp, vp_temp,
                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                         [&](int i, int j, int k) -> Q {
                                                             return vertexDataContainer<k3, Q>::val(indices.spin, i,
                                                                                                    j, k, indices.iK,
                                                                                                    indices.i_in);
                                                         });
                }
                else if constexpr(inter == sloppycubic) {
                    result = interpolate_sloppycubic3D<Q>(w_temp, v_temp, vp_temp,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().f,
                                                  [&](int i, int j, int k) -> Q {
                                                      return vertexDataContainer<k3, Q>::val(indices.spin, i,
                                                                                             j, k, indices.iK,
                                                                                             indices.i_in);
                                                  });
                }
                else assert(false);


            }

            else {

                if constexpr(inter == linear_on_aux) {
                    switch (indices.channel_bubble) {
                        case 'a':
                            result = interpolate_lin_on_aux2D<Q>(v_temp, vp_temp,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int j, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, indices.iw_r,j, k, indices.iK, indices.i_in);});
                            break;
                        case 'p':
                            result = interpolate_lin_on_aux2D<Q>(w_temp, vp_temp,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i, indices.iw_r, k,indices.iK, indices.i_in);});

                            break;
                        case 't':

                            result = interpolate_lin_on_aux2D<Q>(w_temp, v_temp,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int j) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i,j, indices.iw_r,indices.iK, indices.i_in);});
                            break;
                        default:;
                    }
                }
                else if constexpr(inter == linear) {
                    switch (indices.channel_bubble) {
                        case 'a':
                            result = interpolate_lin2D<Q>(v_temp, vp_temp,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int j, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, indices.iw_r,j, k,indices.iK, indices.i_in);});
                            break;
                        case 'p':
                            result = interpolate_lin2D<Q>(w_temp, vp_temp,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i, indices.iw_r, k,indices.iK, indices.i_in);});
                            break;
                        case 't':

                            result = interpolate_lin2D<Q>(w_temp, v_temp,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                          vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int j) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i,j, indices.iw_r,indices.iK, indices.i_in);});
                            break;
                        default:;
                    }
                }
                else if constexpr(inter == sloppycubic) {
                    switch (indices.channel_bubble) {
                        case 'a':
                            result = interpolate_sloppycubic2D<Q>(v_temp, vp_temp,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int j, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, indices.iw_r,j, k,indices.iK, indices.i_in);});
                            break;
                        case 'p':
                            result = interpolate_sloppycubic2D<Q>(w_temp, vp_temp,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int k) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i, indices.iw_r, k,indices.iK, indices.i_in);});

                            break;
                        case 't':
                            result = interpolate_sloppycubic2D<Q>(w_temp, v_temp,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                  vertexDataContainer<k3, Q>::get_VertexFreqGrid().b,
                                                                 [&](int i, int j) -> Q {return vertexDataContainer<k3, Q>::val(indices.spin, i,j, indices.iw_r,indices.iK, indices.i_in);});
                            break;
                        default:;
                    }
                }
                else assert(false);
            }
#endif // DENSEGRID
            assert(isfinite(result));
            return result;


        } else {
            return 0.;  // asymptotic value
        }

    }

    auto operator+= (const vertexBuffer<k3,Q,inter>& rhs) -> vertexBuffer<k3,Q,inter> {vertexDataContainer<k3,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k3,Q,inter>& rhs) -> vertexBuffer<k3,Q,inter> {vertexDataContainer<k3,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k3,Q,inter>& operator+ (vertexBuffer<k3,Q,inter>& lhs, const vertexBuffer<k3,Q,inter>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k3,Q,inter>& operator- (vertexBuffer<k3,Q,inter>& lhs, const vertexBuffer<k3,Q,inter>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};



/// Template specialization for K1 (cubic interpolation)
template<typename Q>
class vertexBuffer<k1,Q, cubic>: public SplineK1<vertexDataContainer<k1,Q>, Q> {
    using base_class = SplineK1<vertexDataContainer<k1,Q>, Q>;
public:
    using index_type = typename base_class::index_type;
    vertexBuffer<k1,Q,cubic>() = default;
    explicit vertexBuffer<k1, Q, cubic>(double Lambda, index_type dims) : SplineK1<vertexDataContainer<k1,Q>, Q>(Lambda, dims) {};
    auto interpolate(const VertexInput &indices) const -> Q {
        // Check if the frequency runs out of the box; if yes: return asymptotic value
        //if (std::abs(indices.w) < vertex.frequencies.b.w_upper + inter_tol)
        //{
        Q result =  SplineK1<vertexDataContainer<k1,Q>, Q>::interpolK1 (indices.iK, indices.spin, indices.w, indices.i_in);
        return result;
        //} else {
        //    return 0.;  // asymptotic value
        //}
    };

    auto operator+= (const vertexBuffer<k1,Q,cubic>& rhs) -> vertexBuffer<k1,Q,cubic> {SplineK1<vertexDataContainer<k1,Q>,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k1,Q,cubic>& rhs) -> vertexBuffer<k1,Q,cubic> {SplineK1<vertexDataContainer<k1,Q>,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k1,Q,cubic>& operator+ (vertexBuffer<k1,Q,cubic>& lhs, const vertexBuffer<k1,Q,cubic>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k1,Q,cubic>& operator- (vertexBuffer<k1,Q,cubic>& lhs, const vertexBuffer<k1,Q,cubic>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/// Template specialization for K2 (cubic interpolation)
template<typename Q>
class vertexBuffer<k2,Q, cubic>: public SplineK2<vertexDataContainer<k2,Q>, Q> {
    using base_class = SplineK2<vertexDataContainer<k2,Q>, Q>;
public:
    using index_type = typename base_class::index_type;
    explicit vertexBuffer<k2, Q, cubic>(double Lambda, index_type dims) : SplineK2<vertexDataContainer<k2,Q>, Q>(Lambda, dims) {};
    auto interpolate(const VertexInput &indices) const -> Q {

        double w_temp = indices.w;
        double v_temp = indices.v1;
        K2_convert2internalFreqs(w_temp, v_temp); // convert natural frequency parametrization in channel r to internal parametrization

        // Check if the frequency runs out of the box; if yes: return asymptotic value
        //if (std::abs(indices.w) < vertex.frequencies.b.w_upper + inter_tol)
        //{

        Q result =  SplineK2<vertexDataContainer<k2,Q>, Q>::interpolK2 (indices.iK, indices.spin, w_temp, v_temp, indices.i_in);
        return result;
        //} else {
        //    return 0.;  // asymptotic value
        //}
    };

    auto operator+= (const vertexBuffer<k2,Q,cubic>& rhs) -> vertexBuffer<k2,Q,cubic> {SplineK1<vertexDataContainer<k2,Q>,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k2,Q,cubic>& rhs) -> vertexBuffer<k2,Q,cubic> {SplineK1<vertexDataContainer<k2,Q>,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k2,Q,cubic>& operator+ (vertexBuffer<k2,Q,cubic>& lhs, const vertexBuffer<k2,Q,cubic>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k2,Q,cubic>& operator- (vertexBuffer<k2,Q,cubic>& lhs, const vertexBuffer<k2,Q,cubic>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

/// Template specialization for K3 (cubic interpolation)
template<typename Q>
class vertexBuffer<k3,Q, cubic>: public SplineK3<vertexDataContainer<k3,Q>, Q> {
    using base_class = SplineK3<vertexDataContainer<k3,Q>, Q>;
public:
    using index_type = typename base_class::index_type;
    explicit vertexBuffer<k3, Q, cubic>(double Lambda, index_type dims) : SplineK3<vertexDataContainer<k3,Q>, Q>(Lambda, dims) {};
    auto interpolate(const VertexInput &indices) const -> Q {

        double w_temp = indices.w;
        double v_temp = indices.v1;
        double vp_temp= indices.v2;
#ifdef BOSONIC_PARAM_FOR_K3
        if (indices.channel == 'a') {switch2bosonicFreqs<'a'>(w_temp, v_temp, vp_temp);}
        else if (indices.channel == 'p') {switch2bosonicFreqs<'p'>(w_temp, v_temp, vp_temp);}
        else if (indices.channel == 't') {switch2bosonicFreqs<'t'>(w_temp, v_temp, vp_temp);}
#endif
        // Check if the frequency runs out of the box; if yes: return asymptotic value
        //if (std::abs(indices.w) < vertex.frequencies.b.w_upper + inter_tol)
        //{
        Q result =  SplineK3<vertexDataContainer<k3,Q>, Q>::interpolK3 (indices.iK, indices.spin, w_temp, v_temp, vp_temp, indices.i_in);
        return result;
        //} else {
        //    return 0.;  // asymptotic value
        //}
    };

    auto operator+= (const vertexBuffer<k3,Q,cubic>& rhs) -> vertexBuffer<k3,Q,cubic> {SplineK1<vertexDataContainer<k3,Q>,Q>::data += rhs.data; return *this;}
    auto operator-= (const vertexBuffer<k3,Q,cubic>& rhs) -> vertexBuffer<k3,Q,cubic> {SplineK1<vertexDataContainer<k3,Q>,Q>::data -= rhs.data; return *this;}
    friend vertexBuffer<k3,Q,cubic>& operator+ (vertexBuffer<k3,Q,cubic>& lhs, const vertexBuffer<k3,Q,cubic>& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend vertexBuffer<k3,Q,cubic>& operator- (vertexBuffer<k3,Q,cubic>& lhs, const vertexBuffer<k3,Q,cubic>& rhs) {
        lhs -= rhs;
        return lhs;
    }
};


*/



#endif //FPP_MFRG_VERTEX_BUFFER_H
