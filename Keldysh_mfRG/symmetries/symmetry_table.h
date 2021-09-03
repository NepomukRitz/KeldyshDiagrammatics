/**
 * This file contains all information of the symmetry-transformations table:
 *  - The struct Components contains the information to which independent Keldysh components all other Keldysh
 *      components in each diagrammatic class K1, K2, K2b, K3 are related, and
 *  - the struct Transformations contains the information how this relation is achieved, i.e., which transformations
 *      have to be applied to access a stored Keldysh component symmetry-related to the desired one.
 */

#ifndef KELDYSH_MFRG_TABLE_H
#define KELDYSH_MFRG_TABLE_H

#include "../data_structures.h"

/** Relate the Keldysh components in each diagrammatic class to the independent ones:
* -1 = this component is zero
*  0 = related to component 0
*  1 = related to component 1
*  ...
*/
struct Components {
    std::vector<std::vector<std::vector<int> > > K
        = std::vector<std::vector<std::vector<int> > > (4,
                                         std::vector<std::vector<int> > (2,
                                                               std::vector<int> (16)));

    Components() = default;
    explicit Components(const char channel) {
        if (KELDYSH){
            switch (channel) {
                case 'a':
                    K[k1] = {std::vector<int> ({-1,  0,  0,  1,
                                                0,  1, -1,  0,
                                                0, -1,  1,  0,
                                                1,  0,  0, -1}),    // spin comp. V
                             std::vector<int> ({-1,  0,  0,  1,
                                                0,  1, -1,  0,
                                                0, -1,  1,  0,
                                                1,  0,  0, -1})};   // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  1,  2,  3,
                                                 2,  3,  0,  1,
                                                 1, -1,  3,  4,
                                                 3,  4,  1, -1}),    // spin comp. V
                             std::vector<int> ({ 0,  1,  2,  3,
                                                 2,  3,  0,  1,
                                                 1, -1,  3,  4,
                                                 3,  4,  1, -1})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({0,  2,  1,  3,
                                                 1,  3, -1,  4,
                                                 2,  0,  3,  1,
                                                 3,  1,  4, -1}),    // spin comp. V
                              std::vector<int> ({0,  2,  1,  3,
                                                 1,  3, -1,  4,
                                                 2,  0,  3,  1,
                                                 3,  1,  4, -1})};   // spin comp. Vhat
                    K[k3] =  {std::vector<int> ({0,  1,  1,  2,
                                                 1,  3,  4,  5,
                                                 1,  4,  3,  5,
                                                 2,  5,  5, -1}),    // spin comp. V
                              std::vector<int> ({0,  1,  1,  2,
                                                 1,  4,  3,  5,
                                                 1,  3,  4,  5,
                                                 2,  5,  5, -1})};   // spin comp. Vhat
                    break;
                case 'p':
                    K[k1] = {std::vector<int> ({-1,  0,  0, -1,
                                                0,  1,  1,  0,
                                                0,  1,  1,  0,
                                                -1,  0,  0, -1}),    // spin comp. V
                             std::vector<int> ({-1,  0,  0, -1,
                                                0,  1,  1,  0,
                                                0,  1,  1,  0,
                                                -1,  0,  0, -1})};   // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  1,  1,  0,
                                                 2,  3,  3,  2,
                                                 2,  3,  3,  2,
                                                 -1,  4,  4, -1}),    // spin comp. V
                             std::vector<int> ({ 0,  1,  1,  0,
                                                 2,  3,  3,  2,
                                                 2,  3,  3,  2,
                                                 -1,  4,  4, -1})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({0,  2,  2, -1,
                                                 1,  3,  3,  4,
                                                 1,  3,  3,  4,
                                                 0,  2,  2, -1}),    // spin comp. V
                              std::vector<int> ({0,  2,  2, -1,
                                                 1,  3,  3,  4,
                                                 1,  3,  3,  4,
                                                 0,  2,  2, -1})};   // spin comp. Vhat
                    K[k3] =  {std::vector<int> ({0,  1,  1,  2,
                                                 1,  3,  4,  5,
                                                 1,  4,  3,  5,
                                                 2,  5,  5, -1}),    // spin comp. V
                              std::vector<int> ({0,  1,  1,  2,
                                                 1,  4,  3,  5,
                                                 1,  3,  4,  5,
                                                 2,  5,  5, -1})};   // spin comp. Vhat
                    break;
                case 't':
                    K[k1] = {std::vector<int> ({-1,  0,  0,  1,
                                                0, -1,  1,  0,
                                                0,  1, -1,  0,
                                                1,  0,  0, -1}),    // spin comp. V
                             std::vector<int> ({-1,  0,  0,  1,
                                                0, -1,  1,  0,
                                                0,  1, -1,  0,
                                                1,  0,  0, -1})};   // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  1,  2,  3,
                                                 1, -1,  3,  4,
                                                 2,  3,  0,  1,
                                                 3,  4,  1, -1}),    // spin comp. V
                             std::vector<int> ({ 0,  1,  2,  3,
                                                 1, -1,  3,  4,
                                                 2,  3,  0,  1,
                                                 3,  4,  1, -1})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({0,  2,  1,  3,
                                                 2,  0,  3,  1,
                                                 1,  3, -1,  4,
                                                 3,  1,  4, -1}),    // spin comp. V
                              std::vector<int> ({0,  2,  1,  3,
                                                 2,  0,  3,  1,
                                                 1,  3, -1,  4,
                                                 3,  1,  4, -1})};   // spin comp. Vhat
                    K[k3]  = {std::vector<int> ({0,  1,  1,  2,
                                                 1,  3,  4,  5,
                                                 1,  4,  3,  5,
                                                 2,  5,  5, -1}),    // spin comp. V
                              std::vector<int> ({0,  1,  1,  2,
                                                 1,  4,  3,  5,
                                                 1,  3,  4,  5,
                                                 2,  5,  5, -1})};   // spin comp. Vhat
                    break;
                default:;
            }
        }
        else{
            K[k1]  = { std::vector<int> ({0}), std::vector<int> ({0}) };
            K[k2]  = { std::vector<int> ({0}), std::vector<int> ({0}) };
            K[k2b] = { std::vector<int> ({0}), std::vector<int> ({0}) };
            K[k3]  = { std::vector<int> ({0}), std::vector<int> ({0}) };
        }
    }
};

/** Transformations that need to be applied to the respective stored components to get the correct actual components:
* 0 = nothing, 1 = T1, 2 = T2, 3 = T3, 4 = TC
* Convention for composite trafos: 43 = first apply 4, then 3 etc. Careful, some operations do not commute!
*/
struct Transformations {
    std::vector<std::vector<std::vector<int> > > K
            = std::vector<std::vector<std::vector<int> > > (4,
                                             std::vector<std::vector<int> > (2,
                                                                   std::vector<int> (16)));

    Transformations() = default;
    explicit Transformations(const char channel) {
        if (KELDYSH){
            switch (channel) {
                case 'a':
                    K[k1] = {std::vector<int> ({ 0,  0,  3,  0,
                                                 3,  0,  0,  0,
                                                 0,  0,  0,  3,
                                                 0,  3,  0,  0}),    // spin comp. V
                             std::vector<int> ({ 0,  2,  1,  1,
                                                 1,  1,  0,  2,
                                                 2,  0,  1,  1,
                                                 1,  1,  2,  0})};   // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  0,  0,  0,
                                                 0,  0,  0,  0,
                                                 43,  0, 43,  0,
                                                 43,  0, 43,  0}),   // spin comp. V
                             std::vector<int> ({ 2,  2,  2,  2,
                                                 2,  2,  2,  2,
                                                 41,  0, 41,  2,
                                                 41,  2, 41,  0})};  // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({ 3,  3,  3,  3,
                                                  4,  4,  0,  3,
                                                  3,  3,  3,  3,
                                                  4,  4,  3,  0}),   // spin comp. V
                              std::vector<int> ({ 1,  1,  1,  1,
                                                  14, 14,  0,  1,
                                                  1,  1,  1,  1,
                                                  14, 14,  1,  0})}; // spin comp. Vhat
                    K[k3] =  {std::vector<int> ({ 0,  0,  3,  0,
                                                  4,  0,  0,  0,
                                                  43,  3,  3,  3,
                                                  4,  4, 43,  0}),    // spin comp. V
                              std::vector<int> ({ 1,  2,  1,  1,
                                                  14,  1,  1,  1,
                                                  41,  2,  2,  2,            //Uses TCT2 = T1TC
                                                  14, 41, 14,  0})};  //spin comp. Vhat
                    break;
                case 'p':
                    K[k1] = {std::vector<int> ({ 0,  0,  0,  0,
                                                 4,  0,  0,  4,
                                                 4,  0,  0,  4,
                                                 0,  0,  0,  0}),        // spin comp. V
                             std::vector<int> ({ 0,  1,  1,  0,
                                                 14,  1,  1, 14,
                                                 14,  1,  1, 14,
                                                 0,  1,  1,  0})};    // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  0,  0,  0,
                                                 0,  0,  0,  0,
                                                 3,  3,  3,  3,
                                                 0,  0,  0,  0}),    // spin comp. V
                             std::vector<int> ({ 1,  1,  1,  1,
                                                 1,  1,  1,  1,
                                                 2,  2,  2,  2,
                                                 0,  1,  1,  0})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({4,  4, 43,  0,
                                                 4,  4, 43,  4,
                                                 4,  4, 43,  4,
                                                 4,  4, 43,  0}),    // spin comp. V
                              std::vector<int> ({41, 41, 14,  0,
                                                 14, 41, 14, 41,
                                                 14, 41, 14, 41,
                                                 41, 41, 14,  0})};   // spin comp. Vhat
                    K[k3] = {std::vector<int> ({ 0,  0,  3,  0,
                                                 4,  0,  0,  0,
                                                 43,  3,  3,  3,
                                                 4,  4, 43,  0}),    // spin comp. V
                             std::vector<int> ({ 1,  2,  1,  1,
                                                 14,  1,  1,  1,
                                                 41,  2,  2,  2,
                                                 14, 41, 14,  0})};  // spin comp. Vhat
                    break;
                case 't':
                    K[k1] = {std::vector<int> ({ 0,  0,  3,  0,
                                                 0,  0,  0,  3,
                                                 3,  0,  0,  0,
                                                 0,  3,  0,  0}),        // spin comp. V
                             std::vector<int> ({ 0,  2,  1,  1,
                                                 2,  0,  1,  1,
                                                 1,  1,  0,  2,
                                                 1,  1,  2,  0})};       // spin comp. Vhat
                    K[k2] = {std::vector<int> ({ 0,  0,  0,  0,
                                                 4,  0,  4,  0,
                                                 0,  0,  0,  0,
                                                 4,  0,  4,  0}),        // spin comp. V
                             std::vector<int> ({ 2,  2,  2,  2,
                                                 14,  0, 14,  2,
                                                 2,  2,  2,  2,
                                                 14,  2, 14,  0})};  // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({3,  3,  3,  3,
                                                 3,  3,  3,  3,
                                                 43, 43,  0,  3,
                                                 43, 43,  3,  0}),  // spin comp. V
                              std::vector<int> ({ 1,  1,  1,  1,
                                                  1,  1,  1,  1,
                                                  41, 41,  0,  1,
                                                  41, 41,  1,  0})}; // spin comp. Vhat
                    K[k3] = {std::vector<int> ({ 0,  0,  3,  0,
                                                 4,  0,  0,  0,
                                                 43,  3,  3,  3,
                                                 4,  4, 43,  0}),    // spin comp. V
                             std::vector<int> ({ 1,  2,  1,  1,
                                                 14,  1,  1,  1,
                                                 41,  2,  2,  2,
                                                 14, 41, 14,  0})}; // spin comp. Vhat
                    break;
                default:;
            }
        }
        else{
            switch (channel) {
                case 'a':
                    K[k1]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k2]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 2})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({ 3}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k3]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   //spin comp. Vhat
                    break;
                case 'p':
                    K[k1]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k2]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({ 4}),    // spin comp. V
                              std::vector<int> ({41})};   // spin comp. Vhat
                    K[k3]  = {std::vector<int> ({  0}),   // spin comp. V
                              std::vector<int> ({  1})};  // spin comp. Vhat
                    break;
                case 't':
                    K[k1]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k2]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 2})};   // spin comp. Vhat
                    K[k2b] = {std::vector<int> ({ 3}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    K[k3]  = {std::vector<int> ({ 0}),    // spin comp. V
                              std::vector<int> ({ 1})};   // spin comp. Vhat
                    break;
                default:;
            }
        }
    }
};

#ifdef KELDYSH_FORMALISM
#ifndef PARTICLE_HOLE_SYMM
std::vector<std::vector<int>> ComponentsK1a {{0, 0}, {0, 0}};
std::vector<std::vector<int>> ComponentsK1p {{0, 1}, {0, 1}};
std::vector<std::vector<int>> ComponentsK1t {{0, 0}, {0, 0}};
std::vector<std::vector<int>> ComponentsK2a {{0, 1, 0, 1}, {0, 1, 2, 3}, {0, 1, 0, 1}, {0, 1, 2, 3}, {0, 1, 0, 1}};
std::vector<std::vector<int>> ComponentsK2p {{0, 0, 2, 2}, {0, 0, 2, 2}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 0, 2, 2}};
std::vector<std::vector<int>> ComponentsK2t {{0, 1, 0, 1}, {0, 1, 2, 3}, {0, 1, 0, 1}, {0, 1, 2, 3}, {0, 1, 0, 1}};
std::vector<std::vector<int>> ComponentsK3a {{0, 0, 2, 2, 0, 0, 2, 2}, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, 1, 0, 3, 2}, {0, 0, 2, 2, 4, 4, 6, 6}, {0, 1, 2, 3, 0, 1, 2, 3}, {0, 1, 2, 3, 4, 5, 6, 7}};
std::vector<std::vector<int>> ComponentsK3p {{0, 0, 0, 0, 4, 4, 4, 4}, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 1, 0, 4, 5, 5, 4}, {0, 0, 2, 2, 4, 4, 6, 6}, {0, 1, 0, 1, 4, 5, 4, 5}, {0, 1, 2, 3, 4, 5, 6, 7}};
std::vector<std::vector<int>> ComponentsK3t {{0, 0, 2, 2, 0, 0, 2, 2}, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2, 3, 1, 0, 3, 2}, {0, 1, 2, 3, 0, 1, 2, 3}, {0, 0, 2, 2, 4, 4, 6, 6}, {0, 1, 2, 3, 4, 5, 6, 7}};
std::vector<std::vector<int>> TransformaK1a {{0, 34}, {0, 3}};
std::vector<std::vector<int>> TransformaK1p {{0, 0}, {0, 0}};
std::vector<std::vector<int>> TransformaK1t {{0, 4}, {0, 3}};
std::vector<std::vector<int>> TransformaK2a {{0, 0, 34, 34}, {0, 0, 0, 0}, {0, 0, 34, 34}, {0, 0, 0, 0}, {0, 0, 34, 34}};
std::vector<std::vector<int>> TransformaK2p {{0, 3, 0, 3}, {0, 3, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 3, 0, 3}};
std::vector<std::vector<int>> TransformaK2t {{0, 0, 4, 4}, {0, 0, 0, 0}, {0, 0, 4, 4}, {0, 0, 0, 0}, {0, 0, 4, 4}};
std::vector<std::vector<int>> TransformaK3a {{0, 4, 0, 4, 34, 3, 34, 3}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 3, 3, 3, 3}, {0, 4, 0, 4, 0, 4, 0, 4}, {0, 0, 0, 0, 34, 34, 34, 34}, {0, 0, 0, 0, 0, 0, 0, 0}};
std::vector<std::vector<int>> TransformaK3p {{0, 4, 34, 3, 0, 4, 34, 3}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 3, 3, 0, 0, 3, 3}, {0, 4, 0, 4, 0, 4, 0, 4}, {0, 0, 34, 34, 0, 0, 34, 34}, {0, 0, 0, 0, 0, 0, 0, 0}};
std::vector<std::vector<int>> TransformaK3t {{0, 34, 0, 34, 4, 3, 4, 3}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 3, 3, 3, 3}, {0, 0, 0, 0, 4, 4, 4, 4}, {0, 34, 0, 34, 0, 34, 0, 34}, {0, 0, 0, 0, 0, 0, 0, 0}};
#else
std::vector<std::vector<int>> ComponentsK1a {{0, 0}, {0, 0}};
std::vector<std::vector<int>> ComponentsK1p {{0, 0}, {0, 0}};
std::vector<std::vector<int>> ComponentsK1t {{0, 0}, {0, 0}};
std::vector<std::vector<int>> ComponentsK2a {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK2p {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK2t {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK3a {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 3, 2, 1, 0}, {0, 1, 0, 1, 1, 0, 1, 0}, {0, 0, 2, 2, 2, 2, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 2, 3, 3, 2, 1, 0}};
std::vector<std::vector<int>> ComponentsK3p {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 3, 2, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 2, 2, 2, 2, 0, 0}, {0, 1, 0, 1, 1, 0, 1, 0}, {0, 1, 2, 3, 3, 2, 1, 0}};
std::vector<std::vector<int>> ComponentsK3t {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 3, 2, 1, 0}, {0, 1, 0, 1, 1, 0, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 2, 2, 2, 2, 0, 0}, {0, 1, 2, 3, 3, 2, 1, 0}};
std::vector<std::vector<int>> TransformaK1a {{0, 6}, {0, 3}};
std::vector<std::vector<int>> TransformaK1p {{0, 6}, {0, 6}};
std::vector<std::vector<int>> TransformaK1t {{0, 6}, {0, 3}};
std::vector<std::vector<int>> TransformaK2a {{0, 346, 34, 6}, {0, 0, 6, 6}, {0, 346, 34, 6}, {0, 0, 6, 6}, {0, 346, 34, 6}};
std::vector<std::vector<int>> TransformaK2p {{0, 3, 36, 6}, {0, 3, 36, 6}, {0, 0, 6, 6}, {0, 0, 6, 6}, {0, 3, 36, 6}};
std::vector<std::vector<int>> TransformaK2t {{0, 46, 4, 6}, {0, 0, 6, 6}, {0, 46, 4, 6}, {0, 0, 6, 6}, {0, 46, 4, 6}};
std::vector<std::vector<int>> TransformaK3a {{0, 4, 36, 346, 34, 3, 46, 6}, {0, 0, 0, 0, 6, 6, 6, 6}, {0, 0, 36, 36, 3, 3, 6, 6}, {0, 4, 0, 4, 46, 6, 46, 6}, {0, 0, 346, 346, 34, 34, 6, 6}, {0, 0, 0, 0, 6, 6, 6, 6}};
std::vector<std::vector<int>> TransformaK3p {{0, 4, 34, 3, 36, 346, 46, 6}, {0, 0, 0, 0, 6, 6, 6, 6}, {0, 0, 3, 3, 36, 36, 6, 6}, {0, 4, 0, 4, 46, 6, 46, 6}, {0, 0, 34, 34, 346, 346, 6, 6}, {0, 0, 0, 0, 6, 6, 6, 6}};
std::vector<std::vector<int>> TransformaK3t {{0, 34, 36, 46, 4, 3, 346, 6}, {0, 0, 0, 0, 6, 6, 6, 6}, {0, 0, 36, 36, 3, 3, 6, 6}, {0, 0, 46, 46, 4, 4, 6, 6}, {0, 34, 0, 34, 346, 6, 346, 6}, {0, 0, 0, 0, 6, 6, 6, 6}};
#endif
#else
std::vector<std::vector<int>> ComponentsK1a {{0, 0}};
std::vector<std::vector<int>> ComponentsK1p {{0, 0}};
std::vector<std::vector<int>> ComponentsK1t {{0, 0}};
std::vector<std::vector<int>> ComponentsK2a {{0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK2p {{0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK2t {{0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK3a {{0, 0, 0, 0, 0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK3p {{0, 0, 0, 0, 0, 0, 0, 0}};
std::vector<std::vector<int>> ComponentsK3t {{0, 0, 0, 0, 0, 0, 0, 0}};
std::vector<std::vector<int>> TransformaK1a {{0, 3}};
std::vector<std::vector<int>> TransformaK1p {{0, 4}};
std::vector<std::vector<int>> TransformaK1t {{0, 3}};
std::vector<std::vector<int>> TransformaK2a {{0, 34, 347, 7}};
std::vector<std::vector<int>> TransformaK2p {{0, 3, 37, 7}};
std::vector<std::vector<int>> TransformaK2t {{0, 4, 47, 7}};
std::vector<std::vector<int>> TransformaK3a {{0, 47, 37, 34, 347, 3, 4, 7}};
std::vector<std::vector<int>> TransformaK3p {{0, 47, 347, 3, 37, 34, 4, 7}};
std::vector<std::vector<int>> TransformaK3t {{0, 347, 37, 4, 47, 3, 34, 7}};
#endif



struct FrequencyComponents {
    std::vector<std::vector<int>> K1, K2, K3;

    FrequencyComponents() {};
    FrequencyComponents(const char channel) {

        switch (channel) {
            case 'a':

                K1 = ComponentsK1a;
                K2 = ComponentsK2a;
                K3 = ComponentsK3a;
                break;
            case 'p':
                K1 = ComponentsK1p;
                K2 = ComponentsK2p;
                K3 = ComponentsK3p;
                break;
            case 't':
                K1 = ComponentsK1t;
                K2 = ComponentsK2t;
                K3 = ComponentsK3t;
                break;
            default:;
        }
    }
};


struct FrequencyTransformations {
    std::vector<std::vector<int>> K1, K2, K3;

    FrequencyTransformations() {};
    FrequencyTransformations(const char channel) {

        switch (channel) {
            case 'a':

                K1 = TransformaK1a;
                K2 = TransformaK2a;
                K3 = TransformaK3a;
                break;
            case 'p':
                K1 = TransformaK1p;
                K2 = TransformaK2p;
                K3 = TransformaK3p;
                break;
            case 't':
                K1 = TransformaK1t;
                K2 = TransformaK2t;
                K3 = TransformaK3t;
                break;
            default:;
        }
    }
};

#endif //KELDYSH_MFRG_TABLE_H