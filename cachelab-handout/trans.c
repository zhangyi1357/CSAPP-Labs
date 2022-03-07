/*
 * trans.c - Matrix transpose B = A^T
 *
 * Author: Zhang Yi
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>

#include "cachelab.h"

// #define BLOCK_SIZE 8

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    // int block_size = 4;
    // int blocks_in_a_line = M / block_size;

    if (M == N && M == 32) {
        for (int ii = 0; ii < 4; ++ii) {
            for (int jj = 0; jj < 4; ++jj) {
                if (ii != jj) {
                    for (int i = ii * 8; i < (ii + 1) * 8; ++i) {
                        for (int j = jj * 8; j < (jj + 1) * 8; ++j) {
                            B[i][j] = A[j][i];
                        }
                    }
                } else {  // on the diagonal
                    for (int i = ii * 8; i < (ii + 1) * 8; ++i) {
                        int tmp0 = A[jj * 8 + 0][i];
                        int tmp1 = A[jj * 8 + 1][i];
                        int tmp2 = A[jj * 8 + 2][i];
                        int tmp3 = A[jj * 8 + 3][i];
                        int tmp4 = A[jj * 8 + 4][i];
                        int tmp5 = A[jj * 8 + 5][i];
                        int tmp6 = A[jj * 8 + 6][i];
                        int tmp7 = A[jj * 8 + 7][i];
                        B[i][jj * 8 + 0] = tmp0;
                        B[i][jj * 8 + 1] = tmp1;
                        B[i][jj * 8 + 2] = tmp2;
                        B[i][jj * 8 + 3] = tmp3;
                        B[i][jj * 8 + 4] = tmp4;
                        B[i][jj * 8 + 5] = tmp5;
                        B[i][jj * 8 + 6] = tmp6;
                        B[i][jj * 8 + 7] = tmp7;
                    }
                }
            }
        }
    } else if (M == N && M == 64) {
        for (int ii = 0; ii < 64; ii += 8) {
            for (int jj = 0; jj < 64; jj += 8) {
                for (int x = 0; x < 4; ++x) {
                    int tmp0 = A[ii + x][jj + 0];
                    int tmp1 = A[ii + x][jj + 1];
                    int tmp2 = A[ii + x][jj + 2];
                    int tmp3 = A[ii + x][jj + 3];
                    int tmp4 = A[ii + x][jj + 4];
                    int tmp5 = A[ii + x][jj + 5];
                    int tmp6 = A[ii + x][jj + 6];
                    int tmp7 = A[ii + x][jj + 7];
                    B[jj + 0][ii + x] = tmp0;
                    B[jj + 1][ii + x] = tmp1;
                    B[jj + 2][ii + x] = tmp2;
                    B[jj + 3][ii + x] = tmp3;
                    B[jj + 0][ii + x + 4] = tmp4;
                    B[jj + 1][ii + x + 4] = tmp5;
                    B[jj + 2][ii + x + 4] = tmp6;
                    B[jj + 3][ii + x + 4] = tmp7;
                }
                for (int x = 0; x < 4; ++x) {
                    int tmp0 = A[ii + 4][x + jj];
                    int tmp1 = A[ii + 5][x + jj];
                    int tmp2 = A[ii + 6][x + jj];
                    int tmp3 = A[ii + 7][x + jj];
                    int tmp4 = B[x + jj][ii + 4];
                    int tmp5 = B[x + jj][ii + 5];
                    int tmp6 = B[x + jj][ii + 6];
                    int tmp7 = B[x + jj][ii + 7];

                    B[x + jj][ii + 4] = tmp0;
                    B[x + jj][ii + 5] = tmp1;
                    B[x + jj][ii + 6] = tmp2;
                    B[x + jj][ii + 7] = tmp3;
                    B[x + jj + 4][ii] = tmp4;
                    B[x + jj + 4][ii + 1] = tmp5;
                    B[x + jj + 4][ii + 2] = tmp6;
                    B[x + jj + 4][ii + 3] = tmp7;
                }
                for (int x = 0; x < 4; ++x) {
                    int tmp0 = A[ii + x + 4][jj + 4];
                    int tmp1 = A[ii + x + 4][jj + 5];
                    int tmp2 = A[ii + x + 4][jj + 6];
                    int tmp3 = A[ii + x + 4][jj + 7];
                    B[jj + 4][ii + x + 4] = tmp0;
                    B[jj + 5][ii + x + 4] = tmp1;
                    B[jj + 6][ii + x + 4] = tmp2;
                    B[jj + 7][ii + x + 4] = tmp3;
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
