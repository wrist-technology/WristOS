/**
 *
 *  fastfixmath.h
 *
 *  Wrist Technology Ltd
 *
*/ 

#ifndef FASTFIXMATH_H_
#define FASTFIXMATH_H_

//this represents the "1"
#define FFM_UNIT 2048
#define FFM_PI   6433 //pi*2048
#define FFM_NAN  0x80000001

extern int ffm_mult(int a, int b);

extern int ffm_sin(int x);

extern int ffm_cos(int x);


#endif
      