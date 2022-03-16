/**
 * Reducible vertex in channel r, split into diagrammatic classes K1, K2, K2b, K3.
 * Member functions allow for access of values at arbitrary frequencies and in each channel-specific
 * frequency representation.
 */

#ifndef KELDYSH_MFRG_R_VERTEX_HPP
#define KELDYSH_MFRG_R_VERTEX_HPP

#include "../../data_structures.hpp"          // real/complex vector classes
#include "../../parameters/master_parameters.hpp"               // system parameters (lengths of vectors etc.)
#include "../../symmetries/Keldysh_symmetries.hpp"       // transformations on Keldysh indices
#include "../../symmetries/internal_symmetries.hpp"      // symmetry transformations for internal indices (momentum etc.), currently trivial
#include "../../symmetries/symmetry_transformations.hpp" // symmetry transformations of frequencies
#include "../../symmetries/symmetry_table.hpp"           // table containing information when to apply which symmetry transformations
#include "../../grids/momentum_grid.hpp"            // functionality for the internal structure of the Hubbard model
#include "../../utilities/math_utils.hpp"
#include "../../utilities/minimizer.hpp"
#include "../n_point/data_buffer.hpp"

/// Possible (unit-)tests:
/// [IMPLEMENTED in test_symmetries.c++] check read-out from symmetry-reduced sector and correctness of symmetry tables
/// [MISSING] check conversion of frequency conventions

template <typename Q> class fullvert; // forward declaration of fullvert
//template <K_class k, typename Q, interpolMethod inter> class vertexBuffer; // forward declaration of vertexDataContainer
//template <typename Q, interpolMethod interp> class vertexInterpolator; // forward declaration of vertexInterpolator
template<typename Q, std::size_t depth, typename H5object> void write_to_hdf(H5object& group, const H5std_string& dataset_name, const multidimensional::multiarray<Q, depth>& data, const bool data_set_exists);
template<typename Q, typename H5object> void write_to_hdf(H5object& group, const H5std_string& dataset_name, const std::vector<Q>& data, const bool data_set_exists);

template <typename Q>
class rvert{
public:
    using freqGrid_type_K1 = bufferFrequencyGrid<k1>;
    using freqGrid_type_K2 = bufferFrequencyGrid<k2>;
    using freqGrid_type_K3 = bufferFrequencyGrid<k3>;
    using buffer_type_K1 = dataBuffer<Q, k1, K1_config.rank, K1_config.num_freqs, K1_config.position_first_freq_index, freqGrid_type_K1, INTERPOLATION>;
    using buffer_type_K2 = dataBuffer<Q, k2, K2_config.rank, K2_config.num_freqs, K2_config.position_first_freq_index, freqGrid_type_K2, INTERPOLATION>; // vertexBuffer<k2,Q,INTERPOLATION>; //
    using buffer_type_K2b= dataBuffer<Q, k2b,K2_config.rank, K2_config.num_freqs, K2_config.position_first_freq_index, freqGrid_type_K2, INTERPOLATION>; // vertexBuffer<k2b,Q,INTERPOLATION>;//
    using buffer_type_K3 = dataBuffer<Q, k3, K3_config.rank, K3_config.num_freqs, K3_config.position_first_freq_index, freqGrid_type_K3, INTERPOLATION>; // vertexBuffer<k3,Q,INTERPOLATION>; //



    char channel;                       // reducibility channel
private:
    Components components = Components(channel);              // lists providing information on how all Keldysh components are related to the
                                                              // independent ones
    Transformations transformations = Transformations(channel);    // lists providing information on which transformations to apply on Keldysh
                                                                   // components to relate them to the independent ones
    FrequencyTransformations freq_transformations = FrequencyTransformations(channel);  // lists providing information on which transformations to apply on
                                                                                        // frequencies to relate them to the independent ones
    FrequencyComponents freq_components = FrequencyComponents(channel);  // lists providing information on which transformations to apply on
                                                                         // frequencies to relate them to the independent ones
public:
    /// When you add a vertex buffer, also adapt the following:
    /// apply_unary_op_to_all_vertexBuffers()
    /// apply_binary_op_to_all_vertexBuffers()
    buffer_type_K1 K1;
    mutable buffer_type_K1 K1_symmetry_expanded;
    /// cross-projected contributions, needed for the Hubbard model;
    /// have to be mutable to allow us to compute them at a stage where the vertex should be const.
    mutable buffer_type_K1 K1_a_proj = K1;
    mutable buffer_type_K1 K1_p_proj = K1;
    mutable buffer_type_K1 K1_t_proj = K1;

    buffer_type_K2 K2;
    mutable buffer_type_K2 K2_symmetry_expanded;
    mutable buffer_type_K2 K2_a_proj = K2;
    mutable buffer_type_K2 K2_p_proj = K2;
    mutable buffer_type_K2 K2_t_proj = K2;

#ifdef DEBUG_SYMMETRIES
    buffer_type_K2b K2b;
#endif
    mutable buffer_type_K2b K2b_symmetry_expanded;

    buffer_type_K3 K3;
    mutable buffer_type_K3 K3_symmetry_expanded;
    mutable buffer_type_K3 K3_a_proj = K3;
    mutable buffer_type_K3 K3_p_proj = K3;
    mutable buffer_type_K3 K3_t_proj = K3;

    /**
     * Applies unary operator f to this rvert
     * @tparam Func
     * @param f             f can be given via a lambda expression (for an example see arithmetic operations)
     * @param other_rvert
     * @return              returns this
     */
    template <typename Func>
    auto apply_unary_op_to_all_vertexBuffers(Func&& f) {
        if (MAX_DIAG_CLASS > 0) f(K1);
        if (MAX_DIAG_CLASS > 1) f(K2);
#ifdef DEBUG_SYMMETRIES
        if (MAX_DIAG_CLASS > 1) f(K2b);
#endif
        if (MAX_DIAG_CLASS > 2) f(K3);

        return *this;
    }
    /**
     * Applies unary operator f to this rvert (const member function)
     * @tparam Func
     * @param f             f can be given via a lambda expression (for an example see arithmetic operations)
     * @param other_rvert
     * @return              returns this
     */
    template <typename Func>
    auto apply_unary_op_to_all_vertexBuffers(Func&& f) const {
        if (MAX_DIAG_CLASS > 0) f(K1);
        if (MAX_DIAG_CLASS > 1) f(K2);
#ifdef DEBUG_SYMMETRIES
        if (MAX_DIAG_CLASS > 1) f(K2b);
#endif
        if (MAX_DIAG_CLASS > 2) f(K3);

        return *this;
    }

    /**
     * Applies binary operator f to this rvert and to another rvertex
     * @tparam Func
     * @param f             f can be given via a lambda expression (for an example see arithmetic operations)
     * @param other_rvert
     * @return              returns this
     */
    template <typename Func>
    auto apply_binary_op_to_all_vertexBuffers(Func&& f, const rvert<Q>& other_rvert) {

        if (MAX_DIAG_CLASS > 0) f(K1, other_rvert.K1);
        if (MAX_DIAG_CLASS > 1) f(K2, other_rvert.K2);
#ifdef DEBUG_SYMMETRIES
        if (MAX_DIAG_CLASS > 1) f(K2b, other_rvert.K2b);
#endif
        if (MAX_DIAG_CLASS > 2) f(K3, other_rvert.K3);

        return *this;
    }


    /**
     * Constructor
     * @param channel_in
     * @param Lambda
     */
    rvert(const char channel_in, const double Lambda, const bool is_reserve)
    : channel(channel_in)
      {
        if (is_reserve) {
            if (MAX_DIAG_CLASS >= 1) K1 = buffer_type_K1(Lambda, K1_config.dims);
            if (MAX_DIAG_CLASS >= 2) K2 = buffer_type_K2(Lambda, K2_config.dims);
            if (MAX_DIAG_CLASS >= 3) K3 = buffer_type_K3(Lambda, K3_config.dims);
            if constexpr(HUBBARD_MODEL) {
                if (MAX_DIAG_CLASS >= 1) {
                    K1_a_proj = buffer_type_K1(Lambda, K1_config.dims);
                    K1_p_proj = buffer_type_K1(Lambda, K1_config.dims);
                    K1_t_proj = buffer_type_K1(Lambda, K1_config.dims);
                }
                if (MAX_DIAG_CLASS >= 2) {
                    K2_a_proj = buffer_type_K2(Lambda, K2_config.dims);
                    K2_p_proj = buffer_type_K2(Lambda, K2_config.dims);
                    K2_t_proj = buffer_type_K2(Lambda, K2_config.dims);
                }
                if (MAX_DIAG_CLASS >= 3) {
                    K3_a_proj = buffer_type_K3(Lambda, K3_config.dims);
                    K3_p_proj = buffer_type_K3(Lambda, K3_config.dims);
                    K3_t_proj = buffer_type_K3(Lambda, K3_config.dims);
                }
            }

#ifdef DEBUG_SYMMETRIES
            K2b = buffer_type_K2b(Lambda, K2_config.dims);
#endif
        }
      };
    rvert() = delete;

    mutable bool calculated_crossprojections = false;
    void cross_project();



    /**
     * Return the value of the reducible vertex in channel r = sum of all K_classes K1, K2, K2b and K3.
     * This version of value() is used for the symmetric_full vertex
     * @param input          : Combination of input arguments.
     * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
     *                         symmetry transformations that map between channels a <--> t.
     */
    template<char ch_bubble> auto value(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q;
    /** Overload for accessing non-symmetric_full vertices, with
     * @param vertex_half2 : vertex related to the calling vertex by symmetry, needed for transformations with
     *                       asymmetry_transform=true
     */
    template<char ch_bubble> auto value(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;

    /// Returns the symmetry reduced vertex component and the information where to read it out (in IndicesSymmetryTransformations)
    /// This version is used for a symmetric_full vertex
    template <K_class k>
    const rvert<Q>& symmetry_reduce(const VertexInput &input, IndicesSymmetryTransformations& indices,
#ifdef DEBUG_SYMMETRIES
                                    const rvert<Q>& rvert_this,
#endif
                                    const rvert<Q>& rvert_crossing) const;

    /// Returns the symmetry reduced vertex component and the information where to read it out (in IndicesSymmetryTransformations)
    /// This version is used for an Asymmetric vertex
    template <K_class k>
    const rvert<Q>& symmetry_reduce(const VertexInput &input, IndicesSymmetryTransformations& indices, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const;

    /**
     * Reads out a symmetry reduced vertex component. Hence symmetry_reduce must be called before!
     * @tparam k        K_class
     * @param indices   contains information for reading out the symmetry reduced vertex component
     * @param readMe    vertex to be read
     * @return
     */
    template<K_class k>
    Q read_symmetryreduced_rvert(const IndicesSymmetryTransformations& indices, const rvert<Q>& readMe) const;

    template<K_class k>
    auto read_value(const IndicesSymmetryTransformations& indices, const rvert<Q>& readMe) const -> Q;

    /**
     * Return the value of the vertex Ki in channel r.
     * @param input          : Combination of input arguments.
     * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
     *                         symmetry transformations that map between channels a <--> t.
     */
    template <K_class k>
    auto valsmooth(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q;

    /** Overload for accessing non-symmetric_full vertices, with
     * @param vertex_half2 : vertex related to the calling vertex by symmetry, needed for transformations with
     *                       asymmetry_transform=true */
    template <K_class k>
    auto valsmooth(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;

    /** Parts of the r vertex that connect to the same/different bare vertices on the left/right of an r bubble */
    auto left_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q;
    auto left_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;
    auto right_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q;
    auto right_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;
    auto left_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q;
    auto left_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;
    auto right_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q;
    auto right_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q;

    /**
     * Transform the frequencies from the frequency convention of input.channel to the frequency convention of
     * this->channel. Necessary when accessing the r vertex from a different channel r'.
     */
    template<char ch_bubble> void transfToR(VertexInput& input) const;

    /**
     * Interpolate the vertex to updated grid when rescaling the grid to new flow parameter Lambda.
     */
    void update_grid(double Lambda);

    /**
     * Interpolate the vertex to updated grid.
     * @tparam k
     * @param frequencyGrid_in  new frequency grid
     * @param rvert4data        vertex to be interpolated
     *                          can be different from *this, so we can backup a vertex and interpolate the backup
     */
    template<K_class k, typename FGrid>
    void update_grid(const FGrid& frequencyGrid_in, rvert<Q>& rvert4data);

    /**
     * Optimizes the frequency grids and updates the grids accordingly
     */
    void findBestFreqGrid(bool verbose=true);

    /** K1-functionality */


    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK1(const rvert<Q>& vertex_symmrelated);

    void K1_crossproject(char channel_out);
    Q K1_BZ_average(const int iK, int ispin, const int iw);

    /** K2 functionality */

    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK2(const rvert<Q>& vertex_symmrelated);

    // TODO: Implement! Needed for the Hubbard model.
    void K2_crossproject(char channel_out);

    /** K3 functionality */

    /**
     * Apply the frequency symmetry relations (for the independent components) to update the vertex after bubble integration.
     */
    void enforce_freqsymmetriesK3(const rvert<Q>& vertex_symmrelated);

    // TODO: Implement! Needed for the Hubbard model.
    void K3_crossproject(char channel_out);

    void initInterpolator() const {
        apply_unary_op_to_all_vertexBuffers([&](auto &&buffer) -> void { buffer.initInterpolator(); });
    }
    void set_initializedInterpol(const bool is_init) const {
        apply_unary_op_to_all_vertexBuffers([&](auto &&buffer) -> void { buffer.initialized = is_init; });
    }

    /**
     * This function is defined if the flag DEBUG_SYMMETRIES is defined.
     * It iterates over all vertex entries and checks whether symmetry relations are fulfilled.
     * Deviations from the symmetry relation are stored in a file.
     * @param identifier        string to distinguish the filename of the stored
     * @param spin              spin component (currently 0 or 1)
     * @param rvert_this        rvert of the related vertex with spin 0 (contains symmetry reduced sector of vertex)
     * @param rvert_crossing    rvert that is related to rvert_this via crossing symmetry
     */
    void check_symmetries(std::string identifier, const rvert<Q>& rvert_this, const rvert<Q> &rvert_crossing) const;

    template<char channel_bubble, bool is_left_vertex> void symmetry_expand(const rvert<Q> &rvert_this, const rvert<Q> &rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel, int spin) const;
    void save_expanded(const std::string &filename) const;


    /// Arithmetric operators act on vertexBuffers:
    auto operator+= (const rvert<Q>& rhs) -> rvert<Q> {
        return apply_binary_op_to_all_vertexBuffers([&](auto&& left, auto&& right) -> void {left.data += right.data;}, rhs);
    }
    friend rvert<Q> operator+ (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs += rhs;
        return lhs;
    }
    auto operator*= (const rvert<Q>& rhs) -> rvert<Q> {
        return apply_binary_op_to_all_vertexBuffers([&](auto&& left, auto&& right) -> void {left.data *= right.data;}, rhs);
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator-= (const rvert<Q>& rhs) -> rvert<Q> {
        return apply_binary_op_to_all_vertexBuffers([&](auto&& left, auto&& right) -> void {left.data -= right.data;}, rhs);
    }
    friend rvert<Q> operator- (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs -= rhs;
        return lhs;
    }
    // Elementwise divion:
    auto operator/= (const rvert<Q>& rhs) -> rvert<Q> {
        return apply_binary_op_to_all_vertexBuffers([&](auto&& left, auto&& right) -> void {left.data /= right.data;}, rhs);
    }
    friend rvert<Q> operator/ (rvert<Q> lhs, const rvert<Q>& rhs) {
        lhs /= rhs;
        return lhs;
    }

    auto operator*= (double alpha) -> rvert<Q> {
        return apply_unary_op_to_all_vertexBuffers([&](auto &&buffer) -> void { buffer.data *= alpha; });
    }
    friend rvert<Q> operator* (rvert<Q> lhs, const double& rhs) {
        lhs *= rhs;
        return lhs;
    }
    auto operator+= (double alpha) -> rvert<Q> {
        return apply_unary_op_to_all_vertexBuffers([&](auto &&buffer) -> void { buffer.data += alpha; });
    }
    friend rvert<Q> operator+ (rvert<Q> lhs, const double& rhs) {
        lhs += rhs;
        return lhs;
    }

    template<K_class k, typename result_type> auto valsmooth_symmetry_expanded(const VertexInput &input, const rvert<Q> &rvert_crossing) const -> result_type;
    template<typename result_type>auto left_same_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const  -> result_type;
    template<typename result_type>auto right_same_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> result_type;
    template<typename result_type>auto left_diff_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const  -> result_type;
    template<typename result_type>auto right_diff_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> result_type;
    template<char ch_bubble, typename result_type> auto value_symmetry_expanded(VertexInput input, const rvert<Q> &rvert_crossing) const -> result_type;
};

/****************************************** MEMBER FUNCTIONS OF THE R-VERTEX ******************************************/

template <typename Q>
template <K_class k>
const rvert<Q>& rvert<Q>::symmetry_reduce(const VertexInput &input, IndicesSymmetryTransformations& indices,
#ifdef DEBUG_SYMMETRIES
                                          const rvert<Q>& rvert_this,
#endif
                                          const rvert<Q>& rvert_crossing) const {

    Ti(indices, transformations.K(k, input.spin, input.iK));  // apply necessary symmetry transformations
#ifndef DEBUG_SYMMETRIES
    indices.iK = components.K(k, input.spin, input.iK);  // check which symmetry-transformed component should be read
#else
    int itK = components.K(k, input.spin, input.iK);  // check which symmetry-transformed component should be read
    //const std::vector<int> all_Keldysh {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    //int res_iK;
    //locate(all_Keldysh, 16, itK, res_iK, -1, 16);
    if constexpr (k == k1) {
        indices.iK = (indices.channel_rvert == 'a' ? non_zero_Keldysh_K1a[itK]: indices.channel_rvert == 'p' ? non_zero_Keldysh_K1p[itK] : non_zero_Keldysh_K1t[itK]);
    }
    else if constexpr (k == k2 or k == k2b) {
        indices.iK = (indices.channel_rvert == 'a' ? non_zero_Keldysh_K2a[itK]: indices.channel_rvert == 'p' ? non_zero_Keldysh_K2p[itK] : non_zero_Keldysh_K2t[itK]);
    }
    else {
        indices.iK = non_zero_Keldysh_K3[itK];
    }


#endif
    if (indices.channel_rvert != channel)
        // if the symmetry transformation switches between channels (a <--> t), return the
        // r vertex in the channel related by crossing symmetry
        return rvert_crossing;
    else
        // otherwise return the calling r vertex
#ifdef DEBUG_SYMMETRIES
        return rvert_this;
#else
        return (*this);
#endif
}

template <typename Q>
template <K_class k>
const rvert<Q>& rvert<Q>::symmetry_reduce(const VertexInput &input, IndicesSymmetryTransformations& indices, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const {

    Ti(indices, transformations.K(k, input.spin, input.iK));  // apply necessary symmetry transformations
    indices.iK = components.K(k, input.spin, input.iK);  // check which symmetry-transformed component should be read

    // first check if the applied transformations switch between half 1 and half 2 of the vertex
    if (indices.asymmetry_transform) {
        // if yes, return the interpolated value of half 2 in the appropriate channel
        if (channel == indices.channel_rvert) {
            // if the applied transformation(s) do not switch between channels a,t, return a vertex of half 2
            return vertex_half2_samechannel;
        }
        else {
            // if they do switch between channels a,t, return t vertex of half 2
            return vertex_half2_switchedchannel;
        }

    }
    else {
        // if no, return the interpolated value of half 1 in the appropriate channel
        if (indices.channel_rvert != channel)
            // if the symmetry transformation switches between channels (a <--> t), return the
            // r vertex in the channel related by crossing symmetry
            return rvert_crossing;
        else
            // otherwise return the calling r vertex
            return (*this);
    }
}

template <typename Q>
template <K_class k>
auto rvert<Q>::read_symmetryreduced_rvert(const IndicesSymmetryTransformations& indices, const rvert<Q>& readMe) const -> Q {
    if (indices.iK < 0) return 0.;  // components with label -1 in the symmetry table are zero --> return 0. directly

    assert(indices.channel_rvert == readMe.channel);
#ifndef DEBUG_SYMMETRIES
    //if constexpr (k == k2) assert(not indices.asymmetry_transform);
    //if constexpr (k == k2b) assert(indices.asymmetry_transform);
#endif

    Q value = read_value<k>(indices, readMe);

    if ((KELDYSH || !PARTICLE_HOLE_SYMMETRY) && indices.conjugate) value = myconj(value);  // apply complex conjugation if T_C has been used

    assert(isfinite(value));
    return indices.prefactor * value;
}

template <typename Q>
template <K_class k>
auto rvert<Q>::read_value(const IndicesSymmetryTransformations& indices, const rvert<Q>& readMe) const -> Q {
    if (HUBBARD_MODEL && indices.channel_parametrization != channel) {
        assert(calculated_crossprojections);
        switch (indices.channel_parametrization) {
            case 'a':
                if      constexpr(k==k1) return readMe.K1_a_proj.interpolate(indices);
                else if constexpr(k==k3) return readMe.K3_a_proj.interpolate(indices);
                else                     return readMe.K2_a_proj.interpolate(indices); // for both k2 and k2b we need to interpolate K2
            case 'p':
                if      constexpr(k==k1) return readMe.K1_p_proj.interpolate(indices);
                else if constexpr(k==k3) return readMe.K3_p_proj.interpolate(indices);
                else                     return readMe.K2_p_proj.interpolate(indices); // for both k2 and k2b we need to interpolate K2
            case 't':
                if      constexpr(k==k1) return readMe.K1_t_proj.interpolate(indices);
                else if constexpr(k==k3) return readMe.K3_t_proj.interpolate(indices);
                else                     return readMe.K2_t_proj.interpolate(indices); // for both k2 and k2b we need to interpolate K2
            default:
                break;
        }
    }
    else {
        if      constexpr(k==k1) return readMe.K1.interpolate(indices);
        else if constexpr(k==k3) return readMe.K3.interpolate(indices);
#ifndef DEBUG_SYMMETRIES
        else { // for both k2 and k2b we need to interpolate K2
            return readMe.K2.interpolate(indices);
        }
#else
        else if(k==k2){ // for both k2 and k2b we need to interpolate K2
            return readMe.K2.interpolate(indices);
        }
        else {
            return readMe.K2b.interpolate(indices);
        }
#endif
    }
}

/**
 * Return the value of the vertex Ki in channel r.
 * @param input          : Combination of input arguments.
 * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
 *                         symmetry transformations that map between channels a <--> t.
 */
template <typename Q>
template<K_class k>auto rvert<Q>::valsmooth(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q {
    IndicesSymmetryTransformations indices (input, channel);
#ifdef DEBUG_SYMMETRIES
    return read_value<k>(indices, *this);
#else
    const rvert<Q>& readMe = symmetry_reduce<k>(input, indices, rvert_crossing);

    return read_symmetryreduced_rvert<k>(indices, readMe);
#endif
}

/**
 * Return the value of the vertex Ki in channel r.
 * @param input          : Combination of input arguments.
 * @param rvert_crossing : Reducible vertex in the related channel (t,p,a) for r=(a,p,t), needed to apply
 *                         symmetry transformations that map between channels a <--> t.
 * @param vertex_half2   : vertex related to the calling vertex by symmetry, needed for transformations with
     *                       asymmetry_transform=true
 */
template <typename Q>
template<K_class k>auto rvert<Q>::valsmooth(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {
    IndicesSymmetryTransformations indices (input, channel);
#ifdef DEBUG_SYMMETRIES
    return read_symmetryreduced_rvert<k>(indices, *this);
#else
    const rvert<Q>& readMe = symmetry_reduce<k>(input, indices, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

    return read_symmetryreduced_rvert<k>(indices, readMe);
#endif
}

template <typename Q>
template<K_class k, typename result_type> auto rvert<Q>::valsmooth_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> result_type {
    //IndicesSymmetryTransformations indices (input, channel);

    if constexpr(k == k1)       return  K1_symmetry_expanded.template interpolate<result_type>(input);
    else if constexpr(k == k2)  return  K2_symmetry_expanded.template interpolate<result_type>(input);
    else if constexpr(k == k2b) return K2b_symmetry_expanded.template interpolate<result_type>(input);
    else                        return  K3_symmetry_expanded.template interpolate<result_type>(input);
}


/**
 * Iterates over all vertex components and compares with the value obtained from application of the symmetry relations
 * @tparam Q
 */
template<typename Q> void rvert<Q>::check_symmetries(const std::string identifier, const rvert<Q>& rvert_this, const rvert<Q>& rvert_crossing) const {
#ifdef  DEBUG_SYMMETRIES
    print("maximal deviation in symmetry in " + identifier +"_channel" + channel + " (normalized by maximal absolute value of K_i)", "\n");

    // K1:
    multidimensional::multiarray<Q,4> deviations_K1(K1.get_dims());
    for (int iflat = 0; iflat < getFlatSize(K1.get_dims()); iflat++) {
        my_defs::K1::index_type idx;
        getMultIndex<rank_K1>(idx, iflat, K1.get_dims());
        int iK              = (int) idx[my_defs::K1::keldysh];
        my_index_t ispin    = idx[my_defs::K1::spin];
        my_index_t iw       = idx[my_defs::K1::omega];
        my_index_t i_in     = idx[my_defs::K1::internal];

        double w;
        K1.frequencies.get_freqs_w(w, iw);
        VertexInput input(iK, ispin, w, 0., 0., i_in, channel);
        IndicesSymmetryTransformations indices(input, channel);
        Q value_direct = read_symmetryreduced_rvert<k1>(indices, *this);

        const rvert<Q>& readMe = symmetry_reduce<k1>(input, indices, rvert_this, rvert_crossing);
        Q value_symmet = read_symmetryreduced_rvert<k1>(indices, readMe);

        Q deviation = value_direct - value_symmet;
        //if (components.K(k1, input.spin, input.iK) != -1) {// zero component is not being computed anyway /// TODO: Why don't I get a numerically exact zero?
            deviations_K1.at(idx) = deviation;
        //}

        // test frequency symmetries for symmetry-reduced Keldysh components:
        if (transformations.K(k1, input.spin, input.iK) == 0 and components.K(k1, input.spin, input.iK) != -1 and (channel == 'a' ? isInList(iK, non_zero_Keldysh_K1a) : ( channel == 'p' ? isInList(iK, non_zero_Keldysh_K1p) : isInList(iK, non_zero_Keldysh_K1t) ) )) {
            // Check frequency symmetries
            IndicesSymmetryTransformations indices_f(iK, ispin, w, 0., 0., i_in, channel, k1, 0, channel);
            int sign_w = sign_index(indices_f.w);
            int itK;
            // find position of iK and store it in itK:
            locate((channel == 'a' ? non_zero_Keldysh_K1a : (channel == 'p' ? non_zero_Keldysh_K1p : non_zero_Keldysh_K1t)), non_zero_Keldysh_K1t.size(), iK, itK, 0, non_zero_Keldysh_K1t.size());
            int trafo_index = freq_transformations.K1[itK][sign_w];
            if (trafo_index != 0){
                Ti(indices_f, trafo_index);
                indices_f.iK = iK;

                Q result_freqsymm = read_symmetryreduced_rvert<k1>(indices_f, *this);
                deviation = value_direct - result_freqsymm;
                deviations_K1.at(idx) = deviation;
            }
        }
    }

    if ( K1.get_vec().max_norm() > 1e-30) print("K1: \t", deviations_K1.max_norm() / K1.get_vec().max_norm(), "\n");

    //rvec deviations_K2(getFlatSize(K2.get_dims()));
    //rvec deviations_K2b(getFlatSize(K2b.get_dims()));
    multidimensional::multiarray<Q,5> deviations_K2 ( K2.get_dims());
    multidimensional::multiarray<Q,5> deviations_K2b(K2b.get_dims());
    if (MAX_DIAG_CLASS > 1) {
        // K2:
        for (int iflat = 0; iflat < getFlatSize(K2.get_dims()); iflat++) {
            my_defs::K2::index_type idx;
            getMultIndex<rank_K2>(idx, iflat, K2.get_dims());
            int iK              = (int) idx[my_defs::K2::keldysh];
            my_index_t ispin    = idx[my_defs::K2::spin];
            my_index_t iw       = idx[my_defs::K2::omega];
            my_index_t iv       = idx[my_defs::K2::nu];
            my_index_t i_in     = idx[my_defs::K2::internal];

            double w, v;
            K2.frequencies.get_freqs_w(w, v, iw, iv);
            VertexInput input(iK, ispin, w, v, 0., i_in, channel);
            IndicesSymmetryTransformations indices(input, channel);
            Q value_direct = read_symmetryreduced_rvert<k2>(indices, *this);

            const rvert<Q>& readMe = symmetry_reduce<k2>(input, indices, rvert_this, rvert_crossing);
            Q value_symmet = read_symmetryreduced_rvert<k2>(indices, readMe);

            Q deviation = value_direct - value_symmet;
            if (components.K(k2, input.spin, input.iK) != -1) {// zero component is not being computed anyway
                deviations_K2.at(idx) = deviation;
            }

            // test frequency symmetries for symmetry-reduced Keldysh components:
            if (transformations.K(k2, input.spin, input.iK) == 0 and components.K[k2, input.spin, input.iK] != -1 and
                (channel == 'a' ? isInList(iK, non_zero_Keldysh_K2a) : (channel == 'p' ? isInList(iK,
                                                                                                  non_zero_Keldysh_K2p)
                                                                                       : isInList(iK,
                                                                                                  non_zero_Keldysh_K2t)))) {
                IndicesSymmetryTransformations indices(iK, ispin, w, v, 0., i_in, channel, k2, 0, channel);
                int sign_w = sign_index(w);
                int sign_v1 = sign_index(v);
                int itK;
                locate((channel == 'a' ? non_zero_Keldysh_K2a : (channel == 'p' ? non_zero_Keldysh_K2p
                                                                                : non_zero_Keldysh_K2t)),
                       non_zero_Keldysh_K2t.size(), iK, itK, 0, non_zero_Keldysh_K2t.size());
                int trafo_index = freq_transformations.K2[itK][sign_w * 2 + sign_v1];
                Ti(indices, trafo_index);
                //indices.iK = itK;

                Q result_freqsymm = read_symmetryreduced_rvert<k2>(indices, *this);
                deviation = value_direct - result_freqsymm;
                deviations_K2.at(idx) = deviation;
            }
        }

        if (K2.get_vec().max_norm() > 1e-30) print("K2: \t", deviations_K2.max_norm() / K2.get_vec().max_norm(), "\n");


        // K2b:
        for (int iflat = 0; iflat < getFlatSize(K2b.get_dims()); iflat++) {

            my_defs::K2::index_type idx;
            getMultIndex<rank_K2>(idx, iflat, K2b.get_dims());
            int iK              = (int) idx[my_defs::K2b::keldysh];
            my_index_t ispin    = idx[my_defs::K2b::spin];
            my_index_t iw       = idx[my_defs::K2b::omega];
            my_index_t iv       = idx[my_defs::K2b::nup];
            my_index_t i_in     = idx[my_defs::K2b::internal];

            double w, vp;
            K2b.frequencies.get_freqs_w(w, vp, iw, iv);
            VertexInput input(iK, ispin, w, 0., vp, i_in, channel);
            IndicesSymmetryTransformations indices(input, channel);
            Q value_direct = read_symmetryreduced_rvert<k2b>(indices, *this);

            const rvert<Q>& readMe = symmetry_reduce<k2b>(input, indices, rvert_this, rvert_crossing);
            Q value_symmet = read_symmetryreduced_rvert<k2>(indices, readMe);

            Q deviation = value_direct - value_symmet;
            if (components.K[k2b, input.spin, input.iK] != -1) {// zero component is not being computed anyway
                deviations_K2b.at(idx) = deviation;
            }
        }

        if (K2b.get_vec().max_norm() > 1e-30)
            print("K2b: \t", deviations_K2b.max_norm() / K2b.get_vec().max_norm(), "\n");
    }

    //rvec deviations_K3(getFlatSize(K3.get_dims()));
    multidimensional::multiarray<Q,6> deviations_K3 (K3.get_dims());
    if (MAX_DIAG_CLASS > 2) {
        // K3:
        for (int iflat = 0; iflat < getFlatSize(K3.get_dims()); iflat++) {
            my_defs::K3::index_type idx;
            getMultIndex<rank_K3>(idx, iflat, K3.get_dims());
            int iK              = (int) idx[my_defs::K3::keldysh];
            my_index_t ispin    = idx[my_defs::K3::spin];
            my_index_t iw       = idx[my_defs::K3::omega];
            my_index_t iv       = idx[my_defs::K3::nu];
            my_index_t ivp      = idx[my_defs::K3::nup];
            my_index_t i_in     = idx[my_defs::K3::internal];

            double w, v, vp;
            K3.frequencies.get_freqs_w(w, v, vp, iw, iv, ivp);
            VertexInput input(iK, ispin, w, v, vp, i_in, channel);
            IndicesSymmetryTransformations indices(input, channel);
            Q value_direct = read_symmetryreduced_rvert<k3>(indices, *this);

            const rvert<Q>& readMe = symmetry_reduce<k3>(input, indices, rvert_this, rvert_crossing);
            Q value_symmet = read_symmetryreduced_rvert<k3>(indices, readMe);

            Q deviation = value_direct - value_symmet;
            if (components.K[k1, input.spin, input.iK] != -1) { // zero component is not being computed anyway
                deviations_K3.at(idx) = deviation;
            }

            // test frequency symmetries for symmetry-reduced Keldysh components:
            if (transformations.K(k3, input.spin, input.iK) == 0 and components.K[k3, input.spin, input.iK] != -1 and
                isInList(iK, non_zero_Keldysh_K3)) {
                IndicesSymmetryTransformations indices(iK, ispin, w, v, vp, i_in, channel, k2, 0, channel);
                int sign_w = sign_index(w);
                int sign_f = sign_index(indices.v1 + indices.v2);
                int sign_fp = sign_index(indices.v1 - indices.v2);
                int itK;
                locate(non_zero_Keldysh_K3, non_zero_Keldysh_K3.size(), iK, itK, 0, non_zero_Keldysh_K3.size());

                int trafo_index = freq_transformations.K3[itK][sign_w * 4 + sign_f * 2 + sign_fp];
                Ti(indices, trafo_index);
                //indices.iK = itK;

                Q result_freqsymm = read_symmetryreduced_rvert<k3>(indices, *this);
                deviation = value_direct - result_freqsymm;
                deviations_K3.at(idx) = deviation;
            }
        }

        if (K3.get_vec().max_norm() > 1e-30) print("K3: \t", deviations_K3.max_norm() / K3.get_vec().max_norm(), "\n");
    }

    std::string filename = data_dir + "deviations_from_symmetry" + identifier +"_channel" + channel + ".h5";
    H5::H5File file(filename.c_str(), H5F_ACC_TRUNC);
    write_to_hdf(file, "K1", deviations_K1 * (1/K1 .get_vec().max_norm()), false);
#if MAX_DIAG_CLASS > 1
    write_to_hdf(file, "K2", deviations_K2 * (1/K2 .get_vec().max_norm()), false);
    write_to_hdf(file, "K2b",deviations_K2b* (1/K2b.get_vec().max_norm()), false);
#endif
#if MAX_DIAG_CLASS > 2
    write_to_hdf(file, "K3", deviations_K3 * (1/K3 .get_vec().max_norm()), false);
#endif
    file.close();


#endif
}



/**
 * Iterates over all vertex components and fills in the value obtained from the symmetry-reduced sector
 * @tparam Q
 */
template<typename Q> template<char channel_bubble, bool is_left_vertex> void rvert<Q>::symmetry_expand(const rvert<Q>& rvert_this, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel, const int spin) const {
    /// TODO: Currently copies frequency_grid of same rvertex; but might actually need the frequency grid of conjugate channel
    assert(0 <= spin and spin < 2);
    K1_symmetry_expanded = buffer_type_K1(0., K1_expanded_config.dims);
    K2_symmetry_expanded = buffer_type_K2(0., K2_expanded_config.dims);
    K2b_symmetry_expanded = buffer_type_K2b (0., K2_expanded_config.dims);
    K3_symmetry_expanded = buffer_type_K3(0., K3_expanded_config.dims);
    if (spin == 0) {
        K1_symmetry_expanded.set_VertexFreqGrid(rvert_this.K1.get_VertexFreqGrid());
        K2_symmetry_expanded.set_VertexFreqGrid(rvert_this.K2.get_VertexFreqGrid());
        K2b_symmetry_expanded.set_VertexFreqGrid(rvert_this.K2.get_VertexFreqGrid());
        K3_symmetry_expanded.set_VertexFreqGrid(rvert_this.K3.get_VertexFreqGrid());
    }
    else {
        K1_symmetry_expanded.set_VertexFreqGrid(rvert_crossing.K1.get_VertexFreqGrid());
        K2_symmetry_expanded.set_VertexFreqGrid(rvert_crossing.K2.get_VertexFreqGrid());
        K2b_symmetry_expanded.set_VertexFreqGrid(rvert_crossing.K2.get_VertexFreqGrid());
        K3_symmetry_expanded.set_VertexFreqGrid(rvert_crossing.K3.get_VertexFreqGrid());
    }



    // K1:
    //vec<Q> deviations_K1(getFlatSize(K1.get_dims()));
#pragma omp parallel for schedule(dynamic, 50)
    for (my_index_t iflat = 0; iflat < getFlatSize(K1_symmetry_expanded.get_dims()); iflat++) {
        //int iK;
        //my_index_t ispin, iw, i_in;
        //getMultIndex<4,my_index_t,my_index_t,int,my_index_t>(ispin, iw, iK, i_in, iflat, K1_symmetry_expanded.get_dims());

        my_defs::K1::index_type idx;
        getMultIndex<rank_K1>(idx, iflat, K1_symmetry_expanded.get_dims());
        int iK           = (int) idx[my_defs::K1::keldysh];
        my_index_t ispin = spin; // idx[my_defs::K1::spin];
        my_index_t iw    = idx[my_defs::K1::omega];
        my_index_t i_in  = idx[my_defs::K1::internal];
        assert(idx[my_defs::K1::spin] == 0);
        double w;
        K1_symmetry_expanded.frequencies.get_freqs_w(w, iw);
        VertexInput input(rotate_to_matrix<channel_bubble,is_left_vertex>(iK), ispin, w, 0., 0., i_in, channel);
        Q value = rvert_this.template valsmooth<k1>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

        K1_symmetry_expanded.setvert(value, idx);

    }
    K1_symmetry_expanded.initInterpolator();


    if (MAX_DIAG_CLASS > 1) {
        // K2:
#pragma omp parallel for schedule(dynamic, 50)
        for (my_index_t iflat = 0; iflat < getFlatSize(K2_symmetry_expanded.get_dims()); iflat++) {
            //int iK;
            //my_index_t ispin, iw, iv, i_in;
            //getMultIndex<5, my_index_t, my_index_t, my_index_t, int, my_index_t>(ispin, iw, iv, iK, i_in, iflat, K2_symmetry_expanded.get_dims());
            my_defs::K2::index_type idx;
            getMultIndex<rank_K2>(idx, iflat, K2_symmetry_expanded.get_dims());
            int iK              = (int) idx[my_defs::K2::keldysh];
            my_index_t ispin    = spin; // idx[my_defs::K2::spin];
            assert(idx[my_defs::K2::spin] == 0);
            my_index_t iw       = idx[my_defs::K2::omega];
            my_index_t iv       = idx[my_defs::K2::nu];
            my_index_t i_in     = idx[my_defs::K2::internal];


            double w, v;
            K2_symmetry_expanded.frequencies.get_freqs_w(w, v, iw, iv);
            VertexInput input(rotate_to_matrix<channel_bubble,is_left_vertex>(iK), ispin, w, v, 0., i_in, channel);
            Q value = rvert_this.template valsmooth<k2>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

            K2_symmetry_expanded.setvert(value, idx);

        }

        // K2b:
#pragma omp parallel for schedule(dynamic, 50)
        for (my_index_t iflat = 0; iflat < getFlatSize(K2b_symmetry_expanded.get_dims()); iflat++) {
            //int iK;
            //my_index_t ispin, iw, iv, i_in;
            //getMultIndex<5, my_index_t, my_index_t, my_index_t, int, my_index_t>(ispin, iw, iv, iK, i_in, iflat, K2b_symmetry_expanded.get_dims());

            my_defs::K2::index_type idx;
            getMultIndex<rank_K2>(idx, iflat, K2b_symmetry_expanded.get_dims());
            int iK              = (int) idx[my_defs::K2b::keldysh];
            my_index_t ispin    = spin; // idx[my_defs::K2b::spin];
            assert(idx[my_defs::K2b::spin] == 0);
            my_index_t iw       = idx[my_defs::K2b::omega];
            my_index_t iv       = idx[my_defs::K2b::nup];
            my_index_t i_in     = idx[my_defs::K2b::internal];

            double w, vp;
            K2b_symmetry_expanded.frequencies.get_freqs_w(w, vp, iw, iv);
            VertexInput input(rotate_to_matrix<channel_bubble,is_left_vertex>(iK), ispin, w, 0., vp, i_in, channel);
            Q value = rvert_this.template valsmooth<k2b>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

            K2b_symmetry_expanded.setvert(value, idx);

        }

        K2_symmetry_expanded.initInterpolator();
        K2b_symmetry_expanded.initInterpolator();
    }

    if (MAX_DIAG_CLASS > 2) {
        // K3:
#pragma omp parallel for schedule(dynamic, 50)
        for (my_index_t iflat = 0; iflat < getFlatSize(K3_symmetry_expanded.get_dims()); iflat++) {
            //int iK;
            //my_index_t ispin, iw, iv, ivp, i_in;
            //getMultIndex<6, my_index_t, my_index_t, my_index_t, my_index_t, int, my_index_t>(ispin, iw, iv, ivp, iK, i_in, iflat, K3_symmetry_expanded.get_dims());

            my_defs::K3::index_type idx;
            getMultIndex<rank_K3>(idx, iflat, K3_symmetry_expanded.get_dims());
            int iK              = (int) idx[my_defs::K3::keldysh];
            my_index_t ispin    = spin; // idx[my_defs::K3::spin];
            assert(idx[my_defs::K3::spin] == 0);
            my_index_t iw       = idx[my_defs::K3::omega];
            my_index_t iv       = idx[my_defs::K3::nu];
            my_index_t ivp      = idx[my_defs::K3::nup];
            my_index_t i_in     = idx[my_defs::K3::internal];


            double w, v, vp;
            K3_symmetry_expanded.frequencies.get_freqs_w(w, v, vp, iw, iv, ivp);
            VertexInput input(rotate_to_matrix<channel_bubble,is_left_vertex>(iK), ispin, w, v, vp, i_in, channel);
            Q value = rvert_this.template valsmooth<k3>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

            K3_symmetry_expanded.setvert(value, idx);

        }
        K3_symmetry_expanded.initInterpolator();

    }

}


template<typename Q> void rvert<Q>::save_expanded(const std::string& filename) const {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    const std::string ch(1, channel);
    write_to_hdf(file, "K1" + ch, K1_symmetry_expanded .get_vec(), false);
    write_to_hdf(file, "K2" + ch, K2_symmetry_expanded .get_vec(), false);
    write_to_hdf(file, "K2b"+ ch, K2b_symmetry_expanded.get_vec(), false);
    write_to_hdf(file, "K3" + ch, K3_symmetry_expanded .get_vec(), false);

    write_to_hdf(file, "bfreqs1", K1_symmetry_expanded.get_VertexFreqGrid().b.get_all_frequencies(), false);
    write_to_hdf(file, "bfreqs2", K2_symmetry_expanded.get_VertexFreqGrid().b.get_all_frequencies(), false);
    write_to_hdf(file, "ffreqs2", K2_symmetry_expanded.get_VertexFreqGrid().f.get_all_frequencies(), false);
    write_to_hdf(file, "bfreqs3", K3_symmetry_expanded.get_VertexFreqGrid().b.get_all_frequencies(), false);
    write_to_hdf(file, "ffreqs3", K3_symmetry_expanded.get_VertexFreqGrid().f.get_all_frequencies(), false);

    file.close();

}

template <typename Q> template<char ch_bubble> auto rvert<Q>::value(VertexInput input, const rvert<Q>& rvert_crossing) const -> Q {

    //VertexInput input_tmp = input;
    transfToR<ch_bubble>(input); // input manipulated here => input needs to be called by value

    Q val;   // force zero initialization

    if constexpr(MAX_DIAG_CLASS >= 0) val = valsmooth<k1>(input, rvert_crossing);
    if constexpr(MAX_DIAG_CLASS >= 2) {
        val += valsmooth<k2> (input, rvert_crossing);
        val += valsmooth<k2b>(input, rvert_crossing);
    }
    if constexpr(MAX_DIAG_CLASS >= 3) val += valsmooth<k3>(input, rvert_crossing);

    return val;
}
template <typename Q> template<char ch_bubble> auto rvert<Q>::value(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {

    VertexInput input_tmp = input;
    transfToR<ch_bubble>(input_tmp);   // input might be in different channel parametrization


    Q val;   // force zero initialization

    if constexpr(MAX_DIAG_CLASS >= 0) val = valsmooth<k1>(input_tmp, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    if constexpr(MAX_DIAG_CLASS >= 2) {
        val += valsmooth<k2> (input_tmp, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
        val += valsmooth<k2b>(input_tmp, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    }
    if constexpr(MAX_DIAG_CLASS >= 3) val += valsmooth<k3>(input_tmp, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);

    return val;
}

template <typename Q> auto rvert<Q>::left_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q {
    if      constexpr(MAX_DIAG_CLASS == 1) return valsmooth<k1>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS  > 1) return valsmooth<k1>(input, rvert_crossing) + valsmooth<k2b>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::left_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    else if constexpr(MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel) + valsmooth<k2b>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
}

template <typename Q> auto rvert<Q>::right_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing) + valsmooth<k2>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::right_same_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)     return valsmooth<k1>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    else if constexpr(MAX_DIAG_CLASS > 1) return valsmooth<k1>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel) + valsmooth<k2>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
}

template <typename Q> auto rvert<Q>::left_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)      return 0.;
    else if constexpr(MAX_DIAG_CLASS == 2) return valsmooth<k2>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS == 3) return valsmooth<k2>(input, rvert_crossing) + valsmooth<k3>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::left_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)      return 0.;
    else if constexpr (MAX_DIAG_CLASS == 2) return valsmooth<k2>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    else if constexpr (MAX_DIAG_CLASS == 3) return valsmooth<k2>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel) + valsmooth<k3>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
}

template <typename Q> auto rvert<Q>::right_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)      return 0.;
    else if constexpr(MAX_DIAG_CLASS == 2) return valsmooth<k2b>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS == 3) return valsmooth<k2b>(input, rvert_crossing) + valsmooth<k3>(input, rvert_crossing);
}

template <typename Q> auto rvert<Q>::right_diff_bare(const VertexInput& input, const rvert<Q>& rvert_crossing, const rvert<Q>& vertex_half2_samechannel, const rvert<Q>& vertex_half2_switchedchannel) const -> Q {
    if constexpr(MAX_DIAG_CLASS == 1)      return 0.;
    else if constexpr(MAX_DIAG_CLASS == 2) return valsmooth<k2b>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
    else if constexpr(MAX_DIAG_CLASS == 3) return valsmooth<k2b>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel) + valsmooth<k3>(input, rvert_crossing, vertex_half2_samechannel, vertex_half2_switchedchannel);
}

template <typename Q> template<char ch_bubble, typename result_type> auto rvert<Q>::value_symmetry_expanded(VertexInput input, const rvert<Q>& rvert_crossing) const -> result_type{

    //VertexInput input_tmp = input;
    transfToR<ch_bubble>(input); // input manipulated here => input needs to be called by value

    auto val = valsmooth_symmetry_expanded<k1, result_type>(input, rvert_crossing);
    if constexpr(MAX_DIAG_CLASS >= 2) {
        val += valsmooth_symmetry_expanded<k2, result_type> (input, rvert_crossing);
        val += valsmooth_symmetry_expanded<k2b, result_type>(input, rvert_crossing);
    }
    if constexpr(MAX_DIAG_CLASS >= 3) val += valsmooth_symmetry_expanded<k3, result_type>(input, rvert_crossing);

    return val;
}

template <typename Q>template<typename result_type>  auto rvert<Q>::left_same_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> result_type{
    if      constexpr (MAX_DIAG_CLASS == 1) return valsmooth_symmetry_expanded<k1,result_type>(input, rvert_crossing);
    else if constexpr (MAX_DIAG_CLASS  > 1) return valsmooth_symmetry_expanded<k1,result_type>(input, rvert_crossing) + valsmooth_symmetry_expanded<k2b,result_type>(input, rvert_crossing);
}

template <typename Q> template<typename result_type>  auto rvert<Q>::right_same_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const -> result_type {
    if constexpr(MAX_DIAG_CLASS == 1)     return valsmooth_symmetry_expanded<k1,result_type>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS > 1) return valsmooth_symmetry_expanded<k1,result_type>(input, rvert_crossing) + valsmooth_symmetry_expanded<k2,result_type>(input, rvert_crossing);
}

template <typename Q> template<typename result_type>  auto rvert<Q>::left_diff_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const  -> result_type{
    //using result_type = decltype(valsmooth_symmetry_expanded<k1,result_type>(std::declval<VertexInput>(), std::declval<rvert<Q>>()));
    if constexpr(MAX_DIAG_CLASS == 1)      if constexpr(std::is_same_v < result_type, Q > ) {return result_type{};} else {return result_type::Zero();}
    else if constexpr(MAX_DIAG_CLASS == 2) return valsmooth_symmetry_expanded<k2,result_type>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS == 3) return valsmooth_symmetry_expanded<k2,result_type>(input, rvert_crossing) + valsmooth_symmetry_expanded<k3,result_type>(input, rvert_crossing);
}

template <typename Q> template<typename result_type>  auto rvert<Q>::right_diff_bare_symmetry_expanded(const VertexInput& input, const rvert<Q>& rvert_crossing) const  -> result_type{
    //using result_type = decltype(valsmooth_symmetry_expanded<k1,result_type>(std::declval<VertexInput>(), std::declval<rvert<Q>>()));
    if constexpr(MAX_DIAG_CLASS == 1)      if constexpr(std::is_same_v < result_type, Q > ) {return result_type{};} else {return result_type::Zero();}
    else if constexpr(MAX_DIAG_CLASS == 2) return valsmooth_symmetry_expanded<k2b,result_type>(input, rvert_crossing);
    else if constexpr(MAX_DIAG_CLASS == 3) return valsmooth_symmetry_expanded<k2b,result_type>(input, rvert_crossing) + valsmooth_symmetry_expanded<k3,result_type>(input, rvert_crossing);
}


template<typename Q>
void rvert<Q>::cross_project() {
    // TODO(high): Rewrite and update!!
    if (!calculated_crossprojections){
        switch (channel) {
            case 'a':
                    K1_a_proj = K1;
                    K1_crossproject('p');
                    K1_crossproject('t');
                if (MAX_DIAG_CLASS >= 2){
                    K2_a_proj = K2;
                    K2_crossproject('p');
                    K2_crossproject('t');
                }
                if (MAX_DIAG_CLASS == 3){
                    K3_a_proj = K3;
                    K3_crossproject('p');
                    K3_crossproject('t');
                }
                break;
            case 'p':
                    K1_crossproject('a');
                    K1_p_proj = K1;
                    K1_crossproject('t');
                if (MAX_DIAG_CLASS >= 2){
                    K2_crossproject('a');
                    K2_p_proj = K2;
                    K2_crossproject('t');
                }
                if (MAX_DIAG_CLASS == 3){
                    K3_crossproject('a');
                    K3_p_proj = K3;
                    K3_crossproject('t');
                }
                break;
            case 't':
                    K1_crossproject('a');
                    K1_crossproject('p');
                    K1_t_proj = K1;
                if (MAX_DIAG_CLASS >= 2){
                    K2_crossproject('a');
                    K2_crossproject('p');
                    K2_t_proj = K2;
                }
                if (MAX_DIAG_CLASS == 3){
                    K3_crossproject('a');
                    K3_crossproject('p');
                    K3_t_proj = K3;
                }
                break;
            default:
                print("Error! Invalid channel index!"); assert(false);
        }
        calculated_crossprojections = true;
    }
    else{
        print("Error! Crossprojections have already been calculated!");
        assert(false);
    }
}


template <typename Q>template<char ch_bubble>  void rvert<Q>::transfToR(VertexInput& input) const {
    double w, v1, v2;

    // Needed for finite-temperature Matsubara
    double floor2bf_w;
    double floor2bf_inputw;
    if (!KELDYSH and !ZERO_T){ floor2bf_inputw = floor2bfreq(input.w / 2.);}
    switch (channel) {
        case 'a':
            switch (ch_bubble) {
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
                        v2 = input.v1 + floor2bf_w - w - floor2bf_inputw;               // input.v2 = v'_t
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
            switch (ch_bubble) {
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
                        v1 =          - input.v1 + floor2bf_w + floor2bf_inputw;                 // input.v1 = v_t
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
            switch (ch_bubble) {
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
                        v1 =-input.v1           + floor2bf_w + floor2bf_inputw;             // input.v1 = v_p
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
    if (MAX_DIAG_CLASS >= 1) {
        K1.update_grid(Lambda);
    }
    if (MAX_DIAG_CLASS >= 2) {
        K2.update_grid(Lambda);
    }
    if (MAX_DIAG_CLASS >= 3) {
        K3.update_grid(Lambda);
    }
}




template <typename Q>
template<K_class k, typename FGrid>
void rvert<Q>::update_grid(const FGrid& frequencies_new, rvert<Q>& rvert4data) {
    if constexpr(k == k1) {
        K1.update_grid(frequencies_new, rvert4data.K1);
    }
    else if constexpr(k == k2) {
        K2.update_grid(frequencies_new, rvert4data.K2);
    }
    else if constexpr(k == k3) {
        K3.update_grid(frequencies_new, rvert4data.K3);
    }
    else assert(false);

}

namespace {

    template<typename Q>
    class CostFullvert_Wscale_b_K1 {
        using freqGrid_type_K1 = bufferFrequencyGrid<k1>;

        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        freqGrid_type_K1 frequencies = rVert.K1.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_b_K1(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (double wscale_test) -> double {
#ifdef HYBRID_GRID

#else
            frequencies.b.update_Wscale(wscale_test);
#endif
            rVert.template update_grid<k1>(frequencies, rVert_backup);
            //rVert.K1.analyze_tails_K1();
            double result = rVert.K1.get_curvature_max();

            if (verbose and mpi_world_rank() == 0) {
                std::cout << "max. Curvature in K1" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale = " << wscale_test << std::endl;

            }
            /*
            std::string filename = "K1_costCurvature_" + std::to_string(wscale_test) + ".h5";
            rvec v = rVert.K1.frequencies.get_freqGrid_b().get_all_frequencies();
            rvec SE_re = rVert.K1.get_vec().real();
            rvec SE_im = rVert.K1.get_vec().imag();
            write_h5_rvecs(filename,
                           {"v", "SE_re", "SE_im"},
                           {v, SE_re, SE_im});
            */
            return result;
        }
    };

    template<typename Q>
    class CostFullvert_Wscale_b_K2 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K2 frequencies = rVert.K2.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_b_K2(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (double wscale_test) -> double {
            frequencies.b.update_Wscale(wscale_test);
#ifdef ROTATEK2
            frequencies.f.update_Wscale(wscale_test);
#endif
            rVert.template update_grid<k2>(frequencies, rVert_backup);
            //rVert.K2.analyze_tails_K2_b();
            double result = rVert.K2.get_curvature_max();

            if (verbose and mpi_world_rank() == 0) {
                std::cout << "max. Curvature in K2" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale = " << wscale_test << std::endl;

            }

            return result;
        }
    };

    template<typename Q>
    class CostFullvert_Wscale_f_K2 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K2 frequencies = rVert.K2.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_f_K2(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (double wscale_test) -> double {
#ifdef ROTATEK2
            frequencies.b.update_Wscale(wscale_test);
#endif
            frequencies.f.update_Wscale(wscale_test);
            rVert.template update_grid<k2>(frequencies, rVert_backup);
            //rVert.K2.analyze_tails_K2_f();
            double result = rVert.K2.get_curvature_max();

            if (verbose and mpi_world_rank() == 0) {
                std::cout << "max. Curvature in K2" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale = " << wscale_test << std::endl;

            }

            return result;
        }
    };


    template<typename Q>
    class CostFullvert_Wscale_K2 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K2 frequencies = rVert.K2.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_K2(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (std::vector<double> Wscales) -> double
        {
            const double wscale_test_b = std::abs(Wscales[0]); // make sure Wscale is positive
            const double wscale_test_f = std::abs(Wscales[1]); // make sure Wscale is positive
            frequencies.b.update_Wscale(wscale_test_b);
            frequencies.f.update_Wscale(wscale_test_f);
            rVert.template update_grid<k2>(frequencies, rVert_backup);
            //rVert.K2.analyze_tails_K2_f();
            double result = rVert.K2.get_curvature_max();

            if (verbose and mpi_world_rank() == 0)
            {
                std::cout << "max. Curvature in K2" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale_w = " << wscale_test_b  << "\t\t with wscale_v = " << wscale_test_f << std::endl;

            }

            return result;
        }
    };



    template<typename Q>
    class CostFullvert_Wscale_b_K3 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K3 frequencies = rVert.K3.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_b_K3(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (double wscale_test) -> double {
            frequencies.b.update_Wscale(wscale_test);
            rVert.template update_grid<k3>(frequencies, rVert_backup);
            //rVert.K3.analyze_tails_K3_b();
            double result = rVert.K3.get_curvature_max();

            if (verbose and mpi_world_rank() == 0) {
                std::cout << "max. Curvature in K3" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale = " << wscale_test << std::endl;
            }

            return result;
        }
    };

    template<typename Q>
    class CostFullvert_Wscale_f_K3 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K3 frequencies = rVert.K3.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_f_K3(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (double wscale_test) -> double {
            if (BOSONIC_PARAM_FOR_K3) {
                /// grids are identical for bosonic parametrization
                frequencies.b.update_Wscale(wscale_test);
            }
            frequencies.f.update_Wscale(wscale_test);
            rVert.template update_grid<k3>(frequencies, rVert_backup);
            //rVert.K3.analyze_tails_K3_b();
            double result = rVert.K3.get_curvature_max();

            if (verbose and mpi_world_rank() == 0) {
                std::cout << "max. Curvature in K3" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale = " << wscale_test << std::endl;

            }

            return result;
        }    };


    template<typename Q>
    class CostFullvert_Wscale_K3 {
        rvert<Q> rVert_backup;
        bool verbose;
    public:
        rvert<Q> rVert;
        typename rvert<Q>::freqGrid_type_K3 frequencies = rVert.K3.get_VertexFreqGrid();
        explicit CostFullvert_Wscale_K3(rvert<Q> rvert_in, bool verbose) : rVert(rvert_in), rVert_backup(rvert_in), verbose(verbose) {};

        auto operator() (std::vector<double> Wscales) -> double
        {
            const double wscale_test_b = std::abs(Wscales[0]); // make sure Wscale is positive
            const double wscale_test_f = std::abs(Wscales[1]); // make sure Wscale is positive
            frequencies.b.update_Wscale(wscale_test_b);
            frequencies.f.update_Wscale(wscale_test_f);
            rVert.template update_grid<k3>(frequencies, rVert_backup);
            //rVert.K3.analyze_tails_K3_f();
            double result = rVert.K3.get_curvature_max();

            if (verbose and mpi_world_rank() == 0)
            {
                std::cout << "max. Curvature in K3" << rVert.channel; // << std::endl;
                std::cout << "\t \t" << result  << "\t\t with wscale_w = " << wscale_test_b  << "\t\t with wscale_v = " << wscale_test_f << std::endl;

            }

            return result;
        }
    };

}

template <typename Q> void rvert<Q>::findBestFreqGrid(bool verbose) {
    K1.optimize_grid(true);
    if constexpr(MAX_DIAG_CLASS > 1) K2.optimize_grid(true);
    if constexpr(MAX_DIAG_CLASS > 2) K3.optimize_grid(true);
}

template <typename Q> void rvert<Q>::enforce_freqsymmetriesK1(const rvert<Q>& vertex_symmrelated) { //TODO(medium): Do this also for the cross-projected parts
#pragma omp parallel for
    for (int iflat = 0; iflat < getFlatSize(K1.get_dims()); iflat++) {
        my_defs::K1::index_type idx;
        getMultIndex<rank_K1>(idx, iflat, K1.get_dims());
        int itK             = (int) idx[my_defs::K1::keldysh];
        my_index_t it_spin  = idx[my_defs::K1::spin];
        my_index_t itw      = idx[my_defs::K1::omega];
        my_index_t i_in     = idx[my_defs::K1::internal];

        int i0_tmp = 0;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a':
                i0_tmp = non_zero_Keldysh_K1a[itK];
                break;
            case 'p':
                i0_tmp = non_zero_Keldysh_K1p[itK];
                break;
            case 't':
                i0_tmp = non_zero_Keldysh_K1t[itK];
                break;
            default:;
        }
        double w_in;
        K1.frequencies.get_freqs_w(w_in, itw);
        IndicesSymmetryTransformations indices(i0_tmp, it_spin, w_in, 0., 0., i_in, channel, k1, 0, channel);
        int sign_w = sign_index(indices.w);
        int trafo_index = freq_transformations.K1[itK][ sign_w];
        if (trafo_index != 0) {
            Ti(indices, trafo_index);
            indices.iK = itK;

            Q result;
            if (indices.asymmetry_transform)
                result = read_symmetryreduced_rvert<k1>(indices, vertex_symmrelated);
            else
                result = read_symmetryreduced_rvert<k1>(indices, *this);

            K1.setvert(result, idx);
        }

    }
}

template<typename Q>
void rvert<Q>::K1_crossproject(const char channel_out) {
    /// Prescription: For K1 it suffices to calculate the average over the BZ, independent of the momentum argument and of the channel.
    for (int iK = 0; iK < nK_K1; ++iK) {
        for (int it_spin = 0; it_spin < n_spin; it_spin++) {
#pragma omp parallel for schedule(dynamic)
            for (int iw = 0; iw < nw1; ++iw) {
                Q projected_value = K1_BZ_average(iK, it_spin, iw);
                for (int i_in = 0; i_in < n_in_K1; ++i_in) {
                    switch (channel_out) {
                        case 'a':
                            K1_a_proj.setvert(projected_value, it_spin, iw, iK, i_in);
                            break; // All internal arguments get the same value for K1!
                        case 'p':
                            K1_p_proj.setvert(projected_value, it_spin, iw, iK, i_in);
                            break;
                        case 't':
                            K1_t_proj.setvert(projected_value, it_spin, iw, iK, i_in);
                            break;
                        default:
                            print("Incompatible channel for K1 cross-projection!");
                    }
                }
            }
        }
    }
}

template<typename Q>
Q rvert<Q>::K1_BZ_average(const int iK, const int ispin, const int iw) {
    /// Perform the average over the BZ by calculating the q-sum over the REDUCED BZ (see notes for details!)
    Q value = 0.;
    value +=      K1.val(ispin, iw, iK, momentum_index(0, 0));
    value +=      K1.val(ispin, iw, iK, momentum_index(glb_N_q - 1, glb_N_q - 1));
    value += 2. * K1.val(ispin, iw, iK, momentum_index(glb_N_q - 1, 0));
    for (int n = 1; n < glb_N_q - 1; ++n) {
        value += 4. * K1.val(ispin, iw, iK, momentum_index(n, 0));
        value += 4. * K1.val(ispin, iw, iK, momentum_index(glb_N_q - 1, n));
        value += 4. * K1.val(ispin, iw, iK, momentum_index(n, n));
        for (int np = 1; np < n; ++np) {
            value += 8. * K1.val(ispin, iw, iK, momentum_index(n, np));
        }
    }
    value /= 4. * (glb_N_q - 1) * (glb_N_q - 1);
    return value;
}

template <typename Q> void rvert<Q>::enforce_freqsymmetriesK2(const rvert<Q>& vertex_symmrelated) { //TODO(medium): Do this also for the cross-projected parts
#pragma omp parallel for
    for (int iflat = 0; iflat < getFlatSize(K2.get_dims()); iflat++) {
        my_defs::K2::index_type idx;
        getMultIndex<rank_K2>(idx, iflat, K2.get_dims());
        int itK             = (int) idx[my_defs::K2::keldysh];
        my_index_t it_spin  = idx[my_defs::K2::spin];
        my_index_t itw      = idx[my_defs::K2::omega];
        my_index_t itv      = idx[my_defs::K2::nu];
        my_index_t i_in     = idx[my_defs::K2::internal];

        int i0_tmp;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        switch (channel) {
            case 'a': i0_tmp = non_zero_Keldysh_K2a[itK]; break;
            case 'p': i0_tmp = non_zero_Keldysh_K2p[itK]; break;
            case 't': i0_tmp = non_zero_Keldysh_K2t[itK]; break;
            default: ;
        }
        double w_in, v_in;
        K2.frequencies.get_freqs_w(w_in, v_in, itw, itv);
        IndicesSymmetryTransformations indices(i0_tmp, it_spin, w_in, v_in, 0., i_in, channel, k2, 0, channel);
        int sign_w = sign_index(w_in);
        int sign_v1 = sign_index(v_in);
        int trafo_index = freq_transformations.K2[itK][ sign_w * 2 + sign_v1];
        Ti(indices, trafo_index);
        indices.iK = itK;


        if (trafo_index != 0) {

            Q result;
            if (indices.asymmetry_transform)
                result = read_symmetryreduced_rvert<k2>(indices, vertex_symmrelated);
            else
                result = read_symmetryreduced_rvert<k2>(indices, *this);

            K2.setvert(result, idx);
        }
        if (!KELDYSH and !ZERO_T and -v_in + signFlipCorrection_MF(w_in)*0.5 < K2.frequencies.get_wlower_f()) {
            K2.setvert(0., idx);                    }
    }

}

template<typename Q>
void rvert<Q>::K2_crossproject(char channel_out) {
// TODO: Currently does nothing!
}


template <typename Q> void rvert<Q>::enforce_freqsymmetriesK3(const rvert<Q>& vertex_symmrelated) { //TODO(medium): Do this also for the cross-projected parts
#pragma omp parallel for
    for (int iflat = 0; iflat < getFlatSize(K3.get_dims()); iflat++) {
        my_defs::K3::index_type idx;
        getMultIndex<rank_K3>(idx, iflat, K3.get_dims());
        int itK             = (int) idx[my_defs::K3::keldysh];
        my_index_t it_spin  = idx[my_defs::K3::spin];
        my_index_t itw      = idx[my_defs::K3::omega];
        my_index_t itv      = idx[my_defs::K3::nu];
        my_index_t itvp     = idx[my_defs::K3::nup];
        my_index_t i_in     = idx[my_defs::K3::internal];

        int i0_tmp;
        // converting index i0_in (0 or 1) into actual Keldysh index i0 (0,...,15)
        i0_tmp = non_zero_Keldysh_K3[itK];


        double w_in, v_in, vp_in;
        K3.frequencies.get_freqs_w(w_in, v_in, vp_in, itw, itv, itvp);
        IndicesSymmetryTransformations indices(i0_tmp, it_spin, w_in, v_in, vp_in, i_in, channel, k2, 0, channel);
        int sign_w = sign_index(w_in);

        int sign_f;
        if (!KELDYSH and !ZERO_T) sign_f = sign_index(indices.v1 + indices.v2 - signFlipCorrection_MF(w_in)*0.5);
        else sign_f = sign_index(indices.v1 + indices.v2);
        int sign_fp = sign_index(indices.v1 - indices.v2);
        int trafo_index = freq_transformations.K3[itK][ sign_w * 4 + sign_f * 2 + sign_fp];
        Ti(indices, trafo_index);
        indices.iK = itK;


        if (trafo_index != 0) {

            Q result;
            if (indices.asymmetry_transform)
                result = read_symmetryreduced_rvert<k3>(indices,
                                                        vertex_symmrelated); // vertex_symmrelated.K3.interpolate(indices);
            else
                result = read_symmetryreduced_rvert<k3>(indices, *this);

            K3.setvert(result, idx);
        }
        if (!KELDYSH and !ZERO_T and (-v_in + signFlipCorrection_MF(w_in)*0.5 < K3.frequencies.get_wlower_f() or -vp_in + signFlipCorrection_MF(w_in)*0.5 < K3.frequencies.get_wlower_f())) {
            K3.setvert(0., idx);
        }


    }

}


template<typename Q>
void rvert<Q>::K3_crossproject(char channel_out) {
    // TODO: Currently does nothing!
}







#endif //KELDYSH_MFRG_R_VERTEX_HPP