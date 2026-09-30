#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpu.h"
#include "render_gmp.h"

/* Reference coordinates are never reconstructed from double coordinates. */
static unsigned int reference_orbit(mpf_srcptr left,mpf_srcptr top,mpf_srcptr step,
 unsigned int limit,float *reference) {
 mpf_t cr,ci,x,y,xx,yy,t,sum;
 mpf_inits(cr,ci,x,y,xx,yy,t,sum,NULL);
 mpf_mul_ui(t,step,320); mpf_add(cr,left,t);
 mpf_mul_ui(t,step,240); mpf_sub(ci,top,t);
 unsigned int count=1;
 for(unsigned int n=0;n<limit;n++) {
  mpf_mul(t,x,y); mpf_mul_ui(t,t,2); mpf_add(y,t,ci);
  mpf_sub(x,xx,yy); mpf_add(x,x,cr);
  mpf_mul(xx,x,x); mpf_mul(yy,y,y); mpf_add(sum,xx,yy);
  float *p=reference+count*4;
  p[0]=(float)mpf_get_d(x); p[1]=(float)mpf_get_d(y);
  p[2]=8.0f*FLT_EPSILON*(fabsf(p[0])+fabsf(p[1]))+32.0f*FLT_MIN;
  count++;
  /* An escaping reference must not grow until it overflows. Remaining pixels
     whose orbits outlive this reference are explicitly repaired with GMP. */
  if(mpf_cmp_ui(sum,256)>0) break;
 }
 mpf_clears(cr,ci,x,y,xx,yy,t,sum,NULL);
 return count;
}

const char *gpu_perturb(mpf_srcptr left,mpf_srcptr top,mpf_srcptr step,
 unsigned int limit,unsigned char *image,void (*row_ready)(unsigned int,const unsigned char *)) {
 float pixel_step=(float)mpf_get_d(step);
 /* No exponent rescaling yet: leave a wide margin above FP32 underflow. */
 if(!isfinite(pixel_step) || pixel_step<1.0e-30f) {
  fprintf(stderr,"perturb: pixel step outside supported FP32 exponent range; using GMP\n");
  return NULL;
 }
 cl_platform_id platforms[16]; cl_uint count=0; cl_device_id device=NULL;
 if(clGetPlatformIDs(16,platforms,&count)!=CL_SUCCESS) return NULL;
 for(cl_uint i=0;i<count && i<16;i++)
  if(clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_GPU,1,&device,NULL)==CL_SUCCESS) break;
 if(!device) return NULL;
 FILE *file=fopen("perturb.cl","rb");
 if(!file) return NULL;
 char source[8192]; size_t length=fread(source,1,sizeof source,file); fclose(file);
 if(!length || length==sizeof source) return NULL;
 float *reference=calloc((limit+1)*4,sizeof(float));
 unsigned char *mask=malloc(640*480);
 if(!reference || !mask) { free(reference); free(mask); return NULL; }
 unsigned int reference_count=reference_orbit(left,top,step,limit,reference);
 const char *text=source,*backend=NULL;
 cl_int error;
 cl_context context=clCreateContext(NULL,1,&device,NULL,NULL,&error);
 cl_command_queue queue=NULL; cl_program program=NULL; cl_kernel kernel=NULL;
 cl_mem orbit=NULL,buffer=NULL,flags=NULL;
 unsigned int repaired=0;
 if(!context || error!=CL_SUCCESS) goto cleanup;
 queue=clCreateCommandQueue(context,device,0,&error);
 if(!queue || error!=CL_SUCCESS) goto cleanup;
 program=clCreateProgramWithSource(context,1,&text,&length,&error);
 if(!program || error!=CL_SUCCESS) goto cleanup;
 if(clBuildProgram(program,1,&device,"",NULL,NULL)!=CL_SUCCESS) {
  char log[4096]={0}; clGetProgramBuildInfo(program,device,CL_PROGRAM_BUILD_LOG,sizeof(log)-1,log,NULL);
  fprintf(stderr,"Perturbation OpenCL build failed: %s\n",log); goto cleanup;
 }
 kernel=clCreateKernel(program,"perturb",&error);
 if(!kernel || error!=CL_SUCCESS) goto cleanup;
 orbit=clCreateBuffer(context,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,reference_count*4*sizeof(float),reference,&error);
 if(!orbit || error!=CL_SUCCESS) goto cleanup;
 buffer=clCreateBuffer(context,CL_MEM_WRITE_ONLY,640*480*3,NULL,&error);
 if(!buffer || error!=CL_SUCCESS) goto cleanup;
 flags=clCreateBuffer(context,CL_MEM_WRITE_ONLY,640*480,NULL,&error);
 if(!flags || error!=CL_SUCCESS) goto cleanup;
 if(clSetKernelArg(kernel,0,sizeof orbit,&orbit)!=CL_SUCCESS ||
    clSetKernelArg(kernel,1,sizeof reference_count,&reference_count)!=CL_SUCCESS ||
    clSetKernelArg(kernel,2,sizeof pixel_step,&pixel_step)!=CL_SUCCESS ||
    clSetKernelArg(kernel,3,sizeof limit,&limit)!=CL_SUCCESS ||
    clSetKernelArg(kernel,4,sizeof buffer,&buffer)!=CL_SUCCESS ||
    clSetKernelArg(kernel,5,sizeof flags,&flags)!=CL_SUCCESS) goto cleanup;
 for(unsigned int y=0;y<480;y+=8) {
  render_turn();
  size_t offset=(size_t)y*640,work=8*640;
  if(clEnqueueNDRangeKernel(queue,kernel,1,&offset,&work,NULL,0,NULL,NULL)!=CL_SUCCESS) goto cleanup;
  if(clEnqueueReadBuffer(queue,buffer,CL_TRUE,offset*3,work*3,image+offset*3,0,NULL,NULL)!=CL_SUCCESS ||
     clEnqueueReadBuffer(queue,flags,CL_TRUE,offset,work,mask+offset,0,NULL,NULL)!=CL_SUCCESS) goto cleanup;
  unsigned int needs_repair=0;
  for(size_t i=offset;i<offset+work;i++) needs_repair+=mask[i]!=0;
  if(needs_repair) repaired+=gmp_rows(left,top,step,limit,image,mask,y,8,row_ready);
  else if(row_ready) for(unsigned int row=y;row<y+8;row++) row_ready(row,image+row*640*3);
 }
 fprintf(stderr,"perturb: reference=%u steps; GMP repairs=%u/307200 pixels\n",reference_count-1,repaired);
 backend=repaired?"opencl-perturb-gmp":"opencl-perturb";
cleanup:
 if(flags) clReleaseMemObject(flags);
 if(buffer) clReleaseMemObject(buffer);
 if(orbit) clReleaseMemObject(orbit);
 if(kernel) clReleaseKernel(kernel);
 if(program) clReleaseProgram(program);
 if(queue) clReleaseCommandQueue(queue);
 if(context) clReleaseContext(context);
 free(mask); free(reference);
 return backend;
}
