/**
 * Functions to write/read a State object to/from an HDF5 file.
 */

#ifndef KELDYSH_MFRG_HDF5_ROUTINES_H
#define KELDYSH_MFRG_HDF5_ROUTINES_H

#include <stdexcept>
#include <cmath>
#include <vector>
#include "util.h"               // printing text
#include "../parameters/master_parameters.h"         // system parameters (necessary for vector lengths etc.)
#include "../data_structures.h"    // comp data type, std::real/complex vector class
#include "../grids/frequency_grid.h"     // store frequency grid parameters
#include "H5Cpp.h"              // HDF5 functions
#include "../state.h"

//template<typename Q> class State;
/// TODO: Save frequency grids and frequency parameters for each channel individually

#ifdef MPI_FLAG
#include "mpi_setup.h"          // mpi routines: when using mpi, only the process with ID 0 writes into file
#endif

// TODO(medium): Currently, global parameters are used to set the size of the buffer arrays.
//  Thus, in order to properly read data from a file, global parameters need to be the same as in the file
//  --> fix this: read buffer sizes (and dims) from file (-> Marc; Write this once and use for several projects!)
//  Also, always save parameters.h and FrequencyGrid.h (and whereever elso global parameters are stored)
//  for each computation and load it as well.

/// --- Constants concerning HDF5 data format --- ///

// Dataset dimensions
const int RANK_K1 = 2;
const int RANK_K2 = 2;
const int RANK_K3 = 2;
const int RANK_irreducible = 2;
const int RANK_self = 2;
const int RANK_freqs = 2;

const int N_freq_params = 64; // number of parameters for frequency grid

// Names of the individual datasets within the hdf5 file
const H5std_string	DATASET_irred("irred");

const H5std_string	DATASET_K1_a("K1_a");
const H5std_string	DATASET_K1_p("K1_p");
const H5std_string	DATASET_K1_t("K1_t");

const H5std_string	DATASET_K2_a("K2_a");
const H5std_string	DATASET_K2_p("K2_p");
const H5std_string	DATASET_K2_t("K2_t");

const H5std_string	DATASET_K3_a("K3_a");
const H5std_string	DATASET_K3_p("K3_p");
const H5std_string	DATASET_K3_t("K3_t");

const H5std_string	SELF_LIST("selflist");
const H5std_string	LAMBDA_LIST("lambdas");
const H5std_string  BFREQS_LISTa ("bfreqs_a");
const H5std_string  BFREQS_LISTp ("bfreqs_p");
const H5std_string  BFREQS_LISTt ("bfreqs_t");
const H5std_string  BFREQS2_LISTa("bfreqs2_a");
const H5std_string  BFREQS2_LISTp("bfreqs2_p");
const H5std_string  BFREQS2_LISTt("bfreqs2_t");
const H5std_string  BFREQS3_LISTa("bfreqs3_a");
const H5std_string  BFREQS3_LISTp("bfreqs3_p");
const H5std_string  BFREQS3_LISTt("bfreqs3_t");
const H5std_string  FFREQS_LIST ("ffreqs");
const H5std_string  FFREQS2_LISTa("ffreqs2_a");
const H5std_string  FFREQS2_LISTp("ffreqs2_p");
const H5std_string  FFREQS2_LISTt("ffreqs2_t");
const H5std_string  FFREQS3_LISTa("ffreqs3_a");
const H5std_string  FFREQS3_LISTp("ffreqs3_p");
const H5std_string  FFREQS3_LISTt("ffreqs3_t");
const H5std_string  FREQ_PARAMS("freq_params");
const H5std_string  PARAM_LIST("parameters");
const H5std_string  RE( "re" );
const H5std_string  IM( "im" );

/// --- Definitions of necessary data types --- ///

// Define struct to save complex numbers in hdf5 file
typedef struct h5_comp {
    double re; // std::real part
    double im; // imaginary part
} h5_comp;

// Create the memory data type for storing complex numbers in file
H5::CompType def_mtype_comp() {
    H5::CompType mtype_comp(sizeof(h5_comp));
    mtype_comp.insertMember(RE, HOFFSET(h5_comp, re), H5::PredType::NATIVE_DOUBLE);
    mtype_comp.insertMember(IM, HOFFSET(h5_comp, im), H5::PredType::NATIVE_DOUBLE);
    return mtype_comp;
}

/// --- Helper classes: buffer, dimension arrays, data sets --- ///

/**
 * Class containing buffer lengths and arrays to buffer data for selfenergy and irreducible vertex
 * as well as K1, K2, K3 classes.
 * Constructor creates new arrays, destructor deletes them.
 */
class Buffer {
public:
    double * lambda;
    double * freq_params;
    double * bfreqsa_buffer;
    double * bfreqsp_buffer;
    double * bfreqst_buffer;
    double * ffreqs_buffer;
    const int self_dim = collapse_all(dimsSE, [](const size_t& a, const size_t& b) -> size_t {return a*b;});    // length of self-energy buffer
#ifdef KELDYSH_FORMALISM
    const int irred_dim = 16 * n_in;                                  // length of irreducible vertex buffer
#else
    const int irred_dim = n_in;                                       // length of irreducible vertex buffer
#endif
    h5_comp * selfenergy;
    h5_comp * irreducible_class;

#if MAX_DIAG_CLASS >= 1
    const int K1_dim = collapse_all(dimsK1, [](const size_t& a, const size_t& b) -> size_t {return a*b;});                         // length of K1 buffer
    h5_comp * K1_class_a;
    h5_comp * K1_class_p;
    h5_comp * K1_class_t;
#endif
#if MAX_DIAG_CLASS >= 2
    double * bfreqs2a_buffer;
    double * ffreqs2a_buffer;
    double * bfreqs2p_buffer;
    double * ffreqs2p_buffer;
    double * bfreqs2t_buffer;
    double * ffreqs2t_buffer;
    const int K2_dim = collapse_all(dimsK2, [](const size_t& a, const size_t& b) -> size_t {return a*b;});               // length of K2 buffer
    h5_comp * K2_class_a;
    h5_comp * K2_class_p;
    h5_comp * K2_class_t;
#endif
#if MAX_DIAG_CLASS >= 3
    double * bfreqs3a_buffer;
    double * ffreqs3a_buffer;
    double * bfreqs3p_buffer;
    double * ffreqs3p_buffer;
    double * bfreqs3t_buffer;
    double * ffreqs3t_buffer;
    const int K3_dim = collapse_all(dimsK3, [](const size_t& a, const size_t& b) -> size_t {return a*b;});    // length of K3 buffer
    h5_comp * K3_class_a;
    h5_comp * K3_class_p;
    h5_comp * K3_class_t;
#endif

    Buffer() {
        lambda = new double [1];
        freq_params = new double[N_freq_params];
        bfreqsa_buffer = new double[nBOS+FREQ_PADDING*2];                        // create buffer for bosonic frequencies
        bfreqsp_buffer = new double[nBOS+FREQ_PADDING*2];                        // create buffer for bosonic frequencies
        bfreqst_buffer = new double[nBOS+FREQ_PADDING*2];                        // create buffer for bosonic frequencies
        ffreqs_buffer = new double[nFER+FREQ_PADDING*2];                        // create buffer for fermionic frequencies
        selfenergy = new h5_comp[self_dim];                           // create buffer for self-energy
        irreducible_class = new h5_comp[irred_dim];              // create buffer for irreducible vertex
#if MAX_DIAG_CLASS >= 1
        K1_class_a = new h5_comp[K1_dim];                        // create buffer for K1_a
        K1_class_p = new h5_comp[K1_dim];                        // create buffer for K1_p
        K1_class_t = new h5_comp[K1_dim];                        // create buffer for K1_t
#endif
#if MAX_DIAG_CLASS >= 2
        bfreqs2a_buffer = new double[nBOS2+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K2
        ffreqs2a_buffer = new double[nFER2+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K2
        bfreqs2p_buffer = new double[nBOS2+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K2
        ffreqs2p_buffer = new double[nFER2+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K2
        bfreqs2t_buffer = new double[nBOS2+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K2
        ffreqs2t_buffer = new double[nFER2+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K2
        K2_class_a = new h5_comp[K2_dim];                        // create buffer for K2_a
        K2_class_p = new h5_comp[K2_dim];                        // create buffer for K2_p
        K2_class_t = new h5_comp[K2_dim];                        // create buffer for K2_t
#endif
#if MAX_DIAG_CLASS >= 3
        bfreqs3a_buffer = new double[nBOS3+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K3
        ffreqs3a_buffer = new double[nFER3+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K3
        bfreqs3p_buffer = new double[nBOS3+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K3
        ffreqs3p_buffer = new double[nFER3+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K3
        bfreqs3t_buffer = new double[nBOS3+FREQ_PADDING*2];                      // create buffer for bosonic frequencies for K3
        ffreqs3t_buffer = new double[nFER3+FREQ_PADDING*2];                      // create buffer for fermionic frequencies for K3
        K3_class_a = new h5_comp[K3_dim];                        // create buffer for K3_a
        K3_class_p = new h5_comp[K3_dim];                        // create buffer for K3_p
        K3_class_t = new h5_comp[K3_dim];                        // create buffer for K3_t
#endif
    }

    ~Buffer() {
        delete[] freq_params;
        delete[] bfreqsa_buffer;
        delete[] bfreqsp_buffer;
        delete[] bfreqst_buffer;
        delete[] ffreqs_buffer;
        delete[] selfenergy;
        delete[] irreducible_class;
#if MAX_DIAG_CLASS >= 1
        delete[] K1_class_a;
        delete[] K1_class_p;
        delete[] K1_class_t;
#endif
#if MAX_DIAG_CLASS >= 2
        delete[] bfreqs2a_buffer;
        delete[] ffreqs2a_buffer;
        delete[] bfreqs2p_buffer;
        delete[] ffreqs2p_buffer;
        delete[] bfreqs2t_buffer;
        delete[] ffreqs2t_buffer;
        delete[] K2_class_a;
        delete[] K2_class_p;
        delete[] K2_class_t;
#endif
#if MAX_DIAG_CLASS >= 3
        delete[] bfreqs3a_buffer;
        delete[] ffreqs3a_buffer;
        delete[] bfreqs3p_buffer;
        delete[] ffreqs3p_buffer;
        delete[] bfreqs3t_buffer;
        delete[] ffreqs3t_buffer;
        delete[] K3_class_a;
        delete[] K3_class_p;
        delete[] K3_class_t;
#endif
    }

template <typename Q>
    void initialize(const State<Q>& state_in) {
        //print("Starting to copy to buffer...", true);
        FrequencyGrid bfreqsa = state_in.vertex[0].avertex().K1.K1_get_freqGrid();
        FrequencyGrid bfreqsp = state_in.vertex[0].pvertex().K1.K1_get_freqGrid();
        FrequencyGrid bfreqst = state_in.vertex[0].tvertex().K1.K1_get_freqGrid();
        FrequencyGrid ffreqs = state_in.selfenergy.frequencies;
        freq_params[0] = (double) bfreqsa.N_w;
        freq_params[1] = bfreqsa.w_upper;
        freq_params[2] = bfreqsa.w_lower;
        freq_params[3] = bfreqsa.W_scale;
        freq_params[4] = (double) ffreqs.N_w;
        freq_params[5] = ffreqs.w_upper;
        freq_params[6] = ffreqs.w_lower;
        freq_params[7] = ffreqs.W_scale;
        freq_params[24] = (double) bfreqsp.N_w;
        freq_params[25] = bfreqsp.w_upper;
        freq_params[26] = bfreqsp.w_lower;
        freq_params[27] = bfreqsp.W_scale;
        freq_params[28] = (double) bfreqst.N_w;
        freq_params[29] = bfreqst.w_upper;
        freq_params[30] = bfreqst.w_lower;
        freq_params[31] = bfreqst.W_scale;

    convert_vec_to_type(bfreqsa.get_ws_vec(), bfreqsa_buffer);
    convert_vec_to_type(bfreqsp.get_ws_vec(), bfreqsp_buffer);
    convert_vec_to_type(bfreqst.get_ws_vec(), bfreqst_buffer);
    convert_vec_to_type(ffreqs.get_ws_vec(), ffreqs_buffer);
        //for (int i=0; i<nBOS+FREQ_PADDING*2; ++i) {
        //    bfreqs_buffer[i] = bfreqs.ws[i];
        //}
        //for (int i=0; i<nFER+FREQ_PADDING*2; ++i) {
        //    ffreqs_buffer[i] = ffreqs.ws[i];
        //}
        for (int i=0; i<self_dim; ++i) {                        // write self-energy into buffer
#if defined(PARTICLE_HOLE_SYMM) and not defined(KELDYSH_FORMALISM)
            // in the particle-hole symmetric case in Matsubara we only save the imaginary part of the selfenergy
            selfenergy[i].re = glb_U/2.;
            selfenergy[i].im = state_in.selfenergy.acc(i);
#else
            selfenergy[i].re = std::real(state_in.selfenergy.acc(i));
            selfenergy[i].im = std::imag(state_in.selfenergy.acc(i));
#endif
        }
        for (int i=0; i<irred_dim; ++i) {                       // write irreducible vertex into buffer
            irreducible_class[i].re = std::real(state_in.vertex[0].irred().acc(i));
            irreducible_class[i].im = std::imag(state_in.vertex[0].irred().acc(i));
        }
#if MAX_DIAG_CLASS >= 1
        for(int i=0; i<K1_dim; ++i) {                                // write K1 into buffer
            K1_class_a[i].re = std::real(state_in.vertex[0].avertex().K1.acc(i));
            K1_class_a[i].im = std::imag(state_in.vertex[0].avertex().K1.acc(i));

            K1_class_p[i].re = std::real(state_in.vertex[0].pvertex().K1.acc(i));
            K1_class_p[i].im = std::imag(state_in.vertex[0].pvertex().K1.acc(i));

            K1_class_t[i].re = std::real(state_in.vertex[0].tvertex().K1.acc(i));
            K1_class_t[i].im = std::imag(state_in.vertex[0].tvertex().K1.acc(i));
        }
#endif
#if MAX_DIAG_CLASS >= 2
    FrequencyGrid bfreqs2a = state_in.vertex[0].avertex().K2.K2_get_freqGrid_b();
    FrequencyGrid ffreqs2a = state_in.vertex[0].avertex().K2.K2_get_freqGrid_f();
    FrequencyGrid bfreqs2p = state_in.vertex[0].pvertex().K2.K2_get_freqGrid_b();
    FrequencyGrid ffreqs2p = state_in.vertex[0].pvertex().K2.K2_get_freqGrid_f();
    FrequencyGrid bfreqs2t = state_in.vertex[0].tvertex().K2.K2_get_freqGrid_b();
    FrequencyGrid ffreqs2t = state_in.vertex[0].tvertex().K2.K2_get_freqGrid_f();
    freq_params[8]  = (double) bfreqs2a.N_w;
    freq_params[9]  = bfreqs2a.w_upper;
    freq_params[10] = bfreqs2a.w_lower;
    freq_params[11] = bfreqs2a.W_scale;
    freq_params[12] = (double) ffreqs2a.N_w;
    freq_params[13] = ffreqs2a.w_upper;
    freq_params[14] = ffreqs2a.w_lower;
    freq_params[15] = ffreqs2a.W_scale;
    freq_params[32]  = (double) bfreqs2p.N_w;
    freq_params[33]  = bfreqs2p.w_upper;
    freq_params[34] = bfreqs2p.w_lower;
    freq_params[35] = bfreqs2p.W_scale;
    freq_params[36] = (double) ffreqs2p.N_w;
    freq_params[37] = ffreqs2p.w_upper;
    freq_params[38] = ffreqs2p.w_lower;
    freq_params[39] = ffreqs2p.W_scale;
    freq_params[40]  = (double) bfreqs2t.N_w;
    freq_params[41]  = bfreqs2t.w_upper;
    freq_params[42] = bfreqs2t.w_lower;
    freq_params[43] = bfreqs2t.W_scale;
    freq_params[44] = (double) ffreqs2t.N_w;
    freq_params[45] = ffreqs2t.w_upper;
    freq_params[46] = ffreqs2t.w_lower;
    freq_params[47] = ffreqs2t.W_scale;

    convert_vec_to_type(bfreqs2a.get_ws_vec(), bfreqs2a_buffer);
    convert_vec_to_type(ffreqs2a.get_ws_vec(), ffreqs2a_buffer);
    convert_vec_to_type(bfreqs2p.get_ws_vec(), bfreqs2p_buffer);
    convert_vec_to_type(ffreqs2p.get_ws_vec(), ffreqs2p_buffer);
    convert_vec_to_type(bfreqs2t.get_ws_vec(), bfreqs2t_buffer);
    convert_vec_to_type(ffreqs2t.get_ws_vec(), ffreqs2t_buffer);
        //for (int i=0; i<nBOS2+FREQ_PADDING*2; ++i) {
        //    bfreqs2_buffer[i] = bfreqs2.ws[i];
        //}
        //for (int i=0; i<nFER2+FREQ_PADDING*2; ++i) {
        //    ffreqs2_buffer[i] = ffreqs2.ws[i];
        //}
        for(int i=0; i<K2_dim; ++i) {                                // write K2 into buffer
            K2_class_a[i].re = std::real(state_in.vertex[0].avertex().K2.acc(i));
            K2_class_a[i].im = std::imag(state_in.vertex[0].avertex().K2.acc(i));

            K2_class_p[i].re = std::real(state_in.vertex[0].pvertex().K2.acc(i));
            K2_class_p[i].im = std::imag(state_in.vertex[0].pvertex().K2.acc(i));

            K2_class_t[i].re = std::real(state_in.vertex[0].tvertex().K2.acc(i));
            K2_class_t[i].im = std::imag(state_in.vertex[0].tvertex().K2.acc(i));
        }
#endif
#if MAX_DIAG_CLASS >= 3
        FrequencyGrid bfreqs3a = state_in.vertex[0].avertex().K3.K3_get_freqGrid_b();
        FrequencyGrid ffreqs3a = state_in.vertex[0].avertex().K3.K3_get_freqGrid_f();
        FrequencyGrid bfreqs3p = state_in.vertex[0].pvertex().K3.K3_get_freqGrid_b();
        FrequencyGrid ffreqs3p = state_in.vertex[0].pvertex().K3.K3_get_freqGrid_f();
        FrequencyGrid bfreqs3t = state_in.vertex[0].tvertex().K3.K3_get_freqGrid_b();
        FrequencyGrid ffreqs3t = state_in.vertex[0].tvertex().K3.K3_get_freqGrid_f();
        freq_params[16] = (double) bfreqs3a.N_w;
        freq_params[17] = bfreqs3a.w_upper;
        freq_params[18] = bfreqs3a.w_lower;
        freq_params[19] = bfreqs3a.W_scale;
        freq_params[20] = (double) ffreqs3a.N_w;
        freq_params[21] = ffreqs3a.w_upper;
        freq_params[22] = ffreqs3a.w_lower;
        freq_params[23] = ffreqs3a.W_scale;
        freq_params[48] = (double) bfreqs3p.N_w;
        freq_params[49] = bfreqs3p.w_upper;
        freq_params[50] = bfreqs3p.w_lower;
        freq_params[51] = bfreqs3p.W_scale;
        freq_params[52] = (double) ffreqs3p.N_w;
        freq_params[53] = ffreqs3p.w_upper;
        freq_params[54] = ffreqs3p.w_lower;
        freq_params[55] = ffreqs3p.W_scale;
        freq_params[56] = (double) bfreqs3t.N_w;
        freq_params[57] = bfreqs3t.w_upper;
        freq_params[58] = bfreqs3t.w_lower;
        freq_params[59] = bfreqs3t.W_scale;
        freq_params[60] = (double) ffreqs3t.N_w;
        freq_params[61] = ffreqs3t.w_upper;
        freq_params[62] = ffreqs3t.w_lower;
        freq_params[63] = ffreqs3t.W_scale;

    convert_vec_to_type(bfreqs3a.get_ws_vec(), bfreqs3a_buffer);
    convert_vec_to_type(ffreqs3a.get_ws_vec(), ffreqs3a_buffer);
    convert_vec_to_type(bfreqs3p.get_ws_vec(), bfreqs3p_buffer);
    convert_vec_to_type(ffreqs3p.get_ws_vec(), ffreqs3p_buffer);
    convert_vec_to_type(bfreqs3t.get_ws_vec(), bfreqs3t_buffer);
    convert_vec_to_type(ffreqs3t.get_ws_vec(), ffreqs3t_buffer);
        //for (int i=0; i<nBOS3+FREQ_PADDING*2; ++i) {
        //    bfreqs3_buffer[i] = bfreqs3.ws[i];
        //}
        //for (int i=0; i<nFER3+FREQ_PADDING*2; ++i) {
        //    ffreqs3_buffer[i] = ffreqs3.get_ws[i];
        //}
        for(int i=0; i<K3_dim; ++i) {                                // write K3 into buffer
            K3_class_a[i].re = std::real(state_in.vertex[0].avertex().K3.acc(i));
            K3_class_a[i].im = std::imag(state_in.vertex[0].avertex().K3.acc(i));

            K3_class_p[i].re = std::real(state_in.vertex[0].pvertex().K3.acc(i));
            K3_class_p[i].im = std::imag(state_in.vertex[0].pvertex().K3.acc(i));

            K3_class_t[i].re = std::real(state_in.vertex[0].tvertex().K3.acc(i));
            K3_class_t[i].im = std::imag(state_in.vertex[0].tvertex().K3.acc(i));
        }
#endif
        //print("Buffer ready. Preparing for saving into Hdf5 file...", true);
    }
};

// Wrapper for static cast from int to hsize_t, used in class Dims
hsize_t h5_cast(int dim) {
    return static_cast<hsize_t>(dim);
}

/**
 * Class containing dimension arrays for data sets in file and for the buffers.
 * Constructor initializes them to fixed values specified via buffer lengths.
 */
class Dims {
public:
    hsize_t Lambda[1];
    hsize_t freq_params_dims[2];
    hsize_t freq_params_buffer_dims[2];
    hsize_t bfreqs_dims[2];
    hsize_t ffreqs_dims[2];
    hsize_t bfreqs_buffer_dims[1];
    hsize_t ffreqs_buffer_dims[1];
    hsize_t params[1];
    hsize_t selfenergy[2];
    hsize_t selfenergy_buffer[1];
    hsize_t irreducible[2];
    hsize_t irreducible_buffer[1];
#if MAX_DIAG_CLASS >= 1
    hsize_t K1[2];
    hsize_t K1_buffer[1];
#endif
#if MAX_DIAG_CLASS >= 2
    hsize_t bfreqs2_dims[2];
    hsize_t ffreqs2_dims[2];
    hsize_t bfreqs2_buffer_dims[1];
    hsize_t ffreqs2_buffer_dims[1];
    hsize_t K2[2];
    hsize_t K2_buffer[1];
#endif
#if MAX_DIAG_CLASS >= 3
    hsize_t bfreqs3_dims[2];
    hsize_t ffreqs3_dims[2];
    hsize_t bfreqs3_buffer_dims[1];
    hsize_t ffreqs3_buffer_dims[1];
    hsize_t K3[2];
    hsize_t K3_buffer[1];
#endif

    Dims (Buffer& buffer, long Lambda_size) :
#if MAX_DIAG_CLASS >= 1
        K1 {h5_cast(Lambda_size), h5_cast(buffer.K1_dim)},
        K1_buffer {h5_cast(buffer.K1_dim)},
#endif
#if MAX_DIAG_CLASS >= 2
        bfreqs2_dims {h5_cast(Lambda_size), h5_cast(nBOS2+FREQ_PADDING*2)},
        ffreqs2_dims {h5_cast(Lambda_size), h5_cast(nFER2+FREQ_PADDING*2)},
        bfreqs2_buffer_dims {h5_cast(nBOS2+FREQ_PADDING*2)},
        ffreqs2_buffer_dims {h5_cast(nFER2+FREQ_PADDING*2)},
        K2 {h5_cast(Lambda_size), h5_cast(buffer.K2_dim)},
        K2_buffer {h5_cast(buffer.K2_dim)},
#endif
#if MAX_DIAG_CLASS >= 3
        bfreqs3_dims {h5_cast(Lambda_size), h5_cast(nBOS3+FREQ_PADDING*2)},
        ffreqs3_dims {h5_cast(Lambda_size), h5_cast(nFER3+FREQ_PADDING*2)},
        bfreqs3_buffer_dims {h5_cast(nBOS3+FREQ_PADDING*2)},
        ffreqs3_buffer_dims {h5_cast(nFER3+FREQ_PADDING*2)},
        K3 {h5_cast(Lambda_size), h5_cast(buffer.K3_dim)},
        K3_buffer {h5_cast(buffer.K3_dim)},
#endif
        Lambda {h5_cast(Lambda_size)},
        freq_params_dims {h5_cast(Lambda_size), h5_cast(N_freq_params)},
        freq_params_buffer_dims {h5_cast(N_freq_params)},
        bfreqs_dims {h5_cast(Lambda_size), h5_cast(nBOS+FREQ_PADDING*2)},
        ffreqs_dims {h5_cast(Lambda_size), h5_cast(nFER+FREQ_PADDING*2)},
        bfreqs_buffer_dims {h5_cast(nBOS+FREQ_PADDING*2)},
        ffreqs_buffer_dims {h5_cast(nFER+FREQ_PADDING*2)},
        params {h5_cast(param_size)},
        selfenergy {h5_cast(Lambda_size), h5_cast(buffer.self_dim)},
        selfenergy_buffer {h5_cast(buffer.self_dim)},
        irreducible {h5_cast(Lambda_size), h5_cast(buffer.irred_dim)},
        irreducible_buffer {h5_cast(buffer.irred_dim)}
    {}
};

/**
 * Class containing HDF5 data sets for all objects that should be stored in the file.
 * There are two sets of members:
 *  - Pointers: when writing to a new file, new data sets are created, which require access via pointers.
 *  - Objects: when writing to an existing file, DataSet objects are read from the file, which are stored in this class.
 * There are also two constructors:
 *  - When writing file, new data sets need to be created if the file does not yet exist.
 *  - When reading file, existing data sets are opened from the file.
 */
class DataSets {
public:
    // Data sets for new file: Pointers
    H5::DataSet *lambda_p, *self_p, *irred_p;
    H5::DataSet *freq_params_p;
    H5::DataSet *bfreqsa_p, *bfreqsp_p, *bfreqst_p, *ffreqs_p, *params_p;
#if MAX_DIAG_CLASS >= 1
    H5::DataSet *K1_a_p, *K1_p_p, *K1_t_p;
#endif
#if MAX_DIAG_CLASS >= 2
    H5::DataSet *bfreqs2a_p, *bfreqs2p_p, *bfreqs2t_p, *ffreqs2a_p, *ffreqs2p_p, *ffreqs2t_p;
    H5::DataSet *K2_a_p, *K2_p_p, *K2_t_p;
#endif
#if MAX_DIAG_CLASS >= 3
    H5::DataSet *bfreqs3a_p, *bfreqs3p_p, *bfreqs3t_p, *ffreqs3a_p, *ffreqs3p_p, *ffreqs3t_p;
    H5::DataSet *K3_a_p, *K3_p_p, *K3_t_p;
#endif

    // Data sets from existing file: Objects
    H5::DataSet lambda, self, irred;
    H5::DataSet freq_params;
    H5::DataSet bfreqsa_dataset, bfreqsp_dataset, bfreqst_dataset, ffreqs_dataset;
#if MAX_DIAG_CLASS >= 1
    H5::DataSet K1_a, K1_p, K1_t;
#endif
#if MAX_DIAG_CLASS >= 2
    H5::DataSet bfreqs2a_dataset, bfreqs2p_dataset, bfreqs2t_dataset, ffreqs2a_dataset, ffreqs2p_dataset, ffreqs2t_dataset;
    H5::DataSet K2_a, K2_p, K2_t;
#endif
#if MAX_DIAG_CLASS >= 3
    H5::DataSet bfreqs3a_dataset, bfreqs3p_dataset, bfreqs3t_dataset, ffreqs3a_dataset, ffreqs3p_dataset, ffreqs3t_dataset;
    H5::DataSet K3_a, K3_p, K3_t;
#endif

    /**
     * Initialize the data sets for writing to file:
     *  - When writing to a new file, create new data sets.
     *  - When writing to an existing file, open existing data sets.
     * @param file            : File to which data sets are added / from which they are read.
     * @param file_exists     : To check if the file already existed.
     * @param dataSpaces_...  : Data spaces, needed when creating new data sets.
     * @param mtype_comp      : Data type of selfenergy and vertex (complex number).
     * @param plist_vert      : Initial value for vertex data sets.
     */
    DataSets(H5::H5File* file, bool file_exists,
             H5::DataSpace& dataSpaces_Lambda,
             H5::DataSpace& dataSpaces_selfenergy,
             H5::DataSpace& dataSpaces_irreducible,
             H5::DataSpace& dataSpaces_freq_params,
             H5::DataSpace& dataSpaces_bfreqsa,
             H5::DataSpace& dataSpaces_bfreqsp,
             H5::DataSpace& dataSpaces_bfreqst,
             H5::DataSpace& dataSpaces_ffreqs,
             H5::DataSpace& dataSpaces_params,
#if MAX_DIAG_CLASS >= 1
             H5::DataSpace& dataSpaces_K1_a,
             H5::DataSpace& dataSpaces_K1_p,
             H5::DataSpace& dataSpaces_K1_t,
#endif
#if MAX_DIAG_CLASS >= 2
             H5::DataSpace& dataSpaces_bfreqs2a,
             H5::DataSpace& dataSpaces_ffreqs2a,
             H5::DataSpace& dataSpaces_bfreqs2p,
             H5::DataSpace& dataSpaces_ffreqs2p,
             H5::DataSpace& dataSpaces_bfreqs2t,
             H5::DataSpace& dataSpaces_ffreqs2t,
             H5::DataSpace& dataSpaces_K2_a,
             H5::DataSpace& dataSpaces_K2_p,
             H5::DataSpace& dataSpaces_K2_t,
#endif
#if MAX_DIAG_CLASS >= 3
             H5::DataSpace& dataSpaces_bfreqs3a,
             H5::DataSpace& dataSpaces_ffreqs3a,
             H5::DataSpace& dataSpaces_bfreqs3p,
             H5::DataSpace& dataSpaces_ffreqs3p,
             H5::DataSpace& dataSpaces_bfreqs3t,
             H5::DataSpace& dataSpaces_ffreqs3t,
             H5::DataSpace& dataSpaces_K3_a,
             H5::DataSpace& dataSpaces_K3_p,
             H5::DataSpace& dataSpaces_K3_t,
#endif
             H5::CompType mtype_comp, H5::DSetCreatPropList plist_vert) {
        if (!file_exists) {
            lambda_p = new H5::DataSet(
                    file->createDataSet(LAMBDA_LIST, H5::PredType::NATIVE_DOUBLE, dataSpaces_Lambda));
            self_p = new H5::DataSet(file->createDataSet(SELF_LIST, mtype_comp, dataSpaces_selfenergy));
            irred_p = new H5::DataSet(
                    file->createDataSet(DATASET_irred, mtype_comp, dataSpaces_irreducible, plist_vert));
            freq_params_p = new H5::DataSet(
                    file->createDataSet(FREQ_PARAMS, H5::PredType::NATIVE_DOUBLE, dataSpaces_freq_params));
            bfreqsa_p = new H5::DataSet(
                    file->createDataSet(BFREQS_LISTa, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqsa));
            bfreqsp_p = new H5::DataSet(
                    file->createDataSet(BFREQS_LISTp, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqsp));
            bfreqst_p = new H5::DataSet(
                    file->createDataSet(BFREQS_LISTt, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqst));
            ffreqs_p = new H5::DataSet(
                    file->createDataSet(FFREQS_LIST, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs));
            params_p = new H5::DataSet(
                    file->createDataSet(PARAM_LIST, H5::PredType::NATIVE_DOUBLE, dataSpaces_params));
#if MAX_DIAG_CLASS >= 1
            // Create the datasets in file:
            K1_a_p = new H5::DataSet(
                    file->createDataSet(DATASET_K1_a, mtype_comp, dataSpaces_K1_a, plist_vert));
            K1_p_p = new H5::DataSet(
                    file->createDataSet(DATASET_K1_p, mtype_comp, dataSpaces_K1_p, plist_vert));
            K1_t_p = new H5::DataSet(
                    file->createDataSet(DATASET_K1_t, mtype_comp, dataSpaces_K1_t, plist_vert));
#endif
#if MAX_DIAG_CLASS >= 2
            // Create the datasets in file:
            bfreqs2a_p = new H5::DataSet(
                    file->createDataSet(BFREQS2_LISTa, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs2a));
            bfreqs2p_p = new H5::DataSet(
                    file->createDataSet(BFREQS2_LISTp, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs2p));
            bfreqs2t_p = new H5::DataSet(
                    file->createDataSet(BFREQS2_LISTt, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs2t));
            ffreqs2a_p = new H5::DataSet(
                    file->createDataSet(FFREQS2_LISTa, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs2a));
            ffreqs2p_p = new H5::DataSet(
                    file->createDataSet(FFREQS2_LISTp, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs2p));
            ffreqs2t_p = new H5::DataSet(
                    file->createDataSet(FFREQS2_LISTt, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs2t));
            K2_a_p = new H5::DataSet(
                    file->createDataSet(DATASET_K2_a, mtype_comp, dataSpaces_K2_a, plist_vert));
            K2_p_p = new H5::DataSet(
                    file->createDataSet(DATASET_K2_p, mtype_comp, dataSpaces_K2_p, plist_vert));
            K2_t_p = new H5::DataSet(
                    file->createDataSet(DATASET_K2_t, mtype_comp, dataSpaces_K2_t, plist_vert));
#endif
#if MAX_DIAG_CLASS >= 3
            // Create the datasets in file:
            bfreqs3a_p = new H5::DataSet(
                    file->createDataSet(BFREQS3_LISTa, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs3a));
            bfreqs3p_p = new H5::DataSet(
                    file->createDataSet(BFREQS3_LISTp, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs3p));
            bfreqs3t_p = new H5::DataSet(
                    file->createDataSet(BFREQS3_LISTt, H5::PredType::NATIVE_DOUBLE, dataSpaces_bfreqs3t));
            ffreqs3a_p = new H5::DataSet(
                    file->createDataSet(FFREQS3_LISTa, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs3a));
            ffreqs3p_p = new H5::DataSet(
                    file->createDataSet(FFREQS3_LISTp, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs3p));
            ffreqs3t_p = new H5::DataSet(
                    file->createDataSet(FFREQS3_LISTt, H5::PredType::NATIVE_DOUBLE, dataSpaces_ffreqs3t));
            K3_a_p = new H5::DataSet(
                    file->createDataSet(DATASET_K3_a, mtype_comp, dataSpaces_K3_a, plist_vert));
            K3_p_p = new H5::DataSet(
                    file->createDataSet(DATASET_K3_p, mtype_comp, dataSpaces_K3_p, plist_vert));
            K3_t_p = new H5::DataSet(
                    file->createDataSet(DATASET_K3_t, mtype_comp, dataSpaces_K3_t, plist_vert));
#endif
        }
        else {
            lambda = file->openDataSet("lambdas");
            self = file->openDataSet("selflist");
            irred = file->openDataSet("irred");
            freq_params = file->openDataSet("freq_params");
            bfreqsa_dataset = file->openDataSet("bfreqs_a");
            bfreqsp_dataset = file->openDataSet("bfreqs_p");
            bfreqst_dataset = file->openDataSet("bfreqs_t");
            ffreqs_dataset = file->openDataSet("ffreqs");
#if MAX_DIAG_CLASS >=1
            K1_a = file->openDataSet("K1_a");
            K1_p = file->openDataSet("K1_p");
            K1_t = file->openDataSet("K1_t");
#endif
#if MAX_DIAG_CLASS >=2
            bfreqs2a_dataset = file->openDataSet("bfreqs2_a");
            ffreqs2a_dataset = file->openDataSet("ffreqs2_a");
            bfreqs2p_dataset = file->openDataSet("bfreqs2_p");
            ffreqs2p_dataset = file->openDataSet("ffreqs2_p");
            bfreqs2t_dataset = file->openDataSet("bfreqs2_t");
            ffreqs2t_dataset = file->openDataSet("ffreqs2_t");
            K2_a = file->openDataSet("K2_a");
            K2_p = file->openDataSet("K2_p");
            K2_t = file->openDataSet("K2_t");
#endif
#if MAX_DIAG_CLASS >=3
            bfreqs3a_dataset = file->openDataSet("bfreqs3_a");
            ffreqs3a_dataset = file->openDataSet("ffreqs3_a");
            bfreqs3p_dataset = file->openDataSet("bfreqs3_p");
            ffreqs3p_dataset = file->openDataSet("ffreqs3_p");
            bfreqs3t_dataset = file->openDataSet("bfreqs3_t");
            ffreqs3t_dataset = file->openDataSet("ffreqs3_t");
            K3_a = file->openDataSet("K3_a");
            K3_p = file->openDataSet("K3_p");
            K3_t = file->openDataSet("K3_t");
#endif
        }
    }
    /**
     * Initialize the data sets when reading from file.
     * @param file : File from which data sets are loaded.
     */
    DataSets(H5::H5File* file) {
        lambda = file->openDataSet("lambdas");
        self = file->openDataSet("selflist");
        irred = file->openDataSet("irred");
        try {   // storing frequency gri parameters was implemented later --> old files do not have it
            freq_params = file->openDataSet("freq_params");
        }
        catch (H5::FileIException error) {
            error.printErrorStack();
        }
        bfreqsa_dataset = file->openDataSet("bfreqs_a");
        bfreqsp_dataset = file->openDataSet("bfreqs_p");
        bfreqst_dataset = file->openDataSet("bfreqs_t");
        ffreqs_dataset = file->openDataSet("ffreqs");
#if MAX_DIAG_CLASS >=1
        K1_a = file->openDataSet("K1_a");
        K1_p = file->openDataSet("K1_p");
        K1_t = file->openDataSet("K1_t");
#endif
#if MAX_DIAG_CLASS >=2
        bfreqs2a_dataset = file->openDataSet("bfreqs2_a");
        ffreqs2a_dataset = file->openDataSet("ffreqs2_a");
        bfreqs2p_dataset = file->openDataSet("bfreqs2_p");
        ffreqs2p_dataset = file->openDataSet("ffreqs2_p");
        bfreqs2t_dataset = file->openDataSet("bfreqs2_t");
        ffreqs2t_dataset = file->openDataSet("ffreqs2_t");
        K2_a = file->openDataSet("K2_a");
        K2_p = file->openDataSet("K2_p");
        K2_t = file->openDataSet("K2_t");
#endif
#if MAX_DIAG_CLASS >=3
        bfreqs3a_dataset = file->openDataSet("bfreqs3_a");
        ffreqs3a_dataset = file->openDataSet("ffreqs3_a");
        bfreqs3p_dataset = file->openDataSet("bfreqs3_p");
        ffreqs3p_dataset = file->openDataSet("ffreqs3_p");
        bfreqs3t_dataset = file->openDataSet("bfreqs3_t");
        ffreqs3t_dataset = file->openDataSet("ffreqs3_t");
        K3_a = file->openDataSet("K3_a");
        K3_p = file->openDataSet("K3_p");
        K3_t = file->openDataSet("K3_t");
#endif
    }
    /**
     * Close all data sets when writing/reading is finished.
     */
    void close(bool file_exists) {
        if (!file_exists) {
            lambda_p -> close();
            freq_params_p -> close();
            bfreqsa_p -> close();
            bfreqsp_p -> close();
            bfreqst_p -> close();
            ffreqs_p -> close();
            params_p -> close();
            irred_p  -> close();
            self_p   -> close();

#if MAX_DIAG_CLASS >= 1
            K1_a_p -> close();
            K1_p_p -> close();
            K1_t_p -> close();
#endif
#if MAX_DIAG_CLASS >= 2
            bfreqs2a_p -> close();
            ffreqs2a_p -> close();
            bfreqs2p_p -> close();
            ffreqs2p_p -> close();
            bfreqs2t_p -> close();
            ffreqs2t_p -> close();
            K2_a_p -> close();
            K2_p_p -> close();
            K2_t_p -> close();
#endif
#if MAX_DIAG_CLASS >= 3
            bfreqs3a_p -> close();
            ffreqs3a_p -> close();
            bfreqs3p_p -> close();
            ffreqs3p_p -> close();
            bfreqs3t_p -> close();
            ffreqs3t_p -> close();
            K3_a_p -> close();
            K3_p_p -> close();
            K3_t_p -> close();
#endif
        }
        else {
            self.close();
            irred.close();
            freq_params.close();
            bfreqsa_dataset.close();
            bfreqsp_dataset.close();
            bfreqst_dataset.close();
            ffreqs_dataset.close();

#if MAX_DIAG_CLASS >=1
            K1_a.close();
            K1_p.close();
            K1_t.close();
#endif
#if MAX_DIAG_CLASS >=2
            bfreqs2a_dataset.close();
            ffreqs2a_dataset.close();
            bfreqs2p_dataset.close();
            ffreqs2p_dataset.close();
            bfreqs2t_dataset.close();
            ffreqs2t_dataset.close();
            K2_a.close();
            K2_p.close();
            K2_t.close();
#endif
#if MAX_DIAG_CLASS >=3
            bfreqs3a_dataset.close();
            ffreqs3a_dataset.close();
            bfreqs3p_dataset.close();
            ffreqs3p_dataset.close();
            bfreqs3t_dataset.close();
            ffreqs3t_dataset.close();
            K3_a.close();
            K3_p.close();
            K3_t.close();
#endif
        }
    }
};


/// --- Functions for reading data from file --- ///

/**
 * Read Lambdas from an existing file
 * @param FILE_NAME   : File name
 * @return            : Lambdas in a vector of doubles
 */
rvec read_Lambdas_from_hdf(const H5std_string FILE_NAME){



    // Open the file. Access rights: read-only
    H5::H5File *file = 0;
    file = new H5::H5File(FILE_NAME, H5F_ACC_RDONLY);

    H5::DataSet lambda_dataset = file->openDataSet("lambdas");

    // Prepare a buffer to which data from file is written. Buffer is copied to result eventually.
    //Buffer buffer;

    // Create the memory data type for storing complex numbers in file
    //H5::CompType mtype_comp = def_mtype_comp();

    // Create the dimension arrays for objects in file and in buffer
    //Dims dims(buffer, Lambda_size);

    // Read the data sets from the file to copy their content into the buffer
    //DataSets dataSets(file);

    // Create the data spaces for the data sets in file and for buffer objects
    H5::DataSpace dataSpace_Lambda = lambda_dataset.getSpace();
    /*
     * Get the number of dimensions in the dataspace.
     */
    int rank = dataSpace_Lambda.getSimpleExtentNdims();
    /*
     * Get the dimension size of each dimension in the dataspace and
     * display them.
     */
    hsize_t dims_out[2];
    int ndims = dataSpace_Lambda.getSimpleExtentDims( dims_out, NULL);
    //std::cout << "rank " << rank << ", dimensions " <<
    //     (unsigned long)(dims_out[0]) << " x " <<
    //     (unsigned long)(dims_out[1]) << std::endl;


    H5::DataSpace dataSpace_Lambda_buffer(1, dims_out);

    /// load data into buffer

    //hsize_t start_1D[1] = {Lambda_it};
    //hsize_t stride_1D[1]= {1};
    //hsize_t count_1D[1] = {1};
    //hsize_t block_1D[1] = {1};
    //dataSpaces_Lambda.selectHyperslab(H5S_SELECT_SET, count_1D, start_1D, stride_1D, block_1D);
    double Lambdas_arr[dims_out[0]];
    lambda_dataset.read(Lambdas_arr, H5::PredType::NATIVE_DOUBLE,
                        dataSpace_Lambda_buffer, dataSpace_Lambda);

    rvec Lambdas (Lambdas_arr, Lambdas_arr + sizeof(Lambdas_arr) / sizeof(double));

    // Terminate
    lambda_dataset.close();
    file->close();
    delete file;

    return Lambdas;


}

/**
 * Initialize the frequency grids of the result using the parameters stored in buffer.
 * @param result : Empty State object into which result is copied.
 * @param buffer : Buffer from which result is read. Should contain data read from a file.
 */
template <typename Q>
void result_set_frequency_grids(State<Q>& result, Buffer& buffer) {
    // create new frequency grids
    FrequencyGrid bfreqsa ('b', 1, Lambda_ini);
    FrequencyGrid bfreqsp ('b', 1, Lambda_ini);
    FrequencyGrid bfreqst ('b', 1, Lambda_ini);
    FrequencyGrid ffreqs ('f', 1, Lambda_ini);
    // read grid parameters from buffer
    bfreqsa.N_w = (int)buffer.freq_params[0];
    bfreqsa.w_upper = buffer.freq_params[1];
    bfreqsa.w_lower = buffer.freq_params[2];
    bfreqsa.W_scale = buffer.freq_params[3];
    ffreqs.N_w = (int)buffer.freq_params[4];
    ffreqs.w_upper = buffer.freq_params[5];
    ffreqs.w_lower = buffer.freq_params[6];
    ffreqs.W_scale = buffer.freq_params[7];
    bfreqsp.N_w = (int)buffer.freq_params[24];
    bfreqsp.w_upper = buffer.freq_params[25];
    bfreqsp.w_lower = buffer.freq_params[26];
    bfreqsp.W_scale = buffer.freq_params[27];
    bfreqst.N_w = (int)buffer.freq_params[28];
    bfreqst.w_upper = buffer.freq_params[29];
    bfreqst.w_lower = buffer.freq_params[30];
    bfreqst.W_scale = buffer.freq_params[31];
    // initialize grids
    bfreqsa.initialize_grid();
    bfreqsp.initialize_grid();
    bfreqst.initialize_grid();
    ffreqs.initialize_grid();
    // copy grids to result
    result.selfenergy.frequencies = ffreqs;
    result.vertex[0].avertex().K1.frequencies_K1.b = bfreqsa;
    result.vertex[0].pvertex().K1.frequencies_K1.b = bfreqsp;
    result.vertex[0].tvertex().K1.frequencies_K1.b = bfreqst;
#if MAX_DIAG_CLASS >= 2
    FrequencyGrid bfreqs2a ('b', 2, Lambda_ini);
    FrequencyGrid ffreqs2a ('f', 2, Lambda_ini);
    FrequencyGrid bfreqs2p ('b', 2, Lambda_ini);
    FrequencyGrid ffreqs2p ('f', 2, Lambda_ini);
    FrequencyGrid bfreqs2t ('b', 2, Lambda_ini);
    FrequencyGrid ffreqs2t ('f', 2, Lambda_ini);
    bfreqs2a.N_w = (int)buffer.freq_params[8];
    bfreqs2a.w_upper = buffer.freq_params[9];
    bfreqs2a.w_lower = buffer.freq_params[10];
    bfreqs2a.W_scale = buffer.freq_params[11];
    ffreqs2a.N_w = (int)buffer.freq_params[12];
    ffreqs2a.w_upper = buffer.freq_params[13];
    ffreqs2a.w_lower = buffer.freq_params[14];
    ffreqs2a.W_scale = buffer.freq_params[15];
    bfreqs2p.N_w = (int)buffer.freq_params[32];
    bfreqs2p.w_upper = buffer.freq_params[33];
    bfreqs2p.w_lower = buffer.freq_params[34];
    bfreqs2p.W_scale = buffer.freq_params[35];
    ffreqs2p.N_w = (int)buffer.freq_params[36];
    ffreqs2p.w_upper = buffer.freq_params[37];
    ffreqs2p.w_lower = buffer.freq_params[38];
    ffreqs2p.W_scale = buffer.freq_params[39];
    bfreqs2t.N_w = (int)buffer.freq_params[40];
    bfreqs2t.w_upper = buffer.freq_params[41];
    bfreqs2t.w_lower = buffer.freq_params[42];
    bfreqs2t.W_scale = buffer.freq_params[43];
    ffreqs2t.N_w = (int)buffer.freq_params[44];
    ffreqs2t.w_upper = buffer.freq_params[45];
    ffreqs2t.w_lower = buffer.freq_params[46];
    ffreqs2t.W_scale = buffer.freq_params[47];
    bfreqs2a.initialize_grid();
    ffreqs2a.initialize_grid();
    bfreqs2p.initialize_grid();
    ffreqs2p.initialize_grid();
    bfreqs2t.initialize_grid();
    ffreqs2t.initialize_grid();
    result.vertex[0].avertex().K2.frequencies_K2.b = bfreqs2a;
    result.vertex[0].pvertex().K2.frequencies_K2.b = bfreqs2a;
    result.vertex[0].tvertex().K2.frequencies_K2.b = bfreqs2p;
    result.vertex[0].avertex().K2.frequencies_K2.f = ffreqs2p;
    result.vertex[0].pvertex().K2.frequencies_K2.f = ffreqs2t;
    result.vertex[0].tvertex().K2.frequencies_K2.f = ffreqs2t;
#endif
#if MAX_DIAG_CLASS >= 3
    FrequencyGrid bfreqs3a ('b', 3, Lambda_ini);
    FrequencyGrid ffreqs3a ('f', 3, Lambda_ini);
    FrequencyGrid bfreqs3p ('b', 3, Lambda_ini);
    FrequencyGrid ffreqs3p ('f', 3, Lambda_ini);
    FrequencyGrid bfreqs3t ('b', 3, Lambda_ini);
    FrequencyGrid ffreqs3t ('f', 3, Lambda_ini);
    bfreqs3a.N_w = (int)buffer.freq_params[16];
    bfreqs3a.w_upper = buffer.freq_params[17];
    bfreqs3a.w_lower = buffer.freq_params[18];
    bfreqs3a.W_scale = buffer.freq_params[19];
    ffreqs3a.N_w = (int)buffer.freq_params[20];
    ffreqs3a.w_upper = buffer.freq_params[21];
    ffreqs3a.w_lower = buffer.freq_params[22];
    ffreqs3a.W_scale = buffer.freq_params[23];
    bfreqs3p.N_w = (int)buffer.freq_params[48];
    bfreqs3p.w_upper = buffer.freq_params[49];
    bfreqs3p.w_lower = buffer.freq_params[50];
    bfreqs3p.W_scale = buffer.freq_params[51];
    ffreqs3p.N_w = (int)buffer.freq_params[52];
    ffreqs3p.w_upper = buffer.freq_params[53];
    ffreqs3p.w_lower = buffer.freq_params[54];
    ffreqs3p.W_scale = buffer.freq_params[55];
    bfreqs3t.N_w = (int)buffer.freq_params[56];
    bfreqs3t.w_upper = buffer.freq_params[57];
    bfreqs3t.w_lower = buffer.freq_params[58];
    bfreqs3t.W_scale = buffer.freq_params[59];
    ffreqs3t.N_w = (int)buffer.freq_params[60];
    ffreqs3t.w_upper = buffer.freq_params[61];
    ffreqs3t.w_lower = buffer.freq_params[62];
    ffreqs3t.W_scale = buffer.freq_params[63];
    bfreqs3a.initialize_grid();
    ffreqs3a.initialize_grid();
    bfreqs3p.initialize_grid();
    ffreqs3p.initialize_grid();
    bfreqs3t.initialize_grid();
    ffreqs3t.initialize_grid();
    result.vertex[0].avertex().K3.frequencies_K3.b = bfreqs3a;
    result.vertex[0].pvertex().K3.frequencies_K3.b = bfreqs3a;
    result.vertex[0].tvertex().K3.frequencies_K3.b = bfreqs3p;
    result.vertex[0].avertex().K3.frequencies_K3.f = ffreqs3p;
    result.vertex[0].pvertex().K3.frequencies_K3.f = ffreqs3t;
    result.vertex[0].tvertex().K3.frequencies_K3.f = ffreqs3t;
#endif
}

/**
 * Copy results that are read from a file to a buffer into a State object.
 * @param result : Empty State object into which result is copied.
 * @param buffer : Buffer from which result is read. Should contain data read from a file.
 */
template <typename Q>
void copy_buffer_to_result(State<Q>& result, Buffer& buffer) {
    Q val; // buffer value

    result.Lambda = *buffer.lambda;

    for (int i=0; i<buffer.self_dim; ++i) {
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.selfenergy[i].re, buffer.selfenergy[i].im};
#else
        val = buffer.selfenergy[i].im;
#endif
        result.selfenergy.direct_set(i, val);
    }
    for (int i=0; i<buffer.irred_dim; ++i) {
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.irreducible_class[i].re, buffer.irreducible_class[i].im};
#else
        val = buffer.irreducible_class[i].re;
#endif
        result.vertex[0].irred().direct_set(i, val);
    }
#if MAX_DIAG_CLASS >= 1
    for (int i=0; i<buffer.K1_dim; ++i) {
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K1_class_a[i].re, buffer.K1_class_a[i].im};
#else
        val = buffer.K1_class_a[i].re;
#endif
        result.vertex[0].avertex().K1.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K1_class_p[i].re, buffer.K1_class_p[i].im};
#else
        val = buffer.K1_class_p[i].re;
#endif
        result.vertex[0].pvertex().K1.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K1_class_t[i].re, buffer.K1_class_t[i].im};
#else
        val = buffer.K1_class_t[i].re;
#endif
        result.vertex[0].tvertex().K1.direct_set(i, val);
    }
#endif
#if MAX_DIAG_CLASS >= 2
    for (int i=0; i<buffer.K2_dim; ++i) {
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K2_class_a[i].re, buffer.K2_class_a[i].im};
#else
        val = buffer.K2_class_a[i].re;
#endif
        result.vertex[0].avertex().K2.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K2_class_p[i].re, buffer.K2_class_p[i].im};
#else
        val = buffer.K2_class_p[i].re;
#endif
        result.vertex[0].pvertex().K2.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K2_class_t[i].re, buffer.K2_class_t[i].im};
#else
        val = buffer.K2_class_t[i].re;
#endif
        result.vertex[0].tvertex().K2.direct_set(i, val);
    }
#endif
#if MAX_DIAG_CLASS >= 3
    for (int i=0; i<buffer.K3_dim; ++i) {
#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K3_class_a[i].re, buffer.K3_class_a[i].im};
#else
        val = buffer.K3_class_a[i].re;
#endif
        result.vertex[0].avertex().K3.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K3_class_p[i].re, buffer.K3_class_p[i].im};
#else
        val = buffer.K3_class_p[i].re;
#endif
        result.vertex[0].pvertex().K3.direct_set(i, val);

#if defined(KELDYSH_FORMALISM) or not defined(PARTICLE_HOLE_SYMM)
        val = {buffer.K3_class_t[i].re, buffer.K3_class_t[i].im};
#else
        val = buffer.K3_class_t[i].re;
#endif
        result.vertex[0].tvertex().K3.direct_set(i, val);
    }
#endif
}

/**
 * Read results from an existing file to a State object. Useful when resuming a computation (checkpointing).
 * @param FILE_NAME   : File name.
 * @param Lambda_it   : Lambda iteration from which to load result.
 * @param Lambda_size : Total number of Lambda iterations saved in the file.
 * @param Lambdas     : Vector containing all Lambda values for which results can be saved in file.
 * @return            : State object containing the result.
 */
State<state_datatype> read_hdf(const H5std_string FILE_NAME, size_t Lambda_it){
    State<state_datatype> result(Lambda_ini);   // Initialize with ANY frequency grid, read grid from HDF file later
    long Lambda_size = read_Lambdas_from_hdf(FILE_NAME).size();
    if (Lambda_it < Lambda_size) {

        // Open the file. Access rights: read-only
        H5::H5File *file = 0;
        file = new H5::H5File(FILE_NAME, H5F_ACC_RDONLY);

        // Prepare a buffer to which data from file is written. Buffer is copied to result eventually.
        Buffer buffer;

        // Create the memory data type for storing complex numbers in file
        H5::CompType mtype_comp = def_mtype_comp();

        // Create the dimension arrays for objects in file and in buffer
        Dims dims(buffer, Lambda_size);

        // Read the data sets from the file to copy their content into the buffer
        DataSets dataSets(file);

        // Create the data spaces for the data sets in file and for buffer objects
        H5::DataSpace dataSpaces_Lambda = dataSets.lambda.getSpace();
        H5::DataSpace dataSpaces_freq_params;
        try {   // storing frequency gri parameters was implemented later --> old files do not have it
            dataSpaces_freq_params = dataSets.freq_params.getSpace();
        }
        catch (H5::DataSetIException error) {
            error.printErrorStack();
        }
        H5::DataSpace dataSpaces_selfenergy = dataSets.self.getSpace();
        H5::DataSpace dataSpaces_irreducible = dataSets.irred.getSpace();

        H5::DataSpace dataSpaces_Lambda_buffer(1, dims.Lambda);
        H5::DataSpace dataSpaces_freq_params_buffer(RANK_freqs-1, dims.freq_params_buffer_dims);
        H5::DataSpace dataSpaces_selfenergy_buffer(RANK_self-1, dims.selfenergy_buffer);
        H5::DataSpace dataSpaces_irreducible_buffer(RANK_irreducible-1, dims.irreducible_buffer);

#if MAX_DIAG_CLASS >= 1
        H5::DataSpace dataSpaces_K1_a = dataSets.K1_a.getSpace();
        H5::DataSpace dataSpaces_K1_p = dataSets.K1_p.getSpace();
        H5::DataSpace dataSpaces_K1_t = dataSets.K1_t.getSpace();

        H5::DataSpace dataSpaces_K1_a_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_p_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_t_buffer(RANK_K1-1, dims.K1_buffer);
#endif

#if MAX_DIAG_CLASS >= 2
        H5::DataSpace dataSpaces_K2_a = dataSets.K2_a.getSpace();
        H5::DataSpace dataSpaces_K2_p = dataSets.K2_p.getSpace();
        H5::DataSpace dataSpaces_K2_t = dataSets.K2_t.getSpace();

        H5::DataSpace dataSpaces_K2_a_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_p_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_t_buffer(RANK_K2-1, dims.K2_buffer);
#endif

#if MAX_DIAG_CLASS >= 3
        H5::DataSpace dataSpaces_K3_a = dataSets.K3_a.getSpace();
        H5::DataSpace dataSpaces_K3_p = dataSets.K3_p.getSpace();
        H5::DataSpace dataSpaces_K3_t = dataSets.K3_t.getSpace();

        H5::DataSpace dataSpaces_K3_a_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_p_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_t_buffer(RANK_K3-1, dims.K3_buffer);
#endif


        //Select hyperslab in the file where the data should be located
        hsize_t start[2];
        hsize_t stride[2];
        hsize_t count[2];
        hsize_t block[2];

        start[0] = Lambda_it;
        start[1] = 0;
        for (int i = 0; i < 2; i++) {
            stride[i] = 1;
            block[i] = 1;
        }
        count[0] = 1;

        /// load data into buffer

        //hsize_t start_1D[1] = {Lambda_it};
        //hsize_t stride_1D[1]= {1};
        //hsize_t count_1D[1] = {1};
        //hsize_t block_1D[1] = {1};
        //dataSpaces_Lambda.selectHyperslab(H5S_SELECT_SET, count_1D, start_1D, stride_1D, block_1D);
        //dataSets.lambda.read(buffer.lambda, H5::PredType::NATIVE_DOUBLE,
        //                   dataSpaces_Lambda_buffer, dataSpaces_Lambda);

        int rank = dataSpaces_Lambda.getSimpleExtentNdims();
        /*
         * Get the dimension size of each dimension in the dataspace and
         * display them.
         */
        hsize_t dims_out[2];
        int ndims = dataSpaces_Lambda.getSimpleExtentDims( dims_out, NULL);
        //std::cout << "rank " << rank << ", dimensions " <<
        //     (unsigned long)(dims_out[0]) << " x " <<
        //     (unsigned long)(dims_out[1]) << std::endl;


        H5::DataSpace dataSpace_Lambda_buffer(1, dims_out);

        /// load data into buffer
        double Lambdas_arr[dims_out[0]];
        dataSets.lambda.read(Lambdas_arr, H5::PredType::NATIVE_DOUBLE,
                             dataSpace_Lambda_buffer, dataSpaces_Lambda);
        *buffer.lambda = Lambdas_arr[Lambda_it];


        count[1] = N_freq_params;
        try {   // storing frequency gri parameters was implemented later --> old files do not have it
            dataSpaces_freq_params.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
            dataSets.freq_params.read(buffer.freq_params, H5::PredType::NATIVE_DOUBLE,
                                      dataSpaces_freq_params_buffer, dataSpaces_freq_params);
        }
        catch (H5::DataSpaceIException error) {
            error.printErrorStack();
        }

        count[1] = buffer.self_dim;
        dataSpaces_selfenergy.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSets.self.read(buffer.selfenergy, mtype_comp,
                           dataSpaces_selfenergy_buffer, dataSpaces_selfenergy);

        count[1] = buffer.irred_dim;
        dataSpaces_irreducible.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSets.irred.read(buffer.irreducible_class, mtype_comp,
                            dataSpaces_irreducible_buffer, dataSpaces_irreducible);


#if MAX_DIAG_CLASS >= 1
        count[1] = buffer.K1_dim;
        dataSpaces_K1_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K1_a.read(buffer.K1_class_a, mtype_comp, dataSpaces_K1_a_buffer, dataSpaces_K1_a);
        dataSets.K1_p.read(buffer.K1_class_p, mtype_comp, dataSpaces_K1_p_buffer, dataSpaces_K1_p);
        dataSets.K1_t.read(buffer.K1_class_t, mtype_comp, dataSpaces_K1_t_buffer, dataSpaces_K1_t);
#endif
#if MAX_DIAG_CLASS >= 2
        count[1] = buffer.K2_dim;
        dataSpaces_K2_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K2_a.read(buffer.K2_class_a, mtype_comp, dataSpaces_K2_a_buffer, dataSpaces_K2_a);
        dataSets.K2_p.read(buffer.K2_class_p, mtype_comp, dataSpaces_K2_p_buffer, dataSpaces_K2_p);
        dataSets.K2_t.read(buffer.K2_class_t, mtype_comp, dataSpaces_K2_t_buffer, dataSpaces_K2_t);
#endif
#if MAX_DIAG_CLASS >= 3
        count[1] = buffer.K3_dim;
        dataSpaces_K3_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K3_a.read(buffer.K3_class_a, mtype_comp, dataSpaces_K3_a_buffer, dataSpaces_K3_a);
        dataSets.K3_p.read(buffer.K3_class_p, mtype_comp, dataSpaces_K3_p_buffer, dataSpaces_K3_p);
        dataSets.K3_t.read(buffer.K3_class_t, mtype_comp, dataSpaces_K3_t_buffer, dataSpaces_K3_t);

#endif

        // Initialize the frequency grids of the result State using the parameters stored in buffer
        result_set_frequency_grids(result, buffer);

        // Copy the buffered result into State object
        copy_buffer_to_result(result, buffer);

        // Terminate
        dataSets.close(true);
        file->close();
        delete file;

        return result;

    } else {
        throw std::runtime_error("Cannot read from file " + FILE_NAME + " since Lambda layer out of range");
    }
}




/// --- Functions for writing data to file --- ///

/**
 * Save data in State object to file.
 * @param FILE_NAME    : File name.
 * @param Lambda_it    : Lambda iteration at which to save the data.
 * @param Lambda_size  : Total number of Lambda iterations saved in the file.
 * @param state_in     : State to be written to the file.
 * @param Lambdas      : Vector containing all Lambda values for which results can be saved in file.
 * @param file_exists  : To check if the file already exists:
 *                        - If not, create new file.
 *                        - If yes, write data to existing file at iteration Lambda_it.
 */
template <typename  Q>
void save_to_hdf(const H5std_string FILE_NAME, int Lambda_it, long Lambda_size,
                 const State<Q>& state_in, rvec& Lambdas, bool file_exists) {
    //Try block to detect exceptions raised by any of the calls inside it
    try {
        // Prepare a buffer for writing data into the file
        Buffer buffer;
        buffer.initialize(state_in);    // copy data from state_in into the buffer

        // Turn off the auto-printing when failure occurs so that we can handle the errors appropriately
        H5::Exception::dontPrint();

        H5::H5File* file = 0;
        if (!file_exists) {
            // If file doesn't exist, create a new file using the default property lists.
            file = new H5::H5File(FILE_NAME, H5F_ACC_TRUNC);
        }
        else {
            // If file exists, open existing file. Access rights: read/write
            file = new H5::H5File(FILE_NAME, H5F_ACC_RDWR);
        }

        // Create the memory data type for storing complex numbers in file
        H5::CompType mtype_comp = def_mtype_comp();

        // Create the dimension arrays for objects in file and in buffer
        Dims dims(buffer, Lambda_size);

        // Create the data spaces for the data sets in file and for buffer objects
        H5::DataSpace dataSpaces_Lambda(1, dims.Lambda);
        H5::DataSpace dataSpaces_freq_params(RANK_freqs, dims.freq_params_dims);
        H5::DataSpace dataSpaces_bfreqsa(RANK_freqs, dims.bfreqs_dims);
        H5::DataSpace dataSpaces_bfreqsp(RANK_freqs, dims.bfreqs_dims);
        H5::DataSpace dataSpaces_bfreqst(RANK_freqs, dims.bfreqs_dims);
        H5::DataSpace dataSpaces_ffreqs(RANK_freqs, dims.ffreqs_dims);
        H5::DataSpace dataSpaces_params(1, dims.params);
        H5::DataSpace dataSpaces_selfenergy(RANK_self, dims.selfenergy);
        H5::DataSpace dataSpaces_irreducible(RANK_irreducible, dims.irreducible);

        H5::DataSpace dataSpaces_freq_params_buffer(RANK_freqs-1, dims.freq_params_buffer_dims);
        H5::DataSpace dataSpaces_bfreqsa_buffer(RANK_freqs-1, dims.bfreqs_buffer_dims);
        H5::DataSpace dataSpaces_bfreqsp_buffer(RANK_freqs-1, dims.bfreqs_buffer_dims);
        H5::DataSpace dataSpaces_bfreqst_buffer(RANK_freqs-1, dims.bfreqs_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs_buffer(RANK_freqs-1, dims.ffreqs_buffer_dims);
        H5::DataSpace dataSpaces_selfenergy_buffer(RANK_self-1, dims.selfenergy_buffer);
        H5::DataSpace dataSpaces_irreducible_buffer(RANK_irreducible-1, dims.irreducible_buffer);

#if MAX_DIAG_CLASS >= 1
        H5::DataSpace dataSpaces_K1_a(RANK_K1, dims.K1);
        H5::DataSpace dataSpaces_K1_p(RANK_K1, dims.K1);
        H5::DataSpace dataSpaces_K1_t(RANK_K1, dims.K1);

        H5::DataSpace dataSpaces_K1_a_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_p_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_t_buffer(RANK_K1-1, dims.K1_buffer);
#endif
#if MAX_DIAG_CLASS >= 2
        H5::DataSpace dataSpaces_bfreqs2a(RANK_freqs, dims.bfreqs2_dims);
        H5::DataSpace dataSpaces_ffreqs2a(RANK_freqs, dims.ffreqs2_dims);
        H5::DataSpace dataSpaces_bfreqs2p(RANK_freqs, dims.bfreqs2_dims);
        H5::DataSpace dataSpaces_ffreqs2p(RANK_freqs, dims.ffreqs2_dims);
        H5::DataSpace dataSpaces_bfreqs2t(RANK_freqs, dims.bfreqs2_dims);
        H5::DataSpace dataSpaces_ffreqs2t(RANK_freqs, dims.ffreqs2_dims);

        H5::DataSpace dataSpaces_bfreqs2a_buffer(RANK_freqs-1, dims.bfreqs2_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs2a_buffer(RANK_freqs-1, dims.ffreqs2_buffer_dims);
        H5::DataSpace dataSpaces_bfreqs2p_buffer(RANK_freqs-1, dims.bfreqs2_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs2p_buffer(RANK_freqs-1, dims.ffreqs2_buffer_dims);
        H5::DataSpace dataSpaces_bfreqs2t_buffer(RANK_freqs-1, dims.bfreqs2_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs2t_buffer(RANK_freqs-1, dims.ffreqs2_buffer_dims);

        H5::DataSpace dataSpaces_K2_a(RANK_K2, dims.K2);
        H5::DataSpace dataSpaces_K2_p(RANK_K2, dims.K2);
        H5::DataSpace dataSpaces_K2_t(RANK_K2, dims.K2);

        H5::DataSpace dataSpaces_K2_a_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_p_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_t_buffer(RANK_K2-1, dims.K2_buffer);
#endif
#if MAX_DIAG_CLASS >= 3
        H5::DataSpace dataSpaces_bfreqs3a(RANK_freqs, dims.bfreqs3_dims);
        H5::DataSpace dataSpaces_ffreqs3a(RANK_freqs, dims.ffreqs3_dims);
        H5::DataSpace dataSpaces_bfreqs3p(RANK_freqs, dims.bfreqs3_dims);
        H5::DataSpace dataSpaces_ffreqs3p(RANK_freqs, dims.ffreqs3_dims);
        H5::DataSpace dataSpaces_bfreqs3t(RANK_freqs, dims.bfreqs3_dims);
        H5::DataSpace dataSpaces_ffreqs3t(RANK_freqs, dims.ffreqs3_dims);

        H5::DataSpace dataSpaces_bfreqs3a_buffer(RANK_freqs-1, dims.bfreqs3_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs3a_buffer(RANK_freqs-1, dims.ffreqs3_buffer_dims);
        H5::DataSpace dataSpaces_bfreqs3p_buffer(RANK_freqs-1, dims.bfreqs3_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs3p_buffer(RANK_freqs-1, dims.ffreqs3_buffer_dims);
        H5::DataSpace dataSpaces_bfreqs3t_buffer(RANK_freqs-1, dims.bfreqs3_buffer_dims);
        H5::DataSpace dataSpaces_ffreqs3t_buffer(RANK_freqs-1, dims.ffreqs3_buffer_dims);

        H5::DataSpace dataSpaces_K3_a(RANK_K3, dims.K3);
        H5::DataSpace dataSpaces_K3_p(RANK_K3, dims.K3);
        H5::DataSpace dataSpaces_K3_t(RANK_K3, dims.K3);

        H5::DataSpace dataSpaces_K3_a_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_p_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_t_buffer(RANK_K3-1, dims.K3_buffer);
#endif

        // Initial value for vertex data sets // TODO(low): remove?
        h5_comp fillvalue_vert;
        fillvalue_vert.re = 0;
        fillvalue_vert.im = 0;
        H5::DSetCreatPropList plist_vert;
        plist_vert.setFillValue(mtype_comp, &fillvalue_vert);

        // Create the data sets for all data to be saved
        DataSets dataSets(file, file_exists,
                          dataSpaces_Lambda, dataSpaces_selfenergy, dataSpaces_irreducible,
                          dataSpaces_freq_params,
                          dataSpaces_bfreqsp, dataSpaces_bfreqsp, dataSpaces_bfreqst, dataSpaces_ffreqs, dataSpaces_params,
#if MAX_DIAG_CLASS >= 1
                          dataSpaces_K1_a, dataSpaces_K1_p, dataSpaces_K1_t,
#endif
#if MAX_DIAG_CLASS >= 2
                          dataSpaces_bfreqs2a, dataSpaces_ffreqs2a,
                          dataSpaces_bfreqs2p, dataSpaces_ffreqs2p,
                          dataSpaces_bfreqs2t, dataSpaces_ffreqs2t,
                          dataSpaces_K2_a, dataSpaces_K2_p, dataSpaces_K2_t,
#endif
#if MAX_DIAG_CLASS >= 3
                          dataSpaces_bfreqs3a, dataSpaces_ffreqs3a,
                          dataSpaces_bfreqs3p, dataSpaces_ffreqs3p,
                          dataSpaces_bfreqs3t, dataSpaces_ffreqs3t,
                          dataSpaces_K3_a, dataSpaces_K3_p, dataSpaces_K3_t,
#endif
                          mtype_comp, plist_vert);

        //Select hyperslab in the file where the data should be located and after that write buffered data into file.
        hsize_t start[2];
        hsize_t stride[2];
        hsize_t count[2];
        hsize_t block[2];

        start[0] = Lambda_it;
        start[1] = 0;
        for (int i = 0; i < 2; i++) {
            stride[i] = 1;
            block[i] = 1;
        }
        count[0] = 1;

        if (!file_exists) {
            dataSets.lambda_p -> write(Lambdas.data(), H5::PredType::NATIVE_DOUBLE);
            dataSets.params_p -> write(parameter_list, H5::PredType::NATIVE_DOUBLE);
        }
        else
            // overwrite vector containing all values for lambda
            dataSets.lambda.write(Lambdas.data(), H5::PredType::NATIVE_DOUBLE);

        count[1] = N_freq_params;
        dataSpaces_freq_params.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.freq_params_p -> write(buffer.freq_params, H5::PredType::NATIVE_DOUBLE,
                                            dataSpaces_freq_params_buffer, dataSpaces_freq_params);
        else
            dataSets.freq_params.write(buffer.freq_params, H5::PredType::NATIVE_DOUBLE,
                                       dataSpaces_freq_params_buffer, dataSpaces_freq_params);


        count[1] = nBOS+FREQ_PADDING*2;
        dataSpaces_bfreqsa.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqsa_p -> write(buffer.bfreqsa_buffer, H5::PredType::NATIVE_DOUBLE,
                                       dataSpaces_bfreqsa_buffer, dataSpaces_bfreqsa);
        else
            dataSets.bfreqsa_dataset.write(buffer.bfreqsa_buffer, H5::PredType::NATIVE_DOUBLE,
                                          dataSpaces_bfreqsa_buffer, dataSpaces_bfreqsa);

        dataSpaces_bfreqsp.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqsp_p -> write(buffer.bfreqsp_buffer, H5::PredType::NATIVE_DOUBLE,
                                       dataSpaces_bfreqsp_buffer, dataSpaces_bfreqsp);
        else
            dataSets.bfreqsp_dataset.write(buffer.bfreqsp_buffer, H5::PredType::NATIVE_DOUBLE,
                                          dataSpaces_bfreqsp_buffer, dataSpaces_bfreqsp);

        dataSpaces_bfreqst.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqst_p -> write(buffer.bfreqst_buffer, H5::PredType::NATIVE_DOUBLE,
                                       dataSpaces_bfreqst_buffer, dataSpaces_bfreqst);
        else
            dataSets.bfreqst_dataset.write(buffer.bfreqst_buffer, H5::PredType::NATIVE_DOUBLE,
                                          dataSpaces_bfreqst_buffer, dataSpaces_bfreqst);


        count[1] = nFER+FREQ_PADDING*2;
        dataSpaces_ffreqs.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs_p -> write(buffer.ffreqs_buffer, H5::PredType::NATIVE_DOUBLE,
                                       dataSpaces_ffreqs_buffer, dataSpaces_ffreqs);
        else
            dataSets.ffreqs_dataset.write(buffer.ffreqs_buffer, H5::PredType::NATIVE_DOUBLE,
                                          dataSpaces_ffreqs_buffer, dataSpaces_ffreqs);

        count[1] = buffer.self_dim;
        dataSpaces_selfenergy.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.self_p -> write(buffer.selfenergy, mtype_comp,
                                     dataSpaces_selfenergy_buffer, dataSpaces_selfenergy);
        else
            dataSets.self.write(buffer.selfenergy, mtype_comp,
                                dataSpaces_selfenergy_buffer, dataSpaces_selfenergy);

        count[1] = buffer.irred_dim;
        dataSpaces_irreducible.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.irred_p -> write(buffer.irreducible_class, mtype_comp,
                                      dataSpaces_irreducible_buffer, dataSpaces_irreducible);
        else
            dataSets.irred.write(buffer.irreducible_class, mtype_comp,
                                 dataSpaces_irreducible_buffer, dataSpaces_irreducible);

#if MAX_DIAG_CLASS >= 1
        count[1]= buffer.K1_dim;
        dataSpaces_K1_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        if (!file_exists) {
            dataSets.K1_a_p -> write(buffer.K1_class_a, mtype_comp, dataSpaces_K1_a_buffer, dataSpaces_K1_a);
            dataSets.K1_p_p -> write(buffer.K1_class_p, mtype_comp, dataSpaces_K1_p_buffer, dataSpaces_K1_p);
            dataSets.K1_t_p -> write(buffer.K1_class_t, mtype_comp, dataSpaces_K1_t_buffer, dataSpaces_K1_t);
        }
        else {
            dataSets.K1_a.write(buffer.K1_class_a, mtype_comp, dataSpaces_K1_a_buffer, dataSpaces_K1_a);
            dataSets.K1_p.write(buffer.K1_class_p, mtype_comp, dataSpaces_K1_p_buffer, dataSpaces_K1_p);
            dataSets.K1_t.write(buffer.K1_class_t, mtype_comp, dataSpaces_K1_t_buffer, dataSpaces_K1_t);
        }
#endif

#if MAX_DIAG_CLASS >= 2
        count[1] = nBOS2+FREQ_PADDING*2;
        dataSpaces_bfreqs2a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs2a_p -> write(buffer.bfreqs2a_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs2a_buffer, dataSpaces_bfreqs2a);
        else
            dataSets.bfreqs2a_dataset.write(buffer.bfreqs2a_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs2a_buffer, dataSpaces_bfreqs2a);

        count[1] = nBOS2+FREQ_PADDING*2;
        dataSpaces_bfreqs2p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs2p_p -> write(buffer.bfreqs2p_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs2p_buffer, dataSpaces_bfreqs2p);
        else
            dataSets.bfreqs2p_dataset.write(buffer.bfreqs2p_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs2p_buffer, dataSpaces_bfreqs2p);

        count[1] = nBOS2+FREQ_PADDING*2;
        dataSpaces_bfreqs2t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs2t_p -> write(buffer.bfreqs2t_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs2t_buffer, dataSpaces_bfreqs2t);
        else
            dataSets.bfreqs2t_dataset.write(buffer.bfreqs2t_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs2t_buffer, dataSpaces_bfreqs2t);


        count[1] = nFER2+FREQ_PADDING*2;
        dataSpaces_ffreqs2a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs2a_p -> write(buffer.ffreqs2a_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs2a_buffer, dataSpaces_ffreqs2a);
        else
            dataSets.ffreqs2a_dataset.write(buffer.ffreqs2a_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs2a_buffer, dataSpaces_ffreqs2a);

        count[1] = nFER2+FREQ_PADDING*2;
        dataSpaces_ffreqs2p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs2p_p -> write(buffer.ffreqs2p_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs2p_buffer, dataSpaces_ffreqs2p);
        else
            dataSets.ffreqs2p_dataset.write(buffer.ffreqs2p_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs2p_buffer, dataSpaces_ffreqs2p);

        count[1] = nFER2+FREQ_PADDING*2;
        dataSpaces_ffreqs2t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs2t_p -> write(buffer.ffreqs2t_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs2t_buffer, dataSpaces_ffreqs2t);
        else
            dataSets.ffreqs2t_dataset.write(buffer.ffreqs2t_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs2t_buffer, dataSpaces_ffreqs2t);

        count[1]= buffer.K2_dim;
        dataSpaces_K2_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        if (!file_exists) {
            dataSets.K2_a_p -> write(buffer.K2_class_a, mtype_comp, dataSpaces_K2_a_buffer, dataSpaces_K2_a);
            dataSets.K2_p_p -> write(buffer.K2_class_p, mtype_comp, dataSpaces_K2_p_buffer, dataSpaces_K2_p);
            dataSets.K2_t_p -> write(buffer.K2_class_t, mtype_comp, dataSpaces_K2_t_buffer, dataSpaces_K2_t);
        }
        else {
            dataSets.K2_a.write(buffer.K2_class_a, mtype_comp, dataSpaces_K2_a_buffer, dataSpaces_K2_a);
            dataSets.K2_p.write(buffer.K2_class_p, mtype_comp, dataSpaces_K2_p_buffer, dataSpaces_K2_p);
            dataSets.K2_t.write(buffer.K2_class_t, mtype_comp, dataSpaces_K2_t_buffer, dataSpaces_K2_t);
        }
#endif

#if MAX_DIAG_CLASS >= 3
        count[1] = nBOS3+FREQ_PADDING*2;
        dataSpaces_bfreqs3a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs3a_p -> write(buffer.bfreqs3a_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs3a_buffer, dataSpaces_bfreqs3a);
        else
            dataSets.bfreqs3a_dataset.write(buffer.bfreqs3a_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs3a_buffer, dataSpaces_bfreqs3a);

        count[1] = nBOS3+FREQ_PADDING*2;
        dataSpaces_bfreqs3p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs3p_p -> write(buffer.bfreqs3p_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs3p_buffer, dataSpaces_bfreqs3p);
        else
            dataSets.bfreqs3p_dataset.write(buffer.bfreqs3p_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs3p_buffer, dataSpaces_bfreqs3p);

        count[1] = nBOS3+FREQ_PADDING*2;
        dataSpaces_bfreqs3t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.bfreqs3t_p -> write(buffer.bfreqs3t_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_bfreqs3t_buffer, dataSpaces_bfreqs3t);
        else
            dataSets.bfreqs3t_dataset.write(buffer.bfreqs3t_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_bfreqs3t_buffer, dataSpaces_bfreqs3t);


        count[1] = nFER3+FREQ_PADDING*2;
        dataSpaces_ffreqs3a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs3a_p -> write(buffer.ffreqs3a_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs3a_buffer, dataSpaces_ffreqs3a);
        else
            dataSets.ffreqs3a_dataset.write(buffer.ffreqs3a_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs3a_buffer, dataSpaces_ffreqs3a);

        count[1] = nFER3+FREQ_PADDING*2;
        dataSpaces_ffreqs3p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs3p_p -> write(buffer.ffreqs3p_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs3p_buffer, dataSpaces_ffreqs3p);
        else
            dataSets.ffreqs3p_dataset.write(buffer.ffreqs3p_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs3p_buffer, dataSpaces_ffreqs3p);

        count[1] = nFER3+FREQ_PADDING*2;
        dataSpaces_ffreqs3t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        if (!file_exists)
            dataSets.ffreqs3t_p -> write(buffer.ffreqs3t_buffer, H5::PredType::NATIVE_DOUBLE,
                                        dataSpaces_ffreqs3t_buffer, dataSpaces_ffreqs3t);
        else
            dataSets.ffreqs3t_dataset.write(buffer.ffreqs3t_buffer, H5::PredType::NATIVE_DOUBLE,
                                           dataSpaces_ffreqs3t_buffer, dataSpaces_ffreqs3t);

        count[1]= buffer.K3_dim;
        dataSpaces_K3_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        if (!file_exists) {
            dataSets.K3_a_p -> write(buffer.K3_class_a, mtype_comp, dataSpaces_K3_a_buffer, dataSpaces_K3_a);
            dataSets.K3_p_p -> write(buffer.K3_class_p, mtype_comp, dataSpaces_K3_p_buffer, dataSpaces_K3_p);
            dataSets.K3_t_p -> write(buffer.K3_class_t, mtype_comp, dataSpaces_K3_t_buffer, dataSpaces_K3_t);
        }
        else {
            dataSets.K3_a.write(buffer.K3_class_a, mtype_comp, dataSpaces_K3_a_buffer, dataSpaces_K3_a);
            dataSets.K3_p.write(buffer.K3_class_p, mtype_comp, dataSpaces_K3_p_buffer, dataSpaces_K3_p);
            dataSets.K3_t.write(buffer.K3_class_t, mtype_comp, dataSpaces_K3_t_buffer, dataSpaces_K3_t);
        }
#endif

        print("Successfully saved in hdf5 file: ", FILE_NAME);
        if (file_exists) print_add(" in Lambda-layer ", Lambda_it, false);
        print_add("", true);

        // Terminate
        dataSets.close(file_exists);
        file -> close();
        delete file;

    }  // end of try block

    // catch failure caused by the H5File operations
    catch (H5::FileIException error) {
        error.printErrorStack();
        return;
    }

    // catch failure caused by the DataSet operations
    catch (H5::DataSetIException error) {
        error.printErrorStack();
        return;
    }

    // catch failure caused by the DataSpace operations
    catch (H5::DataSpaceIException error) {
        error.printErrorStack();
        return;
    }
}

/**
 * Write the inital state to an HDF5 file.
 * @param FILE_NAME   : File name. Creates a new file if it does not exist.
 *                      If a file with this name already exists, overwrite the existing file.
 * @param Lambda_i    : Inital Lambda value.
 * @param Lambda_size : Total number of Lambda iterations to be saved in the file.
 * @param state_in    : State to be written to the file.
 */
template <typename Q>
void write_hdf(const H5std_string FILE_NAME, double Lambda_i, long Lambda_size, const State<Q>& state_in) {
#ifdef MPI_FLAG
    if (mpi_world_rank() == 0)  // only the process with ID 0 writes into file to avoid collisions
#endif
    {
    int Lambda_it = 0;  // store data as 0th Lambda iteration

    // List with Lambda values where only the first one is non-zero
    rvec Lambdas (Lambda_size);
    Lambdas[0] = Lambda_i;
    for (int i = 1; i < Lambda_size; i++) {
        Lambdas[i] = 0;
    }

    // write data to file
    save_to_hdf(FILE_NAME, Lambda_it, Lambda_size, state_in, Lambdas, false);
    }
}

/**
 * Add the state of a new iteration to an existing HDF5 file.
 * @param FILE_NAME   : File name.
 * @param Lambda_it   : Lambda iteration at which to save the data.
 * @param Lambda_size : Total number of Lambda iterations saved in the file.
 * @param state_in    : State to be written to the file.
 * @param Lambdas     : Vector containing all Lambda values for which results can be saved in file.
 */
template <typename Q>
void add_hdf(const H5std_string FILE_NAME, int Lambda_it, const State<Q>& state_in, rvec& Lambdas) {
#ifdef MPI_FLAG
    if (mpi_world_rank() == 0)  // only the process with ID 0 writes into file to avoid collisions
#endif
    {
        long Lambda_size = read_Lambdas_from_hdf(FILE_NAME).size();
        // write data to file if Lambda iteration number is in allowed range, otherwise print error message
        if (Lambda_it < Lambda_size) {
            save_to_hdf(FILE_NAME, Lambda_it, Lambda_size, state_in, Lambdas, true);
        } else {
            print("Cannot write to file ", FILE_NAME, " since Lambda layer is out of range.", true);
        }
    }
}
/// Overload of above function that only updates the Lambda at iteration Lambda_it
template <typename Q>
void add_hdf(const H5std_string FILE_NAME, const double Lambda_now, const int Lambda_it, const State<Q>& state_in) {
    rvec Lambdas = read_Lambdas_from_hdf(FILE_NAME);
    Lambdas[Lambda_it] = Lambda_now; // update Lambda
    add_hdf<Q>(FILE_NAME, Lambda_it, state_in, Lambdas);
}


/** overload of add_hdf for non-States, does not do anything */
template <typename Q>
void add_hdf(const H5std_string FILE_NAME, int Lambda_it, long Lambda_size,
             Q& state_in, rvec& Lambdas) {}



/// --- Test function --- ///

void test_hdf5(H5std_string FILE_NAME, int i, State<state_datatype>& state) {
    // test hdf5: read files and compare to original file
    int cnt = 0;
    State<state_datatype> out = read_hdf(FILE_NAME, i);
    for (int iK=0; iK<2; ++iK) {
        for (int iSE = 0; iSE < nSE; ++iSE) {
            if (state.selfenergy.val(iK, iSE, 0) != out.selfenergy.val(iK, iSE, 0)) {
                std::cout << "Self-energy not equal, " << iK << ", " << iSE << std::endl;
                cnt += 1;
            }
        }
    }

    for (int iK=0; iK<nK_K1; ++iK) {
        for (int i_in=0; i_in<n_in; ++i_in) {
#if MAX_DIAG_CLASS >= 1
            for (int iw1=0; iw1<nBOS+FREQ_PADDING*2; ++iw1) {
                if (state.vertex[0].avertex().K1.val(iK, iw1, i_in) != out.vertex[0].avertex().K1.val(iK, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
                if (state.vertex[0].pvertex().K1.val(iK, iw1, i_in) != out.vertex[0].pvertex().K1.val(iK, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
                if (state.vertex[0].tvertex().K1.val(iK, iw1, i_in) != out.vertex[0].tvertex().K1.val(iK, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
#if MAX_DIAG_CLASS >= 2
                for (int iw2=0; iw2<nFER+FREQ_PADDING*2; ++iw2) {
                    if (state.vertex[0].avertex().K2.val(iK, iw1, iw2, i_in) != out.vertex[0].avertex().K2.val(iK, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
                    if (state.vertex[0].pvertex().K2.val(iK, iw1, iw2, i_in) != out.vertex[0].pvertex().K2.val(iK, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
                    if (state.vertex[0].tvertex().K2.val(iK, iw1, iw2, i_in) != out.vertex[0].tvertex().K2.val(iK, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
#if MAX_DIAG_CLASS == 3
                    for (int iw3=0; iw3<nFER; ++iw3) {
                        if (state.vertex[0].avertex().K3.val(iK, iw1, iw2, iw3, i_in) != out.vertex[0].avertex().K3.val(iK, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                        if (state.vertex[0].pvertex().K3.val(iK, iw1, iw2, iw3, i_in) != out.vertex[0].pvertex().K3.val(iK, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                        if (state.vertex[0].tvertex().K3.val(iK, iw1, iw2, iw3, i_in) != out.vertex[0].tvertex().K3.val(iK, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                    }
#endif
                }
#endif
            }
#endif
        }
    }
    if (cnt == 0) print("HDF5 test successful.", true);
    else print("HDF5 test failed. Number of differences: ", cnt, true);
}

#endif //KELDYSH_MFRG_HDF5_ROUTINES_H