#ifdef USE_DOUBLE
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
typedef double real;
#else
typedef float real;
#endif
#pragma OPENCL FP_CONTRACT OFF
__kernel void mandelbrot(real left, real top, real step, uint limit, __global uchar *image) {
 size_t i=get_global_id(0);
 real cr=left+((real)(i%640)+(real)0.5)*step;
 real ci=top-((real)(i/640)+(real)0.5)*step;
 real x=0,y=0;
 uint n;
 for(n=0;n<limit;n++) {
  real xx=x*x, yy=y*y;
  y=((real)2*x)*y+ci; x=xx-yy+cr;
  if(x*x+y*y>(real)4) break;
 }
 if(n==limit) { image[i*3]=image[i*3+1]=image[i*3+2]=0; }
 else {
  uint c=n*9%768,f=c%256,k=c/256;
  image[i*3]=k==0?f:k==1?255-f:0;
  image[i*3+1]=k==1?f:k==2?255-f:0;
  image[i*3+2]=k==2?f:k==0?255-f:0;
 }
}
