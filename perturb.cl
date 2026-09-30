#pragma OPENCL FP_CONTRACT OFF
#define EPS 1.1920928955078125e-7f
#define TINY 1.1754943508222875e-38f

__kernel void perturb(__global const float4 *reference, uint reference_count,
 float step, uint limit, __global uchar *image, __global uchar *repair) {
 size_t i=get_global_id(0);
 float2 dc=(float2)(((float)(i%640)-319.5f)*step,
                    (239.5f-(float)(i/640))*step);
 float dc_size=fabs(dc.x)+fabs(dc.y);
 float dc_error=8.0f*EPS*dc_size+32.0f*TINY;
 float2 delta=(float2)(0.0f);
 float error=0.0f;
 uint n;
 repair[i]=0;
 for(n=0;n<limit;n++) {
  if(n+1>=reference_count) { repair[i]=1; return; }
  float4 ref=reference[n], next=reference[n+1];
  float d=fabs(delta.x)+fabs(delta.y), z=fabs(ref.x)+fabs(ref.y);
  // Conservative L1 error propagation for 2*Z*delta + delta^2 + dc.
  float next_error=2.0f*(z+d)*error+error*error
    +2.0f*ref.z*(d+error)+dc_error
    +32.0f*EPS*(2.0f*z*d+d*d+dc_size)+32.0f*TINY;
  next_error*=1.00001f;
  float2 updated=(float2)(
    2.0f*(ref.x*delta.x-ref.y*delta.y)+delta.x*delta.x-delta.y*delta.y+dc.x,
    2.0f*(ref.x*delta.y+ref.y*delta.x)+2.0f*delta.x*delta.y+dc.y);
  delta=updated;
  error=next_error;
  float2 actual=next.xy+delta;
  float total_error=error+next.z+4.0f*EPS*(fabs(next.x)+fabs(next.y)+fabs(delta.x)+fabs(delta.y))+32.0f*TINY;
  float norm=actual.x*actual.x+actual.y*actual.y;
  float uncertainty=2.0f*(fabs(actual.x)+fabs(actual.y))*total_error
    +2.0f*total_error*total_error+8.0f*EPS*(norm+1.0f);
  // Cancellation glitches, overflow, or an ambiguous escape test go to GMP.
  if(!isfinite(norm) || !isfinite(uncertainty) || total_error>0.001f ||
     norm<1.0e-6f*(next.x*next.x+next.y*next.y)) {
   repair[i]=1; return;
  }
  if(norm-uncertainty>4.0f) break;
  if(norm+uncertainty>=4.0f) { repair[i]=1; return; }
 }
 if(n==limit) { image[i*3]=image[i*3+1]=image[i*3+2]=0; }
 else {
  uint c=n*9%768,f=c%256,k=c/256;
  image[i*3]=k==0?f:k==1?255-f:0;
  image[i*3+1]=k==1?f:k==2?255-f:0;
  image[i*3+2]=k==2?f:k==0?255-f:0;
 }
}
