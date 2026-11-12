/**
 * @file test_algo_math.c
 * @brief Unit tests for cp_algo and cp_math modules.
 */

#include "cplus/cplus.h"
#include <assert.h>
#include <math.h>

static void test_algorithms(void) {
    printf("[TEST] Algorithms (IntroSort, RadixSort, Binary Search, Shuffle)...\n");

    /* 1. IntroSort i32 */
    i32 arr_i32[200];
    for (int i = 0; i < 200; i++) arr_i32[i] = 200 - i;
    AlgoSortI32(arr_i32, 200);
    for (int i = 0; i < 199; i++) {
        assert(arr_i32[i] <= arr_i32[i + 1]);
    }
    assert(arr_i32[0] == 1 && arr_i32[199] == 200);

    /* 2. Binary Search i32 */
    isize idx = AlgoBinarySearchI32(arr_i32, 200, 42);
    assert(idx == 41);
    assert(AlgoBinarySearchI32(arr_i32, 200, 999) == -1);
    CP_UNUSED(idx);

    /* 3. RadixSort u32 */
    u32 arr_u32[500];
    for (int i = 0; i < 500; i++) {
        arr_u32[i] = (u32)((500 - i) * 37 + (i % 11));
    }
    AlgoRadixSortU32(arr_u32, 500);
    for (int i = 0; i < 499; i++) {
        assert(arr_u32[i] <= arr_u32[i + 1]);
    }

    /* 4. Reverse */
    u32 rev_arr[5] = { 10, 20, 30, 40, 50 };
    AlgoReverse(rev_arr, 5, sizeof(u32));
    assert(rev_arr[0] == 50 && rev_arr[4] == 10);

    /* 5. Shuffle */
    u32 shuf_arr[50];
    for (int i = 0; i < 50; i++) shuf_arr[i] = (u32)i;
    AlgoShuffle(shuf_arr, 50, sizeof(u32));
    /* Verify all elements still exist */
    u32 sum = 0;
    for (int i = 0; i < 50; i++) sum += shuf_arr[i];
    assert(sum == (49 * 50) / 2);
    CP_UNUSED(sum);

    /* 6. Dot notation API */
    i32 dot_arr[10] = { 9, 3, 7, 1, 8, 2, 5, 4, 6, 0 };
    algo.sortI32(dot_arr, 10);
    assert(dot_arr[0] == 0 && dot_arr[9] == 9);
    assert(algo.bsearchI32(dot_arr, 10, 7) == 7);

    printf("  -> Algorithm tests PASSED.\n");
}

static void test_simd_linear_algebra(void) {
    printf("[TEST] Linear Algebra & SIMD Math (Vec2, Vec3, Mat4)...\n");

    /* Vectors */
    Vec2 v2a = cp_vec2(3.0f, 4.0f);
    assert(fabsf(cp_vec2_len(v2a) - 5.0f) < 1e-6f);
    CP_UNUSED(v2a);

    Vec3 v3a = cp_vec3(1.0f, 0.0f, 0.0f);
    Vec3 v3b = cp_vec3(0.0f, 1.0f, 0.0f);
    Vec3 cross = cp_vec3_cross(v3a, v3b);
    assert(fabsf(cross.x) < 1e-6f && fabsf(cross.y) < 1e-6f && fabsf(cross.z - 1.0f) < 1e-6f);
    CP_UNUSED(cross);

    /* Matrix multiplication */
    Mat4 I = Mat4Identity();
    Mat4 T = Mat4Translate(10.0f, 20.0f, 30.0f);
    Mat4 S = Mat4Scale(2.0f, 2.0f, 2.0f);

    /* Multiply Identity by T */
    Mat4 res = Mat4Mul(&I, &T);
    assert(fabsf(res.m[12] - 10.0f) < 1e-6f);
    assert(fabsf(res.m[13] - 20.0f) < 1e-6f);
    assert(fabsf(res.m[14] - 30.0f) < 1e-6f);
    CP_UNUSED(res);

    /* Mat4 * Vec4 */
    Vec4 p = cp_vec4(1.0f, 2.0f, 3.0f, 1.0f);
    Vec4 p_trans = Mat4MulVec4(&T, p);
    assert(fabsf(p_trans.x - 11.0f) < 1e-6f);
    assert(fabsf(p_trans.y - 22.0f) < 1e-6f);
    assert(fabsf(p_trans.z - 33.0f) < 1e-6f);
    CP_UNUSED(p_trans);

    /* Matrix Inversion */
    Mat4 TS = Mat4Mul(&T, &S);
    Mat4 inv_TS;
    b32 inv_ok = Mat4Invert(&TS, &inv_TS);
    assert(inv_ok == true);
    CP_UNUSED(inv_ok);

    /* Verify TS * inv_TS == Identity */
    Mat4 ident_check = Mat4Mul(&TS, &inv_TS);
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            f32 expected = (col == row) ? 1.0f : 0.0f;
            f32 diff = fabsf(ident_check.m[col * 4 + row] - expected);
            assert(diff < 1e-4f);
            CP_UNUSED(diff);
        }
    }

    /* Dot-notation API */
    Mat4 m_dot = math.translate(5.0f, -5.0f, 15.0f);
    Mat4 m_inv;
    assert(math.invert(&m_dot, &m_inv) == true);
    CP_UNUSED(m_dot);
    CP_UNUSED(m_inv);
    assert(fabsf(math.clamp(150.0f, 0.0f, 100.0f) - 100.0f) < 1e-6f);
    assert(fabsf(math.lerp(10.0f, 20.0f, 0.5f) - 15.0f) < 1e-6f);

    /* Test Fast Inverse Sqrt */
    f32 inv_val = MathFastInvSqrt(16.0f);
    assert(fabsf(inv_val - 0.25f) < 0.01f);
    assert(fabsf(math.fastInvSqrt(4.0f) - 0.5f) < 0.01f);
    CP_UNUSED(inv_val);

    /* Test Rotations, Transpose, Perspective, LookAt */
    Mat4 rx = Mat4RotateX(CP_DEG2RAD(90.0f));
    Mat4 ry = Mat4RotateY(CP_DEG2RAD(90.0f));
    Mat4 rz = Mat4RotateZ(CP_DEG2RAD(90.0f));
    CP_UNUSED(rx); CP_UNUSED(ry); CP_UNUSED(rz);

    Mat4 transp = Mat4Transpose(&T);
    assert(transp.m[3] == 10.0f && transp.m[7] == 20.0f && transp.m[11] == 30.0f);
    CP_UNUSED(transp);

    Mat4 persp = Mat4Perspective(CP_DEG2RAD(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    assert(persp.m[11] == -1.0f);
    CP_UNUSED(persp);

    Mat4 look = Mat4LookAt(cp_vec3(0, 0, 5), cp_vec3(0, 0, 0), cp_vec3(0, 1, 0));
    assert(fabsf(look.m[14] - (-5.0f)) < 1e-4f);
    CP_UNUSED(look);

    printf("  -> Linear Algebra & SIMD Math tests PASSED.\n");
}

int main(void) {
    printf("\n=== RUNNING CPLUS ALGORITHMS & SIMD MATH TESTS ===\n");
    test_algorithms();
    test_simd_linear_algebra();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY! ===\n\n");
    return 0;
}
