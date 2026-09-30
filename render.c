#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "render_gmp.h"
#ifdef USE_OPENCL
#include "gpu.h"
#endif
#define W 640
#define H 480
static int scheduled=0;
void render_turn(void) {
 if(!scheduled) return;
 if(fputs("schedule=next\n",stderr)==EOF || fflush(stderr)!=0 || getchar()!='1') _Exit(1);
}
static void emit_row(unsigned int y,const unsigned char *row) {
 unsigned char header[4]={(unsigned char)(y>>24),(unsigned char)(y>>16),(unsigned char)(y>>8),(unsigned char)y};
 /* A frame must stay intact even when several OpenMP workers finish together. */
 #pragma omp critical(row_output)
 {
  if(fwrite(header,1,4,stdout)!=4 || fwrite(row,1,W*3,stdout)!=W*3 || fflush(stdout)!=0) _Exit(1);
 }
}
unsigned int gmp_rows(mpf_srcptr left,mpf_srcptr top,mpf_srcptr step,
 unsigned long limit,unsigned char *image,const unsigned char *mask,
 unsigned int first,unsigned int rows,void (*row_ready)(unsigned int,const unsigned char *)) {
 unsigned int computed=0;
 #pragma omp parallel default(none) shared(left,top,step,image,limit,mask,first,rows,row_ready,scheduled) reduction(+:computed)
 {
  /* GMP operands written by a worker must never be shared with other workers. */
  mpf_t cr,ci,x,y,xx,yy,t,sum;
  mpf_inits(cr,ci,x,y,xx,yy,t,sum,NULL);
  /* Scheduled jobs share one row across cores; standalone jobs share rows. */
  for(unsigned int batch=0;batch<(scheduled?rows:1);batch++) {
  if(scheduled) {
   #pragma omp single
   { if(!mask) render_turn(); } /* Repairs already own the GPU block's turn. */
  }
  #pragma omp for schedule(dynamic,1)
  for(unsigned long work=0;work<(scheduled?W:rows);work++) {
  unsigned long py=first+(scheduled?batch:work);
  mpf_mul_ui(t,step,2*py+1); mpf_div_ui(t,t,2); mpf_sub(ci,top,t);
  for(unsigned long px=scheduled?work:0;px<(scheduled?work+1:W);px++) {
   if(mask && !mask[py*W+px]) continue;
   computed++;
   mpf_mul_ui(t,step,2*px+1); mpf_div_ui(t,t,2); mpf_add(cr,left,t);
   mpf_set_ui(x,0); mpf_set_ui(y,0);
   mpf_set_ui(xx,0); mpf_set_ui(yy,0);
   unsigned long n;
   /* Carry the escape-test squares into the next iteration. */
   for(n=0;n<limit;n++) {
    /*
     * mpf_mul(t,x,y); mpf_mul_ui(t,t,2); mpf_add(y,t,ci);
     */
    mpf_mul(t,x,y); mpf_add(t,t,t); mpf_add(y,t,ci);
    mpf_sub(x,xx,yy); mpf_add(x,x,cr);
    mpf_mul(xx,x,x); mpf_mul(yy,y,y); mpf_add(sum,xx,yy);
    if(mpf_cmp_ui(sum,4)>0) break;
   }
   unsigned char *p=image+(py*W+px)*3;
   if(n==limit) p[0]=p[1]=p[2]=0;
   else { unsigned long c=n*9%768,f=c%256,k=c/256;
    p[0]=k==0?f:k==1?255-f:0;
    p[1]=k==1?f:k==2?255-f:0;
    p[2]=k==2?f:k==0?255-f:0;
   }
  }
  if(!scheduled && row_ready) row_ready((unsigned int)py,image+py*W*3);
  }
  if(scheduled) {
   #pragma omp single
   { if(row_ready) row_ready(first+batch,image+(first+batch)*W*3); }
  }
  }
  mpf_clears(cr,ci,x,y,xx,yy,t,sum,NULL);
 }
 return computed;
}
static long double mpf_to_long_double(mpf_srcptr value) {
 /* Do not pass through mpf_get_d: that would discard the extra mantissa bits. */
 char text[128];
 int length=gmp_snprintf(text,sizeof text,"%.*Fe",LDBL_DECIMAL_DIG+8,value);
 if(length<0 || (size_t)length>=sizeof text) return NAN;
 return strtold(text,NULL);
}

static int long_double_rows(mpf_srcptr left,mpf_srcptr top,mpf_srcptr step,
 unsigned long limit,unsigned char *image,void (*row_ready)(unsigned int,const unsigned char *)) {
 const long double l=mpf_to_long_double(left),t=mpf_to_long_double(top),s=mpf_to_long_double(step);
 const long double scale=fmaxl(1.0L,fmaxl(fabsl(l),fmaxl(fabsl(t),fmaxl(fabsl(l+W*s),fabsl(t-H*s)))));
 fprintf(stderr,"long double: %d mantissa bits, %zu storage bytes\n",LDBL_MANT_DIG,sizeof(long double));
 if(!isfinite(l) || !isfinite(t) || !isfinite(s) || s<=64.0L*LDBL_EPSILON*scale) {
  fprintf(stderr,"long double: insufficient coordinate resolution; using GMP\n");
  return 0;
 }
 #pragma omp parallel default(none) shared(l,t,s,limit,image,row_ready,scheduled)
 {
 for(unsigned int batch=0;batch<(scheduled?H:1);batch++) {
  if(scheduled) {
   #pragma omp single
   { render_turn(); }
  }
  #pragma omp for schedule(dynamic,1)
  for(unsigned int work=0;work<(scheduled?W:H);work++) {
  unsigned int py=scheduled?batch:work;
  const long double ci=t-((long double)py+0.5L)*s;
  for(unsigned int px=scheduled?work:0;px<(scheduled?work+1:W);px++) {
   const long double cr=l+((long double)px+0.5L)*s;
   long double x=0.0L,y=0.0L,xx=0.0L,yy=0.0L;
   unsigned long n;
   for(n=0;n<limit;n++) {
    y=2.0L*(x*y)+ci;
    x=xx-yy+cr;
    xx=x*x; yy=y*y;
    if(xx+yy>4.0L) break;
   }
   unsigned char *p=image+(py*W+px)*3;
   if(n==limit) p[0]=p[1]=p[2]=0;
   else {
    unsigned long c=n*9%768,f=c%256,k=c/256;
    p[0]=k==0?f:k==1?255-f:0;
    p[1]=k==1?f:k==2?255-f:0;
    p[2]=k==2?f:k==0?255-f:0;
   }
  }
  if(!scheduled && row_ready) row_ready(py,image+py*W*3);
  }
  if(scheduled) {
   #pragma omp single
   { if(row_ready) row_ready(batch,image+batch*W*3); }
  }
 }
 }
 return 1;
}

int main(int argc,char **argv) {
 if(argc<2 || (argc-2)%3 || argc>242) return 2;
 const char *schedule_env=getenv("RENDER_SCHEDULED");
 scheduled=schedule_env && strcmp(schedule_env,"1")==0;
 render_turn();
 unsigned long limit=strtoul(argv[1],0,10);
 if(limit<50 || limit>100000) return 2;
 mpf_set_default_prec(128+20*((argc-2)/3));
 mpf_t left,top,span,t,step;
 mpf_inits(left,top,span,t,step,NULL);
 mpf_set_str(left,"-2.5",10); mpf_set_str(top,"1.3125",10); mpf_set_str(span,"3.5",10);
 for(int a=2;a<argc;a+=3) {
  unsigned long u=strtoul(argv[a],0,10),v=strtoul(argv[a+1],0,10),s=strtoul(argv[a+2],0,10);
  if(s<1000 || s>1000000 || u>1000000-s || v>1000000-s) return 2;
  mpf_mul_ui(t,span,u); mpf_div_ui(t,t,1000000); mpf_add(left,left,t);
  mpf_mul_ui(t,span,v); mpf_mul_ui(t,t,H); mpf_div_ui(t,t,W); mpf_div_ui(t,t,1000000); mpf_sub(top,top,t);
  mpf_mul_ui(span,span,s); mpf_div_ui(span,span,1000000);
 }
 mpf_div_ui(step,span,W);
 unsigned char *image=malloc(W*H*3);
 if(!image) { mpf_clears(left,top,span,t,step,NULL); return 1; }
 const char *backend="gmp";
 const char *stream_env=getenv("RENDER_STREAM");
 int streaming=stream_env && strcmp(stream_env,"1")==0;
 const char *requested=getenv("RENDER_BACKEND");
 if(!requested || strcmp(requested,"long-double")==0) {
  if(long_double_rows(left,top,step,limit,image,streaming?emit_row:NULL)) backend="cpu-long-double";
  else { fprintf(stderr,"fallback=long-double-precision\n"); fflush(stderr); }
 }
#ifdef USE_OPENCL
 else if(requested && strcmp(requested,"perturb")==0) {
  const char *gpu=gpu_perturb(left,top,step,(unsigned int)limit,image,streaming?emit_row:NULL);
  if(gpu) backend=gpu;
 } else if(!requested || strcmp(requested,"gmp")!=0) {
  const char *gpu=gpu_render(mpf_get_d(left),mpf_get_d(top),mpf_get_d(step),(unsigned int)limit,image,streaming?emit_row:NULL);
  if(gpu) backend=gpu;
 }
#endif
 if(strcmp(backend,"gmp")==0) {
 gmp_rows(left,top,step,limit,image,NULL,0,H,streaming?emit_row:NULL);
 }
 fprintf(stderr,"backend=%s\n",backend);
 /* Output only after all workers finish, preserving the raster row order. */
 int failed=!streaming && fwrite(image,1,W*H*3,stdout)!=W*H*3;
 free(image);
 mpf_clears(left,top,span,t,step,NULL);
 return fflush(stdout)==0 && !failed?0:1;
}
