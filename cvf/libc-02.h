
//#ifndef _INC_CSDK_01_H
//#define _INC_CSDK_01_H

#ifdef USE_LIBCX_LIB
#define USE_LIBC_LIB
#include"libcx\imp_libcx.hpp"
#endif

#ifdef USE_LIBC_LIB
#define USE_BFC_LIB
#define USE_IFF_LIB
#define USE_IPF_LIB
#endif


#if defined(USE_BFC_LIB)

#include"bfc\src.cpp"

#endif


#ifdef  USE_IFF_LIB

#include"iff\src.cpp"

#endif

#if defined(USE_IPF_LIB)

#include"ipf\src.cpp"

#endif



//#endif

