
//#ifndef _INC_LIBD_01_H
//#define _INC_LIBD_01_H

#include"libname-01.h"


///////////////////////////////////////////////////////
//ANN
#define ANN_SF_LIB	0x01  //short_float
#define ANN_IF_LIB	0x02  //int_float
#define ANN_FF_LIB	0x04  //float_float
#define ANN_ID_LIB	0x08  //int_double
#define ANN_FD_LIB	0x10  //float_double

#ifdef USE_ANN_LIB

#ifndef ANN_LIB_MASK
#error "please define ANN_LIB_MASK"
#endif

#if ANN_LIB_MASK&ANN_SF_LIB
#pragma comment(lib,"ANN_sf.lib")
#endif

#if ANN_LIB_MASK&ANN_IF_LIB
#pragma comment(lib,"ANN_if.lib")
#endif

#if ANN_LIB_MASK&ANN_FF_LIB
#pragma comment(lib,"ANN_ff.lib")
#endif

#if ANN_LIB_MASK&ANN_ID_LIB
#pragma comment(lib,"ANN_id.lib")
#endif

#if ANN_LIB_MASK&ANN_FD_LIB
#pragma comment(lib,"ANN_fd.lib")
#endif

#endif

#ifdef USE_ANN_STATIC_LIB


#ifndef ANN_LIB_MASK
#error "please define ANN_LIB_MASK"
#endif

#if ANN_LIB_MASK&ANN_SF_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(ANN_sf_))
#endif

#if ANN_LIB_MASK&ANN_IF_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(ANN_if_))
#endif

#if ANN_LIB_MASK&ANN_FF_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(ANN_ff_))
#endif

#if ANN_LIB_MASK&ANN_ID_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(ANN_id_))
#endif

#if ANN_LIB_MASK&ANN_FD_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(ANN_fd_))
#endif

#endif


#ifdef USE_CAPX_LIB
#pragma comment(lib,"capxs.lib")
#endif


#ifdef USE_GCOPT_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(gcopt))
#endif


#ifdef USE_MEANSHIFT_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(meanshift))
#endif

#ifdef USE_MLX_LIB
#pragma comment(lib,_LN_DLL_LIB_A(mlx))
#endif

#ifdef USE_NUMX_LIB
#pragma comment(lib,"mynum.lib")
#endif


#if defined(USE_PCRE_STATIC_LIB)||defined(USE_PCRE_LIB)
#pragma comment(lib,_LN_STATIC_LIB_A(pcre))
#endif

#ifdef USE_SIFT_LIB
#pragma comment(lib,_LN_STATIC_LIB_A(sift))
#endif


#ifdef USE_SECF_LIB

#define USE_ZLIB_STATIC_LIB

#pragma comment(lib,"iphlpapi.lib") //import GetAdaptersInfo
#pragma comment(lib,"version.lib")

#endif

#ifdef USE_ZLIB_STATIC_LIB

#pragma comment(lib, _LN_STATIC_LIB(zlib))

#endif


#ifdef USE_FFCV_LIB

#pragma comment(lib,_LN_DLL_LIB_A(ffcv))

#endif


//#endif

