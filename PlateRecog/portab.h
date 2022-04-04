/*
 ********************************************************************************************
 *  \brief
 *      portab.h : Defines the entry point for the console application
 *
 *  \author
 *          launch
 *
 *
 *  \version
 *          0.0.0.2
 *
 *  \date
 *          2007.08.10
 *
 *  \Note
 *
 * ********************************************************************************************
*/
#ifndef _PORTAB_H_
#define _PORTAB_H_

#include <stdlib.h>
//===============================================================================================
#ifdef _MSC_VER

// vc6 does not support __declspec(aligned(x)) instead we need vc6 + sp5(or more), here we do some test
#if _MSC_VER < 1200
#define DECLARE_ALIGNED_MATRIX(name, sizex, sizey, type, alignment) \
    type name[(sizex)*(sizey)]
#else   // vc6 or higher
#define DECLARE_ALIGNED_MATRIX(name, sizex, sizey, type, alignment) \
    __declspec(align(alignment)) type name[(sizex)*(sizey)]
#endif

#else

//  #ifndef _TMS320C6X_
//      #define _TMS320C6X_
//  #endif
#define DECLARE_ALIGNED_MATRIX(name, sizex, sizey, type, alignment) \
    type name[(sizex)*(sizey)]

#endif //_MSC_VER
//===============================================================================================

#define CACHE_SIZE  128

#define COST_MAX    (1<<20)

#define CH_NUMS     (8+1) // �����ַ�����, ��7����Ϊ8�������������ƣ����һ�����ַ�����������

#define CHARACTER_PIXEL (0u)

#ifdef  _TMS320C6X_
extern far int DDR;
extern far int L1DSRAM;
extern far int IRAM;
extern far int DDRALGHEAP;
#endif

//===============================================================================================
typedef   signed char    int8_t;
typedef unsigned char   uint8_t;
typedef          short   int16_t;
typedef unsigned short  uint16_t;
typedef          int     int32_t;
typedef unsigned int    uint32_t;
typedef unsigned long long  uint64_t;

typedef float               svm_type;
typedef float           feature_type; // ע����PC��Ϊ�˷���ʹ��SSE2ָ������ø������������ַ�����
typedef float               pro_type;

#ifdef WIN32
typedef __int64              int40_t;
typedef unsigned __int64    uint40_t;
typedef __int64              int64_t;
typedef unsigned __int64    uint64_t;
typedef unsigned __int64    PACKED64;
typedef float               svm_type;
typedef float           feature_type; // ע����PC��Ϊ�˷���ʹ��SSE2ָ������ø������������ַ�����
typedef float               pro_type;
#elif defined(_TMS320C6X_)
typedef          long        int40_t;
typedef unsigned long       uint40_t;
typedef          long long   int64_t;
typedef unsigned long long  uint64_t;
typedef double              PACKED64;
typedef int                 svm_type;
typedef short           feature_type; // ע����DSP�в���16λ�з������������ַ�����
typedef float               pro_type;
#else
//  typedef __int64              int40_t;
//  typedef unsigned __int64    uint40_t;
//  typedef __int64              int64_t;
//  typedef unsigned __int64    uint64_t;
//  typedef unsigned __int64    PACKED64;
#endif

#ifdef WIN32
#define inline    __inline
#define restrict
#elif defined(_TMS320C6X_)
#define inline      inline
#define restrict    restrict
#else
#define inline    __inline
#define restrict
#endif//WIN32
//===============================================================================================

#if 0
//===============================================================================================
#define ByteSwapU32(abyte)  \
    {   \
        unsigned int temp0, temp1, temp2, temp3, temp4; \
        temp1 = (abyte & 0xFF00FF00)>>8;    \
        temp0 = (abyte & 0x00FF00FF)<<8;    \
        temp2 = temp0 + temp1;  \
        temp3 = (temp2 & 0x0000FFFF)<<16;   \
        temp4 = (temp2 & 0xFFFF0000)>>16;   \
        abyte = temp3 + temp4;  \
    }
//===============================================================================================
#else
//===============================================================================================
//  #define ByteSwapU32(abyte)  \
//      abyte = ((abyte>>24)&0xff) + ((abyte>>8)&0xff00) + ((abyte<<8)&0x00ff0000) + (abyte<<24);

#define ByteSwapU32(abyte)  \
    abyte = ((abyte>>24)&0xff) | ((abyte>>8)&0xff00) | ((abyte<<8)&0x00ff0000) | (abyte<<24);

//  #define ByteSwapU32(a)  \
//  {   \
//      unsigned int temp;  \
//      temp = _swap4(a);   \
//      a = _packlh2(temp, temp);   \
//  }
//===============================================================================================
#endif

// modified by Quanming Wang 2007.12.29
//===============================================================================================
#ifdef _TMS320C6X_
#define DATA_ALIGNED_4B(data)   _nassert((int)data & 0x3 == 0); // single word aligned
#define DATA_ALIGNED_8B(data)   _nassert((int)data & 0x7 == 0); // double word aligned
#else
#define DATA_ALIGNED_4B(data)
#define DATA_ALIGNED_8B(data)
#endif
//===============================================================================================

// ����� ADSP_BF561 �б���� PC �ϵĽ����һ�µ����� modified by Quanming Wang 2007.01.29
//===============================================================================================
//#define CLIP_255(x) ((x & ~255) ? (-x >> 31) : x)
//#define CLIP_255(x) ((x & ~255) ? ((-x) >> 15) : x)

// static int CLIP_255(int x)
// {
//  if((x) < 0)
//  {
//      return 0;
//  }
//  else if((x) > 255)
//  {
//      return 255;
//  }
//  else
//  {
//      return (x);
//  }
// }


#define ABS(x)        ((x) > 0 ? (x) : -(x))
#define MIN2(a, b)    ((a) < (b) ? (a) : (b))
#define MAX2(a, b)    ((a) > (b) ? (a) : (b))
#define EMA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define EMA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) ((a) > (b) ? MAX2(a, c) : MAX2(b, c))
#define MIN3(a, b, c) ((a) < (b) ? MIN2(a, c) : MIN2(b, c))
#define CLIP3(x, minv, maxv)  MIN2((MAX2(x, minv)), maxv)

//===============================================================================================

#if 0
#define EMA_DIFF(a, b)  ((abs((a) - (b)))^2)
#else
#define EMA_DIFF(a, b)  ((abs((a) - (b))))
#endif

#define SWAP(type, x, y) { type* _tmp_; _tmp_ = x; x = y ; y = _tmp_;}

#if 0
#define DP(X)   printf X
#else
#define DP(X)
#endif

#ifdef  WIN32
#define VEC_COPY(dst, src) (*(uint64_t *)&(dst)) = (*(uint64_t *)&(src))
//  #define VEC_COPY(dst, src) ((dst).refno = (src).refno; (dst).x = (src).x;(dst).y = (src).y);
#else
#define VEC_COPY(dst, src) _mem8(&(dst)) = _mem8(&(src));
#endif

typedef struct
{
    int16_t x0; // ��
    int16_t x1; // ��
    int16_t y0; // ��
    int16_t y1; // ��
} Rects;

typedef struct rectnode
{
    int16_t x0; // ��
    int16_t x1; // ��
    int16_t y0; // ��
    int16_t y1; // ��
    struct rectnode *next;
} RectsLink;

#endif // _PORTAB_H_


