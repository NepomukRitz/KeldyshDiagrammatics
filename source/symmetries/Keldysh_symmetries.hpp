/**
 * Auxiliary functions for conversions of Keldysh indices
 */
#ifndef KELDYSH_MFRG_KELDYSH_SYMMETRIES_HPP
#define KELDYSH_MFRG_KELDYSH_SYMMETRIES_HPP

#include <vector>     // standard std::vector
#include <algorithm>  // for find function in isInList
#include "../data_structures.hpp"
#include "../utilities/util.hpp"     // printing text output
#include "../grids/frequency_grid.hpp"

/// Keldysh index parameters ///

template <std::size_t _rank>
struct buffer_config {
    using index_type = std::array<my_index_t, _rank>;
    using dimensions_type = std::array<my_index_t, _rank>;

    dimensions_type dims;

    my_index_t num_freqs;
    my_index_t position_first_freq_index;

    /// the following members are derived from the above ones
    const std::size_t dims_flat = getFlatSize(dims);
    const size_t rank = _rank;

    constexpr buffer_config(dimensions_type dims_in, my_index_t num_freqs_in,
                  my_index_t position_first_freq_index_in): dims(dims_in),
                  num_freqs(num_freqs_in), position_first_freq_index(position_first_freq_index_in){};
};

#if DEBUG_SYMMETRIES
constexpr buffer_config<3> SE_config{
    std::array<size_t,3>({KELDYSH ? 4 : 1, nFER, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1at_config{
    std::array<size_t,4>({n_spin, nBOS, KELDYSH ? 16 : 1, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1p_config {
    std::array<size_t,4>({n_spin, nBOS, KELDYSH ? 16 : 1, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2at_config{
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH ? 16 : 1, n_in})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2p_config {
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH ? 16 : 1, n_in})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<6> K3_config{
    std::array<size_t,6>({n_spin, nBOS3, nFER3, GRID!=2 ? nFER3 : (nFER3-1)/2+1, KELDYSH ? 16 : 1, n_in})
    , 3  // number of frequency dimensions
    , 1};// position of first frequency index
#else
#if CONTOUR_BASIS != 1
constexpr buffer_config<3> SE_config{
    std::array<size_t,3>({ KELDYSH ? 3 : 1, nFER, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1at_config{
    std::array<size_t,4>({n_spin, nBOS, KELDYSH ? 16 : 1, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1p_config {
    std::array<size_t,4>({n_spin, nBOS, KELDYSH ? 16 : 1, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2at_config{
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH ? 16 : 1, n_in})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2p_config {
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH ? 16 : 1, n_in})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<6> K3_config{
    std::array<size_t,6>({n_spin, nBOS3, nFER3, GRID!=2 ? nFER3 : (nFER3-1)/2+1, KELDYSH ? 16 : 1, n_in})
    , 3  // number of frequency dimensions
    , 1};// position of first frequency index
#else
#if not PARTICLE_HOLE_SYMM
constexpr buffer_config<3> SE_config{
    std::array<size_t,3>({ 3, nFER, n_in_K1})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1at_config{
    std::array<size_t,4>({n_spin, nBOS, 2, n_in_K1})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1p_config {
    std::array<size_t,4>({n_spin, nBOS, 3, n_in_K1})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2at_config{
    std::array<size_t,5>({n_spin, nBOS2, nFER2, 4, n_in_K2})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2p_config {
    std::array<size_t,5>({n_spin, nBOS2, nFER2, 6, n_in_K2})
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<6> K3_config{
    std::array<size_t,6>({n_spin, nBOS3, nFER3, GRID!=2 ? nFER3 : (nFER3-1)/2+1, 7, n_in_K3})
    , 3  // number of frequency dimensions
    , 1};// position of first frequency index
#else
constexpr buffer_config<3> SE_config{
    std::array<size_t,3>({ KELDYSH_FORMALISM ? 3 : 1, nFER, n_in_K1}) // dims
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1at_config{
    std::array<size_t,4>({n_spin, nBOS, KELDYSH_FORMALISM ? 3 : 1, n_in_K1}) // dims
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1p_config {
    std::array<size_t,4>({n_spin, nBOS, KELDYSH_FORMALISM ? 3 : 1, n_in_K1}) // dims
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2at_config{
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH_FORMALISM ? 3 : 1, n_in_K2}) // dims
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2p_config {
    std::array<size_t,5>({n_spin, nBOS2, nFER2, KELDYSH_FORMALISM ? 3 : 1, n_in_K2}) // dims
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<6> K3_config{
    std::array<size_t,6>({n_spin, nBOS3, nFER3, GRID!=2 ? nFER3 : (nFER3-1)/2+1, KELDYSH_FORMALISM ? 5 : 1, n_in_K3}) // dims
    , 3  // number of frequency dimensions
    , 1};// position of first frequency index
#endif
#endif
#endif

constexpr buffer_config<3> SE_expanded_config{
    std::array<size_t,3>({ KELDYSH ?  4 : 1, nFER, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<4> K1_expanded_config{
    std::array<size_t,4>({1, nBOS, KELDYSH ?  16 : 1, n_in})
    , 1  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<5> K2_expanded_config{
#if MAX_DIAG_CLASS >=2
    std::array<size_t,5>({1, nBOS2, nFER2, KELDYSH ?  16 : 1, n_in})
#else
    std::array<size_t,5>({1, 0, 0, KELDYSH ?  16 : 1, n_in_K2})
#endif
    , 2  // number of frequency dimensions
    , 1};// position of first frequency index
constexpr buffer_config<6> K3_SBE_expanded_config{
#if MAX_DIAG_CLASS >= 2
        std::array<size_t,6>({1, nBOS2, nFER2, GRID!=2 ? nFER2 : (nFER2-1)/2+1, KELDYSH ?  16 : 1, n_in})
#else
        std::array<size_t,6>({1, 0, 0, 0, KELDYSH ? 16 : 1, n_in_K3})
#endif
        , 3  // number of frequency dimensions
        , 1};// position of first frequency index
constexpr buffer_config<6> K3_expanded_config{
#if MAX_DIAG_CLASS == 3
    std::array<size_t,6>({1, nBOS3, nFER3, GRID!=2 ? nFER3 : (nFER3-1)/2+1, KELDYSH ?  16 : 1, n_in})
#else
    std::array<size_t,6>({1, 0, 0, 0, KELDYSH ? 16 : 1, n_in_K3})
#endif
    , 3  // number of frequency dimensions
    , 1};// position of first frequency index

constexpr unsigned int pos_first_freq = 1;  // position of first frequency index

#if KELDYSH_FORMALISM
// Vector of indices of the non-zero Keldysh components of the bubbles
const std::vector<int> glb_non_zero_Keldysh_bubble({3,6,7,9,11,12,13,14,15});
constexpr int glb_number_of_Keldysh_components_bubble = 9; // length of the previous vector

// Vector of indices of independent components of the diagrammatic classes, density channel
#if CONTOUR_BASIS != 1
//const std::vector<int> non_zero_Keldysh_K1a({1,3,15});
//const std::vector<int> non_zero_Keldysh_K1p({1,5,15});
//const std::vector<int> non_zero_Keldysh_K1t({1,3,15});
#if SBE_DECOMPOSITION
const std::vector<int> non_zero_Keldysh_K2a({0,1,2,3,9});
const std::vector<int> non_zero_Keldysh_K2p({0,1,4,5,12});
const std::vector<int> non_zero_Keldysh_K2t({0,1,2,3,5});
#else
//const std::vector<int> non_zero_Keldysh_K2a({0,1,2,3,11});
//const std::vector<int> non_zero_Keldysh_K2p({0,1,4,5,13});
//const std::vector<int> non_zero_Keldysh_K2t({0,1,2,3,7});
#endif
//const std::vector<int> non_zero_Keldysh_K3({0,1,3,5,6,7});
#else
#if not PARTICLE_HOLE_SYMM
const std::vector<int> non_zero_Keldysh_K1a({0,6});
const std::vector<int> non_zero_Keldysh_K2a({0,1,6,8});
const std::vector<int> non_zero_Keldysh_K1p({0,3,12});
const std::vector<int> non_zero_Keldysh_K2p({0,3,4,7,12,15});
const std::vector<int> non_zero_Keldysh_K1t({0,5});
const std::vector<int> non_zero_Keldysh_K2t({0,1,4,5});
const std::vector<int> non_zero_Keldysh_K3({0,1,3,4,5,6,12});
#else
const std::vector<int> non_zero_Keldysh_K1a({0,6});
const std::vector<int> non_zero_Keldysh_K2a({0,1,6});
const std::vector<int> non_zero_Keldysh_K1p({0,3});
const std::vector<int> non_zero_Keldysh_K2p({0,3,4});
const std::vector<int> non_zero_Keldysh_K1t({0,5});
const std::vector<int> non_zero_Keldysh_K2t({0,1,5});
const std::vector<int> non_zero_Keldysh_K3({0,1,3,5,6});
#endif
#endif

// Vector of indices whose respective Keldysh indices add up to an odd number
const std::vector<int> odd_Keldysh({1, 2, 4, 7, 8, 11, 13, 14});
#else
const std::vector<int> glb_non_zero_Keldysh_bubble {0};
constexpr int glb_number_of_Keldysh_components_bubble = 1; // length of the previous vector


// Vector of indices of independent components of the diagrammatic classes, density channel
const std::vector<int> non_zero_Keldysh_K1a({0});
const std::vector<int> non_zero_Keldysh_K2a({0});
const std::vector<int> non_zero_Keldysh_K1p({0});
const std::vector<int> non_zero_Keldysh_K2p({0});
const std::vector<int> non_zero_Keldysh_K1t({0});
const std::vector<int> non_zero_Keldysh_K2t({0});
const std::vector<int> non_zero_Keldysh_K3({0});

const std::vector<int> odd_Keldysh({1});    // trivial, never used in Matsubara
#endif // KELDYSH_FORMALISM



// Checks if a given variable val is in a list passed by reference.
// Used to check if a Keldysh index is in a list of indices that should be equal.
template <typename T>
auto isInList (T val, const std::vector<T>& list) -> bool {
    return (std::find(list.begin(), list.end(), val) != list.end());
}

// This function converts indices in the range 0...5 to the actual Keldysh index they correspond to
// Rule: {0,1,3,5,6,7} -> {0,1,2,3,4,5}
// The components 0,1,3,5,6 and 7 are the chosen reference components, numerated in the 0...15 convention
auto convertToIndepIndex(int iK) -> int;

// This function returns the values of the 4 alphas for a given index in the 0...15 set
auto iK_to_alphas(int index) -> std::array<int,4>;
auto alphas_to_iK(const std::array<int,4>& alphas) -> int;



template<char channel_bubble, typename I> void get_i0_left_right(const my_index_t iK, I& i0_left, I& i0_right)  {
    constexpr std::array<I, 4> Keldysh4pointdims = {2,2,2,2};
    std::array<I ,4> alpha;
    getMultIndex(alpha, iK, Keldysh4pointdims);

    if constexpr(channel_bubble == 'a') {
        i0_left = alpha[0]*2 + alpha[3];
        i0_right= alpha[2]*2 + alpha[1];
    }
    else if constexpr(channel_bubble == 'p') {
        i0_left = alpha[0]*2 + alpha[1];
        i0_right= alpha[2]*2 + alpha[3];
    }
    else {
        static_assert(channel_bubble=='t', "Please use a, p or t for the channels.");
        i0_left = alpha[1]*2 + alpha[3];
        i0_right= alpha[2]*2 + alpha[0];
    }
}


template<char channel_bubble, bool is_left_vertex> auto rotate_Keldysh_matrix(const my_index_t iK) -> my_index_t {
    constexpr std::array<my_index_t, 4> Keldysh4pointdims = {2,2,2,2};
    std::array<my_index_t ,4> alpha;
    getMultIndex(alpha, iK, Keldysh4pointdims);

    my_index_t i0_left, i0_right;
    if constexpr(channel_bubble == 'a') {

        if constexpr(is_left_vertex) {
            i0_left  = alpha[0] * 2 + alpha[3];
            i0_right = alpha[2] * 2 + alpha[1];
        }
        else {
            i0_left  = alpha[2] * 2 + alpha[1];
            i0_right = alpha[0] * 2 + alpha[3];
        }
    }
    else if constexpr(channel_bubble == 'p') {
        if constexpr(is_left_vertex) {
            i0_left = alpha[0]*2 + alpha[1];
            i0_right= alpha[2]*2 + alpha[3];
        }
        else {
            i0_left = alpha[2]*2 + alpha[3];
            i0_right= alpha[0]*2 + alpha[1];

        }
    }
    else {
        static_assert(channel_bubble=='t', "Please use a, p or t for the channels.");

        if constexpr(is_left_vertex) {
            i0_left  = alpha[3] * 2 + alpha[0];
            i0_right = alpha[2] * 2 + alpha[1];
        }
        else {
            i0_left  = alpha[1] * 2 + alpha[2];
            i0_right = alpha[0] * 2 + alpha[3];

        }
    }

    const my_index_t iK_read = i0_left * 4 + i0_right;
    assert(iK_read < 16);
    return iK_read;
}

template<char channel_bubble, bool is_left_vertex> auto unrotate_Keldysh_matrix(const my_index_t iK) -> my_index_t {
    constexpr std::array<my_index_t, 4> Keldysh4pointdims = {2,2,2,2};
    std::array<my_index_t ,4> alpha;
    getMultIndex(alpha, iK, Keldysh4pointdims);

    my_index_t i0_left, i0_right;
    if constexpr(channel_bubble == 'a') {

        if constexpr(is_left_vertex) {
            i0_left  = alpha[0] * 2 + alpha[3];
            i0_right = alpha[2] * 2 + alpha[1];
        }
        else {
            i0_left  = alpha[2] * 2 + alpha[1];
            i0_right = alpha[0] * 2 + alpha[3];
        }
    }
    else if constexpr(channel_bubble == 'p') {
        if constexpr(is_left_vertex) {
            i0_left = alpha[0]*2 + alpha[1];
            i0_right= alpha[2]*2 + alpha[3];
        }
        else {
            i0_left = alpha[2]*2 + alpha[3];
            i0_right= alpha[0]*2 + alpha[1];

        }
    }
    else {
        static_assert(channel_bubble=='t', "Please use a, p or t for the channels.");

        if constexpr(is_left_vertex) {
            i0_left  = alpha[1] * 2 + alpha[3];
            i0_right = alpha[2] * 2 + alpha[0];
        }
        else {
            i0_left  = alpha[2] * 2 + alpha[0];
            i0_right = alpha[1] * 2 + alpha[3];

        }
    }

    const my_index_t iK_read = i0_left * 4 + i0_right;
    assert(iK_read < 16);
    return iK_read;
}


/**
 * Function that returns, for an input i0, i2 in 0...15, the two Keldysh indices of the left [0] and right [1] vertices
 * of a bubble in a given channel.
 * @param i0      : Keldysh index of the lhs of a derivative equation for the vertex
 * @param i2      : Keldysh index of the non-zero components of the bubble propagators (takes values in a set of size 9)
 * @param channel : channel of the bubble
 * @return        : Vector of two Keldysh indices for the left [0] and right [1] vertex in a bubble
 */
auto indices_sum(int i0, int i2, const char channel) -> std::vector<int>;




#endif //KELDYSH_MFRG_KELDYSH_SYMMETRIES_HPP
