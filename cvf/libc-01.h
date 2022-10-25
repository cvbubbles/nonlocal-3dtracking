
//#ifndef _INC_CSDK_01_H
//#define _INC_CSDK_01_H

#include"libname-01.h"

#ifdef USE_LIBCX_LIB
#define USE_LIBC_LIB
#pragma comment(lib,_LN_STATIC_LIB(libcx))
#endif

#ifdef USE_LIBC_LIB
#define USE_FFS_LIB
#define USE_BFC_STATIC_LIB
#define USE_IFF_LIB
#define USE_IPF_STATIC_LIB
#endif


#ifdef  USE_FFS_LIB

#ifdef _FFS_STATIC
#pragma comment(lib,_LN_STATIC_LIB(ffs))
#else
#pragma comment(lib,_LN_DLL_LIB(ffs))
#endif

#endif

#if defined(USE_BFC_STATIC_LIB)||defined(USE_BFC_LIB)

#pragma comment(lib,_LN_STATIC_LIB(bfc) )

#endif


#ifdef  USE_IFF_LIB

#ifdef _IFF_STATIC
#pragma comment(lib,_LN_STATIC_LIB(iff))
#else
#pragma comment(lib,_LN_DLL_LIB(iff))
#endif

#endif

#if defined(USE_IPF_STATIC_LIB)||defined(USE_IPF_LIB)

#pragma comment(lib, _LN_STATIC_LIB(ipf))

#endif



//#endif

