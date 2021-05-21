//
// Created by nepomuk on 27.04.21.
//

#ifndef KELDYSH_MFRG_TESTING_TEST_MOMENTUM_GRID_H
#define KELDYSH_MFRG_TESTING_TEST_MOMENTUM_GRID_H

#include "../momentum_grid.h"
#include <cassert>

void test_index_conversions(){
    for (int n_x = 0; n_x < glb_N_q; ++n_x) {
        for (int n_y = 0; n_y < n_x+1; ++n_y) {
            int n = momentum_index(n_x, n_y);
            int n_x_recalc, n_y_recalc;
            get_n_x_and_n_y(n, n_x_recalc, n_y_recalc);
            assert (n_x - n_x_recalc == 0);
            assert (n_y - n_y_recalc == 0);
        }
    }
    std::cout << "All momentum-index recalculations successful!" << "\n";
}


#endif //KELDYSH_MFRG_TESTING_TEST_MOMENTUM_GRID_H
