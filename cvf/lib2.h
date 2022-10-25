


#include"libc-02.h"
#include"libd-01.h"


#ifdef  USE_VFX_LIB

#ifndef USE_WXWINDOWS_LIB
#define USE_WXWINDOWS_LIB
#endif

#endif

#ifdef  USE_VFX2_LIB

#ifndef USE_WXWIDGETS2_LIB
#define USE_WXWIDGETS2_LIB
#endif

#endif



#include"libx-01.h"


#if defined(USE_VFX_LIB)||defined(USE_VFX2_LIB)

#pragma comment(lib,_LN_DLL_LIB_AS(vfx))
//#pragma comment(lib,_LN_STR(vfx)##_LN_LIB_TYPE_AS(""))

#endif

#ifdef USE_VFXS_LIB

#pragma comment(lib, "vfxserv.lib")

#endif

#ifdef USE_ARD_LIB
#pragma comment(lib,"ardcore.lib")
#endif

#ifdef USE_MATLABX_LIB
#ifdef _DEBUG
#pragma comment(lib,"matlabxd.lib")
#else
#pragma comment(lib,"matlabx.lib")
#endif
#endif


#ifdef USE_CAFFEX_LIB
#ifdef _DEBUG
#pragma comment(lib,"caffexd.lib")
#else
#pragma comment(lib,"caffex.lib")
#endif
#endif


