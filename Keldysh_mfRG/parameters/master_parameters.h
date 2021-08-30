#ifndef KELDYSH_MFRG_PARAMETERS_H
#define KELDYSH_MFRG_PARAMETERS_H

#include "frequency_parameters.h"
#include "technical_parameters.h"
#include <cmath>             // log function
#include <vector>            // standard vector for Keldysh indices

// For production: uncomment the following line to switch off assert()-functions
//#define NDEBUG

// Determines whether the 2D Hubbard model shall be studied instead of the SIAM
#define HUBBARD
#ifdef HUBBARD
constexpr bool HUBBARD_MODEL = true;
#else
constexpr bool HUBBARD_MODEL = false;
#endif

// Defines the formalism (not defined: Matsubara formalism, defined: Keldysh formalism)
#define KELDYSH_FORMALISM
#ifdef KELDYSH_FORMALISM
constexpr bool KELDYSH = true;
constexpr bool ZERO_T = false; // only needed for Matsubara
#else
constexpr bool KELDYSH = false;
#define ZERO_TEMP   // Determines whether to work in the T = 0 limit (in the Matsubara formalism)
#ifdef ZERO_TEMP
constexpr bool ZERO_T = true;
#endif
#endif

// Determines whether particle-hole symmetry is assumed
//#define PARTICLE_HOLE_SYMM
#ifdef PARTICLE_HOLE_SYMM
constexpr bool PARTICLE_HOLE_SYMMETRY = true;
#else
constexpr bool PARTICLE_HOLE_SYMMETRY = false;
#endif


/// Production runs parameters ///

// Defines the number of diagrammatic classes that are relevant for a code:
// 1 for only K1, 2 for K1 and K2 and 3 for the full dependencies
#define MAX_DIAG_CLASS 3

constexpr int N_LOOPS = 1;  // Number of loops
#define SELF_ENERGY_FLOW_CORRECTIONS

// If defined, use static K1 inter-channel feedback as done by Severin Jakobs.
// Only makes sense for pure K1 calculations.
//#define STATIC_FEEDBACK

/// Physical parameters ///
#if not defined(KELDYSH_FORMALISM) and not defined(ZERO_TEMP)
constexpr double glb_T = 2./M_PI; //0.01;                     // Temperature
#else
constexpr double glb_T = 0.01;                     // Temperature
#endif
#ifdef PARTICLE_HOLE_SYMM
    constexpr double glb_mu = 0.000;                     // Chemical potential // set to zero as energy offset
#else
    constexpr double glb_mu = 0.000;                    // Chemical potential // set to zero as energy offset
#endif
constexpr double glb_U = 1.0;                      // Impurity on-site interaction strength
constexpr double glb_Vg = glb_mu;                  // Impurity level shift
constexpr double glb_epsilon = glb_Vg - glb_U/2.;  // Impurity on-site energy                                               //NOLINT(cert-err58-cpp)
constexpr double glb_Gamma = 1./5.;                // Hybridization of Anderson model
constexpr double glb_V = 0.;                       // Bias voltage (glb_V == 0. in equilibrium)
constexpr bool EQUILIBRIUM = true;                 // If defined, use equilibrium FDT's for propagators
                                                   // (only sensible when glb_V = 0)




/// Spin parameters ///

// Number of independent spin components. n_spin = 1 with SU(2) symmetry.
constexpr int n_spin = 1;

/// Parameters for internal structure ///

// Dimension of the space defining the internal structure for the Hubbard model
constexpr int glb_N_q = 9; // Number of transfer momentum points in one dimension.
constexpr int glb_N_transfer = glb_N_q * (glb_N_q + 1) / 2; // Integer division fine, as glb_N_q * (glb_N_q + 1) is always even.

#ifdef HUBBARD
constexpr int n_in = glb_N_transfer;
#else
constexpr int n_in = 1;
#endif

/// fRG parameters ///

// Regulator
// 1: sharp cutoff, 2: hybridization flow, 3: frequency regulator (as used in Vienna, Stuttgart, Tuebingen)
#define REG 3

// Computation is flowing or not (determines the value of the vertex).
// Define FLOW for flow and comment out for static calculation
//#define FLOW


constexpr int nODE = 50;

// Limits of the fRG flow
constexpr double Lambda_ini = 20.;                // NOLINT(cert-err58-cpp)
constexpr double Lambda_fin = 0.0;
constexpr double Lambda_scale = 1./200.;             //Scale of the log substitution

// Vector with the values of U for which we have NRG data to compare with (exclude zero!)
// Attention: these values are in units of Delta/2, not Delta -> corresponding U_fRG values are twice as large!
std::vector<double> U_NRG {0.05, 0.1, 0.2, 0.25, 0.5, 0.75, 1., 1.2, 1.25, 1.5, 1.75, 2., 2.25, 2.5, 3., 5.};                                                    // NOLINT(cert-err58-cpp)


#if REG==2
constexpr int param_size = 14;
constexpr double parameter_list[param_size] = {GRID, REG, glb_Gamma, MAX_DIAG_CLASS, N_LOOPS,
                                           glb_T, glb_mu, glb_U, glb_epsilon, glb_V, glb_w_upper, glb_w_lower, glb_v_upper, glb_v_lower};
#else
constexpr int param_size = 13;
constexpr double parameter_list[param_size] = {GRID, REG, MAX_DIAG_CLASS, N_LOOPS,
                                           glb_T, glb_mu, glb_U, glb_epsilon, glb_V, glb_w_upper, glb_w_lower, glb_v_upper, glb_v_lower};
#endif


#endif //KELDYSH_MFRG_PARAMETERS_H
