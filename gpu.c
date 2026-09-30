#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpu.h"
#include "render_gmp.h"

const char *gpu_render(double left,double top,double step,unsigned int limit,unsigned char *image,void (*row_ready)(unsigned int,const unsigned char *)) {
 cl_platform_id platforms[16]; cl_uint count=0;
 cl_device_id device=NULL;
 if(clGetPlatformIDs(16,platforms,&count)!=CL_SUCCESS) return NULL;
 for(cl_uint i=0;i<count && i<16;i++)
  if(clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_GPU,1,&device,NULL)==CL_SUCCESS) break;
 if(!device) return NULL;
 cl_device_fp_config fp64=0;
 clGetDeviceInfo(device,CL_DEVICE_DOUBLE_FP_CONFIG,sizeof fp64,&fp64,NULL);
 /* Keep at least 64 representable coordinate steps per pixel. This guards
    coordinate resolution, not accumulated iteration error near the boundary. */
 double scale=fmax(1.0,fmax(fabs(left),fabs(top)));
 double epsilon=fp64?DBL_EPSILON:FLT_EPSILON;
 if(!isfinite(step) || step <= 64*epsilon*scale) return NULL;
 FILE *file=fopen("mandelbrot.cl","rb");
 if(!file) return NULL;
 char source[8192]; size_t length=fread(source,1,sizeof source,file); fclose(file);
 if(!length || length==sizeof source) return NULL;
 const char *text=source; const char *backend=NULL;
 cl_int error;
 cl_context context=clCreateContext(NULL,1,&device,NULL,NULL,&error);
 if(!context) return NULL;
 cl_command_queue queue=NULL; cl_program program=NULL; cl_kernel kernel=NULL; cl_mem buffer=NULL;
 queue=clCreateCommandQueue(context,device,0,&error);
 if(!queue || error!=CL_SUCCESS) goto cleanup;
 program=clCreateProgramWithSource(context,1,&text,&length,&error);
 if(!program || error!=CL_SUCCESS) goto cleanup;
 error=clBuildProgram(program,1,&device,fp64?"-DUSE_DOUBLE":"",NULL,NULL);
 if(error!=CL_SUCCESS) {
  char log[4096]={0}; clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL);
  fprintf(stderr,"OpenCL build failed: %s\n",log); goto cleanup;
 }
 kernel=clCreateKernel(program,"mandelbrot",&error);
 if(!kernel || error!=CL_SUCCESS) goto cleanup;
 buffer=clCreateBuffer(context,CL_MEM_WRITE_ONLY,640*480*3,NULL,&error);
 if(!buffer || error!=CL_SUCCESS) goto cleanup;
 double coords[]={left,top,step};
 for(cl_uint i=0;i<3;i++) {
  float value=(float)coords[i];
  error=clSetKernelArg(kernel,i,fp64?sizeof(double):sizeof(float),fp64?(void*)&coords[i]:(void*)&value);
  if(error!=CL_SUCCESS) goto cleanup;
 }
 if(clSetKernelArg(kernel,3,sizeof limit,&limit)!=CL_SUCCESS ||
    clSetKernelArg(kernel,4,sizeof buffer,&buffer)!=CL_SUCCESS) goto cleanup;
 unsigned int rows=row_ready?8:480;
 for(unsigned int y=0;y<480;y+=rows) {
  render_turn();
  size_t offset=(size_t)y*640,work=(size_t)rows*640;
  if(clEnqueueNDRangeKernel(queue,kernel,1,&offset,&work,NULL,0,NULL,NULL)!=CL_SUCCESS) goto cleanup;
  if(clEnqueueReadBuffer(queue,buffer,CL_TRUE,offset*3,work*3,image+offset*3,0,NULL,NULL)!=CL_SUCCESS) goto cleanup;
  if(row_ready) for(unsigned int row=y;row<y+rows;row++) row_ready(row,image+row*640*3);
 }
 backend=fp64?"opencl-fp64":"opencl-fp32";
cleanup:
 if(buffer) clReleaseMemObject(buffer);
 if(kernel) clReleaseKernel(kernel);
 if(program) clReleaseProgram(program);
 if(queue) clReleaseCommandQueue(queue);
 clReleaseContext(context);
 return backend;
}
