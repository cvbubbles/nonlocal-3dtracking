#pragma once

#ifdef RE3D_STATIC
#define _RE3D_API
#endif

#ifndef _RE3D_API

#if defined(_WIN32)

#ifdef RE3D_EXPORTS
#define _RE3D_API __declspec(dllexport)
#else
#define _RE3D_API __declspec(dllimport)
#endif

#pragma warning(disable:4251)

#elif defined(__GNUC__) && __GNUC__ >= 4

#ifdef RE3D_EXPORTS
#define _RE3D_API __attribute__ ((visibility ("default")))
#else
#define _RE3D_API 
#endif

#else
#define _RE3D_API
#endif

#endif

#define _RE3D_BEG  namespace re3d{
#define _RE3D_END  }

#ifdef _STATIC_BEG
#undef _STATIC_BEG
#undef _STATIC_END
#endif

#define _STATIC_BEG  using namespace re3d; namespace { 
#define _STATIC_END  }

#define namespace_beg(name) namespace name{
#define namespace_end(name) }

#define namespace_beg2(x,xx) namespace x{namespace xx{
#define namespace_end2(x,xx) }}


#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define _Re3D_UNIQUE_NAME(x) TOKENPASTE2(x, __LINE__)


#ifdef _MSC_VER

#pragma warning(disable:4267)
#pragma warning(disable:4251)
#pragma warning(disable:4190)

#endif


