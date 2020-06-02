/* ************************************************************************
 * Copyright 2016-2020 Advanced Micro Devices, Inc.
 *
 * ************************************************************************ */

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include "cblas_interface.h"
#include "flops.h"
#include "hipblas.hpp"
#include "norm.h"
#include "unit.h"
#include "utility.h"

using namespace std;

/* ============================================================================================ */

template <typename T>
hipblasStatus_t testing_spr2_batched(Arguments argus)
{
    bool FORTRAN = argus.fortran;
    auto hipblasSpr2BatchedFn = FORTRAN ? hipblasSpr2Batched<T, true> : hipblasSpr2Batched<T, false>;

    int               N           = argus.N;
    int               incx        = argus.incx;
    int               incy        = argus.incy;
    char              char_uplo   = argus.uplo_option;
    hipblasFillMode_t uplo        = char2hipblas_fill(char_uplo);
    int               batch_count = argus.batch_count;

    int abs_incx = incx < 0 ? -incx : incx;
    int abs_incy = incy < 0 ? -incy : incy;
    int A_size   = N * (N + 1) / 2;
    int x_size   = abs_incx * N;
    int y_size   = abs_incy * N;

    double gpu_time_used, cpu_time_used;
    double hipblasGflops, cblas_gflops, hipblasBandwidth;
    double rocblas_error;

    T alpha = argus.get_alpha<T>();

    hipblasStatus_t status = HIPBLAS_STATUS_SUCCESS;

    // argument sanity check, quick return if input parameters are invalid before allocating invalid
    // memory
    if(N < 0 || incx == 0 || incy == 0 || batch_count < 0)
        return HIPBLAS_STATUS_INVALID_VALUE;
    else if(batch_count == 0)
        return HIPBLAS_STATUS_SUCCESS;

    hipblasHandle_t handle;
    hipblasCreate(&handle);

    // Naming: dK is in GPU (device) memory. hK is in CPU (host) memory
    host_vector<T> hA[batch_count];
    host_vector<T> hA_cpu[batch_count];
    host_vector<T> hx[batch_count];
    host_vector<T> hy[batch_count];

    device_batch_vector<T> bA(batch_count, A_size);
    device_batch_vector<T> bx(batch_count, x_size);
    device_batch_vector<T> by(batch_count, y_size);

    device_vector<T*, 0, T> dA(batch_count);
    device_vector<T*, 0, T> dx(batch_count);
    device_vector<T*, 0, T> dy(batch_count);

    int last = batch_count - 1;
    if(!dA || !dx || !dy || (!bA[last] && A_size) || (!bx[last] && x_size) || (!by[last] && y_size))
    {
        hipblasDestroy(handle);
        return HIPBLAS_STATUS_ALLOC_FAILED;
    }

    // Initial Data on CPU
    srand(1);
    for(int b = 0; b < batch_count; b++)
    {
        hA[b]     = host_vector<T>(A_size);
        hA_cpu[b] = host_vector<T>(A_size);
        hx[b]     = host_vector<T>(x_size);
        hy[b]     = host_vector<T>(y_size);

        srand(1);
        hipblas_init<T>(hA[b], 1, A_size, 1);
        hipblas_init<T>(hx[b], 1, N, abs_incx);
        hipblas_init<T>(hy[b], 1, N, abs_incy);
        hA_cpu[b] = hA[b];

        CHECK_HIP_ERROR(hipMemcpy(bA[b], hA[b], sizeof(T) * A_size, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(bx[b], hx[b], sizeof(T) * x_size, hipMemcpyHostToDevice));
        CHECK_HIP_ERROR(hipMemcpy(by[b], hy[b], sizeof(T) * y_size, hipMemcpyHostToDevice));
    }
    CHECK_HIP_ERROR(hipMemcpy(dA, bA, sizeof(T*) * batch_count, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dx, bx, sizeof(T*) * batch_count, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dy, by, sizeof(T*) * batch_count, hipMemcpyHostToDevice));

    /* =====================================================================
           ROCBLAS
    =================================================================== */
    if(argus.timing)
    {
        gpu_time_used = get_time_us(); // in microseconds
    }

    for(int iter = 0; iter < 1; iter++)
    {
        status = hipblasSpr2BatchedFn(
            handle, uplo, N, (T*)&alpha, dx, incx, dy, incy, dA, batch_count);

        if(status != HIPBLAS_STATUS_SUCCESS)
        {
            hipblasDestroy(handle);
            return status;
        }
    }

    // copy output from device to CPU
    for(int b = 0; b < batch_count; b++)
    {
        hipMemcpy(hA[b], bA[b], sizeof(T) * A_size, hipMemcpyDeviceToHost);
    }

    if(argus.unit_check)
    {
        /* =====================================================================
           CPU BLAS
        =================================================================== */
        for(int b = 0; b < batch_count; b++)
        {
            cblas_spr2<T>(uplo, N, alpha, hx[b], incx, hy[b], incy, hA_cpu[b]);
        }

        // enable unit check, notice unit check is not invasive, but norm check is,
        // unit check and norm check can not be interchanged their order
        if(argus.unit_check)
        {
            unit_check_general<T>(1, A_size, batch_count, 1, hA, hA_cpu);
        }
    }

    hipblasDestroy(handle);
    return HIPBLAS_STATUS_SUCCESS;
}
