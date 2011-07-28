/***** GCL Generated File *********************/
/* Automatically generated file, do not edit! */
/**********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dispatch/dispatch.h>
#include <OpenCL/opencl.h>
#include <OpenCL/gcl_priv.h>
#include "gaussian_filter.cl.h"

static void initBlocks(void);

// Initialize static data structures
static block_kernel_pair pair_map[1] = {
      { NULL, NULL }
};

static block_kernel_map bmap = { 0, 1, initBlocks, pair_map };

// Block function
void (^gaussian_filter_kernel)(const cl_ndrange *ndrange, cl_image srcImg, cl_image dstImg, sampler_t sampler, cl_int width, cl_int height) =
^(const cl_ndrange *ndrange, cl_image srcImg, cl_image dstImg, sampler_t sampler, cl_int width, cl_int height) {
  int err = 0;
  cl_kernel k = bmap.map[0].kernel;
  if (!k) {
    initBlocks();
    k = bmap.map[0].kernel;
  }
  if (!k)
    gcl_log_fatal("kernel gaussian_filter does not exist for device");
  kargs_struct kargs;
  gclCreateArgsAPPLE(k, &kargs);
  err |= gclSetKernelArgMemAPPLE(k, 0, srcImg, &kargs);
  err |= gclSetKernelArgMemAPPLE(k, 1, dstImg, &kargs);
  cl_sampler sampler_obj;
  err |= gclSetKernelArgSamplerAPPLE(k, 2, &sampler, &kargs, &sampler_obj);
  err |= gclSetKernelArgAPPLE(k, 3, sizeof(width), &width, &kargs);
  err |= gclSetKernelArgAPPLE(k, 4, sizeof(height), &height, &kargs);
  gcl_log_cl_fatal(err, "setting argument for gaussian_filter failed");
  err = gclExecKernelAPPLE(k, ndrange, &kargs);
  gcl_log_cl_fatal(err, "Executing gaussian_filter failed");
  gclReleaseSampler(sampler_obj);
  gclDeleteArgsAPPLE(k, &kargs);
};

// Initialization functions
static void initBlocks(void) {
  const char* build_opts = " -cl-std=CL1.1";
  static dispatch_once_t once;
  dispatch_once(&once,
    ^{ int err = gclBuildProgramBinaryAPPLE("OpenCL/gaussian_filter.cl", "", &bmap, build_opts);
       if (!err) {
          assert(bmap.map[0].block_ptr == gaussian_filter_kernel && "mismatch block");
          bmap.map[0].kernel = clCreateKernel(bmap.program, "gaussian_filter", &err);
       }
     });
}

__attribute__((constructor))
static void RegisterMap(void) {
  gclRegisterBlockKernelMap(&bmap);
  bmap.map[0].block_ptr = gaussian_filter_kernel;
}

