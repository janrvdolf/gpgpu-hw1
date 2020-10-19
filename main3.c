#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MATRIX_DIM 3

double random_double(int upper) {
    return rand() % upper;
}

void print_matrix(double * matrix) {
    for (int y = 0; y < MATRIX_DIM; y++) {
        for (int x = 0; x < MATRIX_DIM +1; x++) {
            int idx = y * (MATRIX_DIM+1) + x;
            printf("%lf ", matrix[idx]);
        }
        printf("\n");
    }
}

void matrix_triangulation (double * matrix) {
    for (int j = 0; j < MATRIX_DIM; j++) {
        for (int i = j + 1; i < MATRIX_DIM; i++) {
            int idx_diagonal = j * (MATRIX_DIM+1) + j;
            int idx = i * (MATRIX_DIM+1) + j;

            double f = matrix[idx]/matrix[idx_diagonal];

            for (int k = 0; k < MATRIX_DIM+1; k++) {
                int idx_i = i * (MATRIX_DIM+1) + k;
                int idx_j = j * (MATRIX_DIM+1) + k;

                matrix[idx_i] -= matrix[idx_j] * f;
            }
        }
    }
}

int main () {
//    for(int i = 0; i < 5; i++) {
//        int lower = 0;
//        int upper = 10;
//        double num = (rand() % (upper - lower + 1)) + lower;
//
//        printf(" %lf ", num);
//    }

    int m_size = MATRIX_DIM*MATRIX_DIM+MATRIX_DIM;

    double m_fill[12] = {9, 3, 4, 7,  4, 3 , 4, 8, 1, 1, 1, 3};

    double *matrix = (double *) malloc(sizeof(double) * m_size); // matrix A with  b

    double *solution = (double *) malloc(sizeof(double) * MATRIX_DIM);

    for (int i = 0; i < MATRIX_DIM; i++) {
        solution[i] = 1;
    }

    for (int i = 0; i < m_size; i++) {
        double r = m_fill[i]; // random_double(10);
        matrix[i] = r;

        printf("Generated at %d random number %lf\n", i, r);
    }

    printf("Matrix:\n");
    print_matrix(matrix);

    matrix_triangulation(matrix);

    printf("Matrix:\n");
    print_matrix(matrix);

    int is_one_solution = 1;

    for (int i = 0; i < MATRIX_DIM; i++) {
        int idx = i * (MATRIX_DIM + 1) + i;

        if (fabs(matrix[idx]) < 0.0001) {
            is_one_solution = 0;
        }
    }

    if (is_one_solution) {
        for (int i = MATRIX_DIM - 1; i >= 0; i--) {
            double sum = 0;
            for (int k = i + 1; k < MATRIX_DIM; k++) {
                int idx = i * (MATRIX_DIM+1) + k;

                sum += solution[k] * matrix[idx];
            }

            int idx = (i+1) * (MATRIX_DIM+1) -1 ;

            solution[i] = (matrix[idx] - sum) / matrix[i * (MATRIX_DIM+1) + i];
        }

        for (int i = 0; i < MATRIX_DIM; i++) {
            printf("x_%d = %lf\n", i, solution[i]);
        }
    } else {
        printf("Multiple solutions of no solutions.");
    }

    free(matrix);

    free(solution);

    return 0;
}