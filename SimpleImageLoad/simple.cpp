//
//  main.cpp
//  Simple
//
//  Created by Beau Johnston on 25/07/11.
//  Copyright 2011 University Of New England. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include "openCLUtilities.h"


// If more than one platform installed then set this to pick which
// one to use
#define PLATFORM_INDEX 0
#define NUM_BUFFER_ELEMENTS 10 

cl_int errNum;
cl_uint numPlatforms;
cl_uint numDevices;
cl_platform_id * platformIDs;
cl_device_id * deviceIDs;
cl_context context;
cl_program program;
//std::vector<cl_kernel> kernels;
//std::vector<cl_command_queue> queues;
//std::vector<cl_mem> imageObjects; // device memory used for the input/output array
cl_mem inputImage, outputImage;

cl_sampler sampler;
cl_kernel kernel;                   // compute kernel
cl_command_queue commands;          // compute command queue
int width;
int height;                  //input and output image specs

void cleanKill(int errNumber){
    clReleaseMemObject(inputImage);
	clReleaseMemObject(outputImage);
	clReleaseProgram(program);
    clReleaseSampler(sampler);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);
    exit(errNumber);
}

// main() for simple buffer and sub-buffer example
//
int main(int argc, char** argv)
{
    
    std::cout << "Simple Image Processing Example" << std::endl;
    
    
    // First, select an OpenCL platform to run on.
    errNum = clGetPlatformIDs(0, NULL, &numPlatforms);
    checkErr(
             (errNum != CL_SUCCESS) ?
             errNum : (numPlatforms <= 0 ? -1 : CL_SUCCESS),
             "clGetPlatformIDs");
    platformIDs = (cl_platform_id *)alloca(sizeof(cl_platform_id) * numPlatforms);
    std::cout << "Number of platforms: \t" << numPlatforms << std::endl;
    errNum = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
    checkErr(
             (errNum != CL_SUCCESS) ?
             errNum : (numPlatforms <= 0 ? -1 : CL_SUCCESS),
             "clGetPlatformIDs");
    std::ifstream srcFile("gaussian_filter.cl");
    
    checkErr(srcFile.is_open() ? CL_SUCCESS : -1, "reading simple.cl");
    
    std::string srcProg(
                        std::istreambuf_iterator<char>(srcFile),
                        (std::istreambuf_iterator<char>()));
    const char * src = srcProg.c_str();
    size_t length = srcProg.length();
    deviceIDs = NULL;
    DisplayPlatformInfo(
                        platformIDs[PLATFORM_INDEX],
                        CL_PLATFORM_VENDOR,
                        "CL_PLATFORM_VENDOR");
    errNum = clGetDeviceIDs(
                            platformIDs[PLATFORM_INDEX],
                            CL_DEVICE_TYPE_ALL,
                            0,
                            NULL,
                            &numDevices);
    if (errNum != CL_SUCCESS && errNum != CL_DEVICE_NOT_FOUND){
        checkErr(errNum, "clGetDeviceIDs");
    }
    
    deviceIDs = (cl_device_id *)alloca(sizeof(cl_device_id) * numDevices);
    errNum = clGetDeviceIDs(
                            platformIDs[PLATFORM_INDEX],
                            CL_DEVICE_TYPE_ALL,
                            numDevices,
                            &deviceIDs[0],
                            NULL);
    checkErr(errNum, "clGetDeviceIDs");
    
    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)platformIDs[PLATFORM_INDEX],
        0
    };
    
    context = clCreateContext(
                              contextProperties,
                              numDevices,
                              deviceIDs,
                              NULL,
                              NULL,
                              &errNum);
    
    checkErr(errNum, "clCreateContext");
    // Create program from source
    program = clCreateProgramWithSource(
                                        context,
                                        1,
                                        &src,
                                        &length,
                                        &errNum);
    checkErr(errNum, "clCreateProgramWithSource");
    
    // Build program
    errNum = clBuildProgram(
                            program,
                            numDevices,
                            deviceIDs,
                            "-I.",
                            NULL,
                            NULL);

    if (errNum != CL_SUCCESS){
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(
                              program,
                              deviceIDs[0],
                              CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog),
                              buildLog,
                              NULL);
        std::cerr << "Error in OpenCL C source: " << std::endl;
        std::cerr << buildLog;
        checkErr(errNum, "clBuildProgram");
    }
    
    // Create a command commands
	//
	if(!(commands = clCreateCommandQueue(context, deviceIDs[0], 0, &errNum))) {
        std::cout << "Failed to create a command commands!" << std::endl;
        cleanKill(EXIT_FAILURE);
    }
    
    cl_kernel kernel = clCreateKernel(program, "gaussian_filter", &errNum);
    checkErr(errNum, "clCreateKernel(gaussian_filter)");

    if(!doesGPUSupportImageObjects){
        cleanKill(EXIT_FAILURE);
    }
    
    inputImage = LoadImage(context, (char*)"rgba.png", width, height);
        
    cl_image_format format; 
    format.image_channel_order = CL_RGBA; 
    format.image_channel_data_type = CL_UNORM_INT8;
    
    outputImage = clCreateImage2D(context, 
                             CL_MEM_WRITE_ONLY, 
                             &format, 
                             width, 
                             height,
                             0, 
                             NULL, 
                             &errNum);
    
    if(there_was_an_error(errNum)){
        std::cout << "Output Image Buffer creation error!" << std::endl;
        cleanKill(EXIT_FAILURE);
    }    
    
	if (!inputImage || !outputImage ){
        std::cout << "Failed to allocate device memory!" << std::endl;
        cleanKill(EXIT_FAILURE);
	}
    
    char *buffer = new char [width * height * 4];
    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { width, height, 1};

    sampler = clCreateSampler(context,
                              CL_FALSE, // Non-normalized coordinates 
                              CL_ADDRESS_CLAMP_TO_EDGE, 
                              CL_FILTER_NEAREST, 
                              &errNum);
    
    if(there_was_an_error(errNum)){
        std::cout << "Error creating CL sampler object." << std::endl;
        cleanKill(EXIT_FAILURE);
    }
    
    // Set the kernel arguments
    errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputImage);
    errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputImage);
    errNum |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler);
    errNum |= clSetKernelArg(kernel, 3, sizeof(cl_int), &width);
    errNum |= clSetKernelArg(kernel, 4, sizeof(cl_int), &height);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        std::cerr << print_cl_errstring(errNum) << std::endl;
        cleanKill(EXIT_FAILURE);
    }
    
    //errNum = clGetKernelWorkGroupInfo(kernel, deviceIDs, CL_KERNEL_WORK_GROUP_SIZE, sizeof(unsigned short)* height*width*4, &local, NULL);
    
//	if (errNum != CL_SUCCESS)
//	{
//        cout << print_cl_errstring(err) << endl;
//        if(err == CL_INVALID_VALUE){
//            cout << "if param_name is not valid, or if size in bytes specified by param_value_size "
//            << "is less than the size of return type as described in the table above and "
//            << "param_value is not NULL." << endl;
//        }
//		cout << "Error: Failed to retrieve kernel work group info!" << err << endl;
//		cleanKill(EXIT_FAILURE);
//	}
    
    std::cout << "Max work group size is " << CL_DEVICE_MAX_WORK_GROUP_SIZE << std::endl;
    std::cout << "Max work item size is " << CL_DEVICE_MAX_WORK_ITEM_SIZES << std::endl;
    
    size_t localWorkSize[2];
    size_t globalWorkSize[2];
    
    localWorkSize[0] = 1;
    localWorkSize[1] = localWorkSize[0];
    globalWorkSize[0] = width*height;
    globalWorkSize[1] = globalWorkSize[0];
    
    //CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of work-items specified by global_work_size is not evenly divisable by size of work-group given by local_work_size
    
    
    
        //size_t globalWorkSize[2] =  { RoundUp(localWorkSize[0], width), RoundUp(localWorkSize[1], height)};

//    size_t globalWorkSize[1] = {sizeof(unsigned short)* height * width};
//	size_t localWorkSize[1] = {64};
    
    // Queue the kernel up for execution
    errNum = clEnqueueNDRangeKernel(commands, kernel, 2, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    
    if (errNum != CL_SUCCESS){
        std::cerr << "Error queuing kernel for execution." << std::endl;
        std::cerr << print_cl_errstring(errNum) << std::endl;
        cleanKill(EXIT_FAILURE);
    }
    
    // Wait for the command commands to get serviced before reading back results
	//
	clFinish(commands);
    
    // Read back computed data
    errNum = clEnqueueReadImage(commands, outputImage,
                                     CL_TRUE, origin, region, 0, 0, buffer, 0, NULL, NULL);
    
    SaveImage((char*)"outRGBA.png", (char*)buffer, width, height);

    std::cout << "Program completed successfully" << std::endl;        
    return 0;     
}
            