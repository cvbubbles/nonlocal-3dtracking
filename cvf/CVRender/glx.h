
#ifndef _FVT_INC_GLX_H
#define _FVT_INC_GLX_H

#include<string>

#include<Windows.h>

#include<gl\gl.h>
#include<gl\glu.h>

typedef unsigned uint;

//Help to initialize a @BITMAPINFO struct.
//@width,@height: size of the bitmap.
//@nBits : color bits per-pixel.
//@compression : compression method.
void wxInitBitmapInfo(BITMAPINFO& bmh, int width, int height, uint nBits = 32, int compression = BI_RGB);

//Create a DDB device compatible with the specified device.
//@width,@height: the size of the DDB to create.
//@hdc : the handle of the device to specify the DDB format, that is, the created DDB must compatible with
//		this device. The default value NULL specify current desktop to be such a device.
HDC wxCreateDeviceInDDB(unsigned width, unsigned height, HDC hdc = NULL);

//Create a DIB section with 8 color bits.
//@width,@height: the size of the DIB to create.
//@ppv : a pointer to pointer which is used to receive the beginning address of the data block allocated for the DIB.
//@pallete: a pointer to a 256-color table which contain pallete entries for the DIB to use,
//			the default value NULL cause gray pallete be used.
//Return value: the handle to the created DIB.
HBITMAP wxCreateDIBSection8b(unsigned width, unsigned height, void** ppv = 0, const RGBQUAD* palette = 0);

//Create a DIB section.
//@width,@height,@ppv : see @wxCreateDIBSection8b.
//@nBits : specify the color bits of the DIB, if this parameter is 8, the @wxCreateDIBSection8b will be called
//			to create a DIB with gray pallete.
HBITMAP wxCreateDIBSection(unsigned width, unsigned height, void** ppv = 0, unsigned nBits = 32);

//create a compatible memroy device in bitmap.
HDC wxCreateDeviceInBitmap(HBITMAP hbmp);

//Create a memory device in DIB.
//this function will create the required DIB and a compatible memory device, and then select the
//bitmap into the device.
HDC wxCreateDeviceInDIB(unsigned width, unsigned height, void** ppv = 0, unsigned nBits = 32);

//_FVT_BEG

/////////////////////////////////////////////////////////////////
//OpenGL error handling routines.

//return OpenGL error code and format @msg with error description string if any error occur.
int  glxDetectError(std::string &msg);


 void _verify_opengl(const char *file, int line);

#define verify_gl() fvt::_verify_opengl(__FILE__,__LINE__)

#define FVT_CHECK_OPENGL() verify_gl()

#ifdef _DEBUG
#define FVT_ASSERT_OPENGL() FVT_CHECK_OPENGL()
#else
#define FVT_ASSERT_OPENGL()
#endif

/////////////////////////////////////////////////////////////////

//Whether the current OpenGL context support double-buffers.
bool  glxSupportDoubleBuffers();

typedef PIXELFORMATDESCRIPTOR PFDT;

//get the pixel format associated with current context.
void  glxGetCurrentPixelFormat(PFDT *pfd);

enum GLXEnvFlag
{
	GLX_DRAW_TO_WINDOW=0x01, //use a window as drawing surface.
	GLX_DRAW_TO_BITMAP=0x02, //use a bitmap selected into a memory device as drawing surface.
	GLX_DOUBLEBUFFER=0x04,   //enable double buffer.
	GLX_FORCE_ALPHA=0x08	 //create an OpenGL drawing surface with alpha channel, throw exception if failed.
};

//Initialize Pixel-Format-Descriptor structure, which will be used to create OpenGL context in
//device with handle @hdc.
void  glxInitPFDStruct(HDC hdc,PFDT *pfd,GLXEnvFlag flag=GLX_DRAW_TO_WINDOW);

//Create OpenGL context for device @hdc, the selected context should be most similar to the one
//described by @ppfd.

//NOTE: if GLX_DRAW_TO_BITMAP is specified in @exFlag, @hdc must be a handle to a compatible memory device.

HGLRC  glxInitGL(HDC hdc,const PFDT* ppfd=NULL,GLXEnvFlag exFlag=GLX_DRAW_TO_WINDOW);

//
//void   glxApplyRendContext(const RendContext &rc,int rcm);
//
//void   glxSetMaterial(const float pMat[12],int face=GL_FRONT); //(a,d,s)*(rgba)
//
//void   glxRendMesh(const float (*pVtx)[3],const float (*pNormal)[3],int nVtx,
//						   const int   (*pFace)[3],const int    *pFaceMat,int nFace,
//						   const float  (*pMat)[12],int matStride, //(ambient,diffuse,specular rgb)
//						   const float  pDefMat[12], //default material when face material is not specified.
//						   int elemType
//						   );
//
//enum
//{
//	FVT_TD_TO_GL=1,
//	FVT_TD_TO_WORLD=2
//};
//
//void  glxTransformDepth(const float * pIn,float *pOut, int count,int mode,
//								 float fNear,float fFar,float scale=1.0f,float shift=0.0f);



//_FVT_END

#endif

