/* ************************************************************************
 * Copyright 2016 Advanced Micro Devices, Inc.
 *
 * ************************************************************************ */

#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "hipblas.hpp"
#include "utility.h"
#include "cblas_interface.h"
#include "norm.h"
#include "unit.h"
#include <complex.h>

using namespace std;

/* ============================================================================================ */

template<typename T>
hipblasStatus_t testing_dot(Arguments argus)
{

    int N = argus.N;
    int incx = argus.incx;
    int incy = argus.incy;

    hipblasStatus_t status = HIPBLAS_STATUS_SUCCESS;

    //argument sanity check, quick return if input parameters are invalid before allocating invalid memory
    if ( N < 0 ){
        status = HIPBLAS_STATUS_INVALID_VALUE;
        return status;
    }
    else if ( incx < 0 ){
        status = HIPBLAS_STATUS_INVALID_VALUE;
        return status;
    }
    else if ( incy < 0 ){
        status = HIPBLAS_STATUS_INVALID_VALUE;
        return status;
    }


    int sizeX = N * incx;
    int sizeY = N * incy;

    //Naming: dX is in GPU (device) memory. hK is in CPU (host) memory, plz follow this practice
    vector<T> hx(sizeX);
    vector<T> hy(sizeY);

    T cpu_result, rocblas_result;
    T *dx, *dy, *d_rocblas_result;
    int device_pointer = 1;

    double gpu_time_used, cpu_time_used;
    double rocblas_error;

    hipblasHandle_t handle;
    hipblasCreate(&handle);

    //allocate memory on device
    CHECK_HIP_ERROR(hipMalloc(&dx, sizeX * sizeof(T)));
    CHECK_HIP_ERROR(hipMalloc(&dy, sizeY * sizeof(T)));
    CHECK_HIP_ERROR(hipMalloc(&d_rocblas_result, sizeof(T)));

    //Initial Data on CPU
    srand(1);
    hipblas_init<T>(hx, 1, N, incx);
    hipblas_init<T>(hy, 1, N, incy);

    //copy data from CPU to device, does not work for incx != 1
    CHECK_HIP_ERROR(hipMemcpy(dx, hx.data(), sizeof(T)*N*incx, hipMemcpyHostToDevice));
    CHECK_HIP_ERROR(hipMemcpy(dy, hy.data(), sizeof(T)*N*incy, hipMemcpyHostToDevice));


    /* =====================================================================
         ROCBLAS
    =================================================================== */
     /* =====================================================================
                 CPU BLAS
     =================================================================== */
     //hipblasDot accept both dev/host pointer for the scalar
     if(device_pointer){
        status = hipblasDot<T>(handle,
                        N,
                        dx, incx,
                        dy, incy, d_rocblas_result);
    }
    else{
        status = hipblasDot<T>(handle,
                        N,
                        dx, incx,
                        dy, incy, &rocblas_result);
    }

    if (status != HIPBLAS_STATUS_SUCCESS) {
        CHECK_HIP_ERROR(hipFree(dx));
        CHECK_HIP_ERROR(hipFree(dy));
        CHECK_HIP_ERROR(hipFree(d_rocblas_result));
        hipblasDestroy(handle);
        return status;
    }

    if(device_pointer)    CHECK_HIP_ERROR(hipMemcpy(&rocblas_result, d_rocblas_result, sizeof(T), hipMemcpyDeviceToHost));


    if(argus.unit_check || argus.norm_check){

     /* =====================================================================
                 CPU BLAS
     =================================================================== */
        cblas_dot<T>(N,
                    hx.data(), incx,
                    hy.data(), incy, &cpu_result);


        if(argus.unit_check){
            unit_check_general<T>(1, 1, 1, &cpu_result, &rocblas_result);
        }

    }// end of if unit/norm check

//  BLAS_1_RESULT_PRINT


    CHECK_HIP_ERROR(hipFree(dx));
    CHECK_HIP_ERROR(hipFree(dy));
    CHECK_HIP_ERROR(hipFree(d_rocblas_result));
    hipblasDestroy(handle);
    return HIPBLAS_STATUS_SUCCESS;
}