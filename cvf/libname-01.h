
#ifndef _INC_LIBNAME_01_H
#define _INC_LIBNAME_01_H


#if !defined(_MSC_VER)||_MSC_VER>1850||_MSC_VER<1400
//#error "The compiler is not supported by current lib-configuraton!"
#endif

#if _MSC_VER==1900
	#define _LN_CVER() "8"
#elif _MSC_VER==1800
	#define _LN_CVER() "7"
#elif _MSC_VER==1700 
	#define _LN_CVER() "6"
#elif _MSC_VER==1600
	#define _LN_CVER() "5"
#elif _MSC_VER==1400
	#define _LN_CVER() "3"
#endif

#ifdef _UNICODE
#define _LN_CHAR() "u"
#else
#define _LN_CHAR() ""
#endif


#ifdef _DLL
#define _LN_MT()  ""
#else
#define _LN_MT()  "t" //not supported yet
//#define _LN_MT()  "" //not supported yet
#endif

#ifdef _DEBUG
#define _LN_DEBUG() "d"
#else
#define _LN_DEBUG() ""
#endif

#define _LN_STR_VAL(s) s

#define _LN_LIB_TYPE(s) _LN_CVER()##_LN_STR_VAL(s)##_LN_CHAR()_LN_MT()_LN_DEBUG()".lib"

#define _LN_LIB_TYPE_A(s) _LN_CVER()##_LN_STR_VAL(s)##_LN_MT()_LN_DEBUG()".lib"

#define _LN_LIB_TYPE_AS(s) _LN_STR_VAL(s)_LN_MT()##_LN_DEBUG()##".lib"

#define _LN_STR(str) #str


#define _LN_DLL_LIB(name) _LN_STR(name)_LN_LIB_TYPE("")

#define _LN_STATIC_LIB(name) _LN_STR(name)_LN_LIB_TYPE("s")

#define _LN_DLL_LIB_A(name) _LN_STR(name)_LN_LIB_TYPE_A("")

#define _LN_DLL_LIB_AS(name) _LN_STR(name)##_LN_LIB_TYPE_AS("")

#define _LN_STATIC_LIB_A(name) _LN_STR(name)_LN_LIB_TYPE_A("s")


#endif


