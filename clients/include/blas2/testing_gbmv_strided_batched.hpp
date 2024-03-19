/* ************************************************************************
 * Copyright (C) 2016-2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ************************************************************************ */

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include "testing_common.hpp"

/* ============================================================================================ */

using hipblasGbmvStridedBatchedModel = ArgumentModel<e_a_type,
                                                     e_transA,
                                                     e_M,
                                                     e_N,
                                                     e_KL,
                                                     e_KU,
                                                     e_alpha,
                                                     e_lda,
                                                     e_incx,
                                                     e_beta,
                                                     e_incy,
                                                     e_stride_scale,
                                                     e_batch_count>;

inline void testname_gbmv_strided_batched(const Arguments& arg, std::string& name)
{
    hipblasGbmvStridedBatchedModel{}.test_name(arg, name);
}

template <typename T>
void testing_gbmv_strided_batched_bad_arg(const Arguments& arg)
{
    bool FORTRAN = arg.api == hipblas_client_api::FORTRAN;
    auto hipblasGbmvStridedBatchedFn
        = FORTRAN ? hipblasGbmvStridedBatched<T, true> : hipblasGbmvStridedBatched<T, false>;

    auto hipblasGbmvStridedBatchedFn_64 = arg.api == FORTRAN_64
                                              ? hipblasGbmvStridedBatched_64<T, true>
                                              : hipblasGbmvStridedBatched_64<T, false>;

    for(auto pointer_mode : {HIPBLAS_POINTER_MODE_HOST, HIPBLAS_POINTER_MODE_DEVICE})
    {
        hipblasLocalHandle handle(arg);
        CHECK_HIPBLAS_ERROR(hipblasSetPointerMode(handle, pointer_mode));

        hipblasOperation_t transA      = HIPBLAS_OP_N;
        int64_t            N           = 100;
        int64_t            M           = 100;
        int64_t            KL          = 5;
        int64_t            KU          = 5;
        int64_t            lda         = 100;
        int64_t            incx        = 1;
        int64_t            incy        = 1;
        int64_t            batch_count = 2;

        hipblasStride strideA = N * lda;
        hipblasStride stridex = N * incx;
        hipblasStride stridey = M * incy;

        device_vector<T> d_alpha(1), d_beta(1), d_one(1), d_zero(1);

        const T  h_alpha(1), h_beta(2), h_one(1), h_zero(0);
        const T* alpha = &h_alpha;
        const T* beta  = &h_beta;
        const T* one   = &h_one;
        const T* zero  = &h_zero;

        if(pointer_mode == HIPBLAS_POINTER_MODE_DEVICE)
        {
            CHECK_HIP_ERROR(hipMemcpy(d_alpha, alpha, sizeof(*alpha), hipMemcpyHostToDevice));
            CHECK_HIP_ERROR(hipMemcpy(d_beta, beta, sizeof(*beta), hipMemcpyHostToDevice));
            CHECK_HIP_ERROR(hipMemcpy(d_one, one, sizeof(*one), hipMemcpyHostToDevice));
            CHECK_HIP_ERROR(hipMemcpy(d_zero, zero, sizeof(*zero), hipMemcpyHostToDevice));
            alpha = d_alpha;
            beta  = d_beta;
            one   = d_one;
            zero  = d_zero;
        }

        device_vector<T> dA(strideA * batch_count);
        device_vector<T> dx(stridex * batch_count);
        device_vector<T> dy(stridey * batch_count);

        DAPI_EXPECT(HIPBLAS_STATUS_NOT_INITIALIZED,
                    hipblasGbmvStridedBatchedFn,
                    (nullptr,
                     transA,
                     M,
                     N,
                     KL,
                     KU,
                     alpha,
                     dA,
                     lda,
                     strideA,
                     dx,
                     incx,
                     stridex,
                     beta,
                     dy,
                     incy,
                     stridey,
                     batch_count));

        DAPI_EXPECT(HIPBLAS_STATUS_INVALID_ENUM,
                    hipblasGbmvStridedBatchedFn,
                    (handle,
                     (hipblasOperation_t)HIPBLAS_FILL_MODE_FULL,
                     M,
                     N,
                     KL,
                     KU,
                     alpha,
                     dA,
                     lda,
                     strideA,
                     dx,
                     incx,
                     stridex,
                     beta,
                     dy,
                     incy,
                     stridey,
                     batch_count));

        DAPI_EXPECT(HIPBLAS_STATUS_INVALID_VALUE,
                    hipblasGbmvStridedBatchedFn,
                    (handle,
                     transA,
                     M,
                     N,
                     KL,
                     KU,
                     nullptr,
                     dA,
                     lda,
                     strideA,
                     dx,
                     incx,
                     stridex,
                     beta,
                     dy,
                     incy,
                     stridey,
                     batch_count));

        DAPI_EXPECT(HIPBLAS_STATUS_INVALID_VALUE,
                    hipblasGbmvStridedBatchedFn,
                    (handle,
                     transA,
                     M,
                     N,
                     KL,
                     KU,
                     alpha,
                     dA,
                     lda,
                     strideA,
                     dx,
                     incx,
                     stridex,
                     nullptr,
                     dy,
                     incy,
                     stridey,
                     batch_count));

        if(pointer_mode == HIPBLAS_POINTER_MODE_HOST)
        {
            // For device mode in rocBLAS we don't have checks for dA, dx, dy as we may be able to quick return
            DAPI_EXPECT(HIPBLAS_STATUS_INVALID_VALUE,
                        hipblasGbmvStridedBatchedFn,
                        (handle,
                         transA,
                         M,
                         N,
                         KL,
                         KU,
                         alpha,
                         nullptr,
                         lda,
                         strideA,
                         dx,
                         incx,
                         stridex,
                         beta,
                         dy,
                         incy,
                         stridey,
                         batch_count));

            DAPI_EXPECT(HIPBLAS_STATUS_INVALID_VALUE,
                        hipblasGbmvStridedBatchedFn,
                        (handle,
                         transA,
                         M,
                         N,
                         KL,
                         KU,
                         alpha,
                         dA,
                         lda,
                         strideA,
                         nullptr,
                         incx,
                         stridex,
                         beta,
                         dy,
                         incy,
                         stridey,
                         batch_count));

            DAPI_EXPECT(HIPBLAS_STATUS_INVALID_VALUE,
                        hipblasGbmvStridedBatchedFn,
                        (handle,
                         transA,
                         M,
                         N,
                         KL,
                         KU,
                         alpha,
                         dA,
                         lda,
                         strideA,
                         dx,
                         incx,
                         stridex,
                         beta,
                         nullptr,
                         incy,
                         stridey,
                         batch_count));

            // rocBLAS implementation has alpha == 0 and beta == 1 quick return after arg checks, so if we're using 32-bit params,
            // this should fail with invalid-value as c_i32_overflow will rollover to -2147483648
            // Note: that this strategy can't check incx as rocBLAS supports negative. Also depends on implementation so not testing cuBLAS for now

            DAPI_EXPECT((arg.api & c_API_64) ? HIPBLAS_STATUS_SUCCESS
                                             : HIPBLAS_STATUS_INVALID_VALUE,
                        hipblasGbmvStridedBatchedFn,
                        (handle,
                         transA,
                         c_i32_overflow,
                         c_i32_overflow,
                         KL,
                         KU,
                         zero,
                         nullptr,
                         c_i32_overflow + 1,
                         strideA,
                         nullptr,
                         incx,
                         stridex,
                         one,
                         nullptr,
                         incy,
                         stridey,
                         c_i32_overflow));
        }

        // With M == 0 || N == 0, can have all nullptrs
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    0,
                    N,
                    KL,
                    KU,
                    nullptr,
                    nullptr,
                    lda,
                    strideA,
                    nullptr,
                    incx,
                    stridex,
                    nullptr,
                    nullptr,
                    incy,
                    stridey,
                    batch_count));
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    0,
                    KL,
                    KU,
                    nullptr,
                    nullptr,
                    lda,
                    strideA,
                    nullptr,
                    incx,
                    stridex,
                    nullptr,
                    nullptr,
                    incy,
                    stridey,
                    batch_count));
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    N,
                    KL,
                    KU,
                    nullptr,
                    nullptr,
                    lda,
                    strideA,
                    nullptr,
                    incx,
                    stridex,
                    nullptr,
                    nullptr,
                    incy,
                    stridey,
                    0));

        // With alpha == 0 can have x nullptr
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    N,
                    KL,
                    KU,
                    zero,
                    nullptr,
                    lda,
                    strideA,
                    nullptr,
                    incx,
                    stridex,
                    beta,
                    dy,
                    incy,
                    stridey,
                    batch_count));

        // With alpha == 0 && beta == 1, all other ptrs can be nullptr
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    N,
                    KL,
                    KU,
                    zero,
                    nullptr,
                    lda,
                    strideA,
                    nullptr,
                    incx,
                    stridex,
                    one,
                    nullptr,
                    incy,
                    stridey,
                    batch_count));
    }
}

template <typename T>
void testing_gbmv_strided_batched(const Arguments& arg)
{
    bool FORTRAN = arg.api == hipblas_client_api::FORTRAN;
    auto hipblasGbmvStridedBatchedFn
        = FORTRAN ? hipblasGbmvStridedBatched<T, true> : hipblasGbmvStridedBatched<T, false>;

    auto hipblasGbmvStridedBatchedFn_64 = arg.api == FORTRAN_64
                                              ? hipblasGbmvStridedBatched_64<T, true>
                                              : hipblasGbmvStridedBatched_64<T, false>;

    int64_t M            = arg.M;
    int64_t N            = arg.N;
    int64_t KL           = arg.KL;
    int64_t KU           = arg.KU;
    int64_t lda          = arg.lda;
    int64_t incx         = arg.incx;
    int64_t incy         = arg.incy;
    double  stride_scale = arg.stride_scale;
    int64_t batch_count  = arg.batch_count;

    hipblasStride stride_A = size_t(lda) * N * stride_scale;
    hipblasStride stride_x;
    hipblasStride stride_y;

    size_t A_size = stride_A * batch_count;
    size_t dim_x;
    size_t dim_y;

    hipblasOperation_t transA = char2hipblas_operation(arg.transA);

    if(transA == HIPBLAS_OP_N)
    {
        dim_x = N;
        dim_y = M;
    }
    else
    {
        dim_x = M;
        dim_y = N;
    }

    size_t abs_incx = incx >= 0 ? incx : -incx;
    size_t abs_incy = incy >= 0 ? incy : -incy;

    stride_x      = dim_x * abs_incx * stride_scale;
    stride_y      = dim_y * abs_incy * stride_scale;
    size_t X_size = stride_x * batch_count;
    size_t Y_size = stride_y * batch_count;

    hipblasLocalHandle handle(arg);

    // argument sanity check, quick return if input parameters are invalid before allocating invalid
    // memory
    bool invalid_size = M < 0 || N < 0 || lda < KL + KU + 1 || !incx || !incy || KL < 0 || KU < 0
                        || batch_count < 0;
    if(invalid_size || !M || !N || !batch_count)
    {
        DAPI_EXPECT(invalid_size ? HIPBLAS_STATUS_INVALID_VALUE : HIPBLAS_STATUS_SUCCESS,
                    hipblasGbmvStridedBatchedFn,
                    (handle,
                     transA,
                     M,
                     N,
                     KL,
                     KU,
                     nullptr,
                     nullptr,
                     lda,
                     stride_A,
                     nullptr,
                     incx,
                     stride_x,
                     nullptr,
                     nullptr,
                     incy,
                     stride_y,
                     batch_count));
        return;
    }

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA(A_size);
    host_vector<T> hx(X_size);
    host_vector<T> hy(Y_size);
    host_vector<T> hy_host(Y_size);
    host_vector<T> hy_device(Y_size);
    host_vector<T> hy_cpu(Y_size);

    device_vector<T> dA(A_size);
    device_vector<T> dx(X_size);
    device_vector<T> dy(Y_size);
    device_vector<T> d_alpha(1);
    device_vector<T> d_beta(1);

    double gpu_time_used, hipblas_error_host, hipblas_error_device;

    T h_alpha = arg.get_alpha<T>();
    T h_beta  = arg.get_beta<T>();

    // Initial Data on CPU
    hipblas_init_matrix(
        hA, arg, M, N, lda, stride_A, batch_count, hipblas_client_alpha_sets_nan, true);
    hipblas_init_vector(hx,
                        arg,
                        dim_x,
                        abs_incx,
                        stride_x,
                        batch_count,
                        hipblas_client_alpha_sets_nan,
                        false,
                        true);
    hipblas_init_vector(
        hy, arg, dim_y, abs_incy, stride_y, batch_count, hipblas_client_beta_sets_nan);

    // copy vector is easy in STL; hz = hy: save a copy in hz which will be output of CPU BLAS
    hy_cpu = hy;

    // copy data from CPU to device
    CHECK_HIP_ERROR(hipMemcpy(dA, hA.data(), sizeof(T) * A_size, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dx, hx.data(), sizeof(T) * X_size, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dy, hy.data(), sizeof(T) * Y_size, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(d_alpha, &h_alpha, sizeof(T), hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(d_beta, &h_beta, sizeof(T), hipMemcpyHostToDevice));

    if(arg.unit_check || arg.norm_check)
    {
        /* =====================================================================
            HIPBLAS
        =================================================================== */
        CHECK_HIPBLAS_ERROR(hipblasSetPointerMode(handle, HIPBLAS_POINTER_MODE_HOST));
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    N,
                    KL,
                    KU,
                    (T*)&h_alpha,
                    dA,
                    lda,
                    stride_A,
                    dx,
                    incx,
                    stride_x,
                    (T*)&h_beta,
                    dy,
                    incy,
                    stride_y,
                    batch_count));

        CHECK_HIP_ERROR(hipMemcpy(hy_host.data(), dy, sizeof(T) * Y_size, hipMemcpyDeviceToHost));
        CHECK_HIP_ERROR(hipMemcpy(dy, hy.data(), sizeof(T) * Y_size, hipMemcpyHostToDevice));

        CHECK_HIPBLAS_ERROR(hipblasSetPointerMode(handle, HIPBLAS_POINTER_MODE_DEVICE));
        DAPI_CHECK(hipblasGbmvStridedBatchedFn,
                   (handle,
                    transA,
                    M,
                    N,
                    KL,
                    KU,
                    d_alpha,
                    dA,
                    lda,
                    stride_A,
                    dx,
                    incx,
                    stride_x,
                    d_beta,
                    dy,
                    incy,
                    stride_y,
                    batch_count));

        CHECK_HIP_ERROR(hipMemcpy(hy_device.data(), dy, sizeof(T) * Y_size, hipMemcpyDeviceToHost));

        /* =====================================================================
           CPU BLAS
        =================================================================== */

        for(size_t b = 0; b < batch_count; b++)
        {
            ref_gbmv<T>(transA,
                        M,
                        N,
                        KL,
                        KU,
                        h_alpha,
                        hA.data() + b * stride_A,
                        lda,
                        hx.data() + b * stride_x,
                        incx,
                        h_beta,
                        hy_cpu.data() + b * stride_y,
                        incy);
        }

        // enable unit check, notice unit check is not invasive, but norm check is,
        // unit check and norm check can not be interchanged their order
        if(arg.unit_check)
        {
            unit_check_general<T>(1, dim_y, batch_count, abs_incy, stride_y, hy_cpu, hy_host);
            unit_check_general<T>(1, dim_y, batch_count, abs_incy, stride_y, hy_cpu, hy_device);
        }
        if(arg.norm_check)
        {
            hipblas_error_host = norm_check_general<T>(
                'F', 1, dim_y, abs_incy, stride_y, hy_cpu, hy_host, batch_count);
            hipblas_error_device = norm_check_general<T>(
                'F', 1, dim_y, abs_incy, stride_y, hy_cpu, hy_device, batch_count);
        }
    }

    if(arg.timing)
    {
        double gpu_time_used;
        CHECK_HIP_ERROR(hipMemcpy(dy, hy.data(), sizeof(T) * Y_size, hipMemcpyHostToDevice));
        hipStream_t stream;
        CHECK_HIPBLAS_ERROR(hipblasGetStream(handle, &stream));
        CHECK_HIPBLAS_ERROR(hipblasSetPointerMode(handle, HIPBLAS_POINTER_MODE_DEVICE));

        int runs = arg.cold_iters + arg.iters;
        for(int iter = 0; iter < runs; iter++)
        {
            if(iter == arg.cold_iters)
                gpu_time_used = get_time_us_sync(stream);

            DAPI_DISPATCH(hipblasGbmvStridedBatchedFn,
                          (handle,
                           transA,
                           M,
                           N,
                           KL,
                           KU,
                           d_alpha,
                           dA,
                           lda,
                           stride_A,
                           dx,
                           incx,
                           stride_x,
                           d_beta,
                           dy,
                           incy,
                           stride_y,
                           batch_count));
        }
        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        hipblasGbmvStridedBatchedModel{}.log_args<T>(std::cout,
                                                     arg,
                                                     gpu_time_used,
                                                     gbmv_gflop_count<T>(transA, M, N, KL, KU),
                                                     gbmv_gbyte_count<T>(transA, M, N, KL, KU),
                                                     hipblas_error_host,
                                                     hipblas_error_device);
    }
}
