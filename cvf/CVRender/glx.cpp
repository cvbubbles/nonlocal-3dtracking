
//#include FVT_WINX_H
//#include FVT_GLX_H
#include"glx.h"

#define FVT_WINAPI_FAILED() throw "winapi failed"
#define _WIN_API_FAILED FVT_WINAPI_FAILED()


void  wxInitBitmapInfo(BITMAPINFO& bmh, int width, int height, unsigned nBits, int compression)
{
	memset(&bmh, 0, sizeof(bmh));
	bmh.bmiHeader.biSize = sizeof(bmh.bmiHeader);
	bmh.bmiHeader.biBitCount = nBits;
	bmh.bmiHeader.biHeight = height;
	bmh.bmiHeader.biWidth = width;
	bmh.bmiHeader.biPlanes = 1;
	bmh.bmiHeader.biCompression = compression;
}

HDC  wxCreateDeviceInDDB(unsigned width, unsigned height, HDC hdc)
{
	bool bRelease = false;
	HBITMAP hBmp = NULL;
	if (!hdc)
	{
		if (!(hdc = GetDC(NULL)))
			goto _END;
		bRelease = true;
	}
	hBmp = CreateCompatibleBitmap(hdc, width, height);

	if (bRelease)
		ReleaseDC(NULL, hdc);
_END:
	if (!hBmp)
		_WIN_API_FAILED;
	return wxCreateDeviceInBitmap(hBmp);
}
HBITMAP  wxCreateDIBSection8b(uint width, uint height, void** ppv, const RGBQUAD* palette)
{
	char buf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
	wxInitBitmapInfo(*(BITMAPINFO*)buf, width, height, 8);
	RGBQUAD* pbeg = &((BITMAPINFO*)buf)->bmiColors[1];
	if (palette)
		memcpy(pbeg, palette, 256 * sizeof(RGBQUAD));
	else
	{
		for (int i = 0; i<256; ++i, ++pbeg)
		{
			pbeg->rgbBlue = pbeg->rgbGreen = pbeg->rgbRed = i;
			pbeg->rgbReserved = 0;
		}
	}
	HBITMAP hBmp = CreateDIBSection(NULL, (BITMAPINFO*)buf, 0, ppv, NULL, 0);

	if (!hBmp)
		_WIN_API_FAILED;
	return hBmp;
}

HBITMAP  wxCreateDIBSection(unsigned width, unsigned height, void** ppv, unsigned nBits)
{
	HBITMAP  hBmp = NULL;

	if (nBits == 8)
		hBmp = wxCreateDIBSection8b(width, height, ppv, NULL);
	else
	{
		BITMAPINFO bmi;
		wxInitBitmapInfo(bmi, width, height, nBits);
		hBmp = CreateDIBSection(NULL, &bmi, 0, ppv, NULL, 0);
	}
	if (!hBmp)
		_WIN_API_FAILED;
	return hBmp;
}
HDC  wxCreateDeviceInBitmap(HBITMAP hbmp)
{
	HDC hMem = CreateCompatibleDC(NULL);
	if (!hMem)
		_WIN_API_FAILED;
	SelectObject(hMem, hbmp);
	return hMem;
}
HDC  wxCreateDeviceInDIB(unsigned width, unsigned height, void** ppv, unsigned nBits)
{
	return wxCreateDeviceInBitmap(
		wxCreateDIBSection(width, height, ppv, nBits)
	);
}

//_FVT_BEG

int  glxDetectError(std::string &msg)
{
	if(wglGetCurrentDC()==NULL)
	{
		msg="OpenGL is not initialized!";
		return GL_INVALID_OPERATION;
	}
	int ec=glGetError();
	if(ec!=GL_NO_ERROR)
		msg=std::string((char*)gluErrorString(ec));
	return ec;
}

 void _verify_opengl(const char *file, int line)
{
	std::string msg;
	if(glxDetectError(msg)!=GL_NO_ERROR) 
	{
		//_HandleVerifyError(true,FVT_ERROR_EXCEPTION,"error_opengl_failed",msg.c_str(),file,line);
	}
}

bool  glxSupportDoubleBuffers()
{
	PFDT pfd;
	glxGetCurrentPixelFormat(&pfd);
	return (pfd.dwFlags&PFD_DOUBLEBUFFER)!=0;
}


void  glxGetCurrentPixelFormat(PFDT *pfd)
{
	if(pfd)
	{
		HDC hdc=wglGetCurrentDC();
		int pfi=0;
		if(hdc&&(pfi=GetPixelFormat(hdc)))
			DescribePixelFormat(hdc,pfi,sizeof(PFDT),pfd);
		else
			FVT_WINAPI_FAILED();
	}
}
void  glxInitPFDStruct(HDC hdc,PFDT *pfd,GLXEnvFlag flag)
{
	if(pfd)
	{
		int nMax=DescribePixelFormat(hdc,1,sizeof(PFDT),pfd);
		
		if(nMax<=0)
			FVT_WINAPI_FAILED();

		if(flag&GLX_FORCE_ALPHA)
		{
			for(int i=1;i<nMax;++i)
			{
				nMax=DescribePixelFormat(hdc,i,sizeof(PFDT),pfd);
				if(pfd->cAlphaBits>0)
					break;
			}
			if(pfd->cAlphaBits<=0)
				//FVT_EXCEPT(FVT_EC_UNSUPPORTED_MODE,"No pixel format with alpha channel found!");
				FVT_WINAPI_FAILED();
		}
		int glFlag=PFD_SUPPORT_OPENGL;

		if(flag&GLX_DRAW_TO_BITMAP)
			glFlag|=PFD_DRAW_TO_BITMAP;
		else
			glFlag|=PFD_DRAW_TO_WINDOW;

		if(flag&GLX_DOUBLEBUFFER)
			glFlag|=PFD_DOUBLEBUFFER;

		pfd->dwFlags=glFlag;
	}
}

static HGLRC fvtiInitGL(HDC hdc,const PFDT& pfd,GLXEnvFlag exFlag)
{
	int idx=ChoosePixelFormat(hdc,&pfd);
	/*if(!idx||!SetPixelFormat(hdc,idx,&pfd))
		FVT_WINAPI_FAILED();*/

	
	PFDT pfdx;
	DescribePixelFormat(hdc,idx,sizeof(PFDT),&pfdx);
	
	//check format
	if(pfdx.dwFlags&PFD_NEED_PALETTE) 
	{
		//FVT_EXCEPT(FVT_EC_UNSUPPORTED_MODE,"No palette is available for the selected pixel format!");
		FVT_WINAPI_FAILED();
	}
	if((exFlag&GLX_FORCE_ALPHA)&&pfdx.cAlphaBits<=0)
	{
		//FVT_EXCEPT(FVT_EC_UNSUPPORTED_MODE,"No alpha channel contained in the selected pixel format!");
		FVT_WINAPI_FAILED();
	}

	HGLRC hgl=wglCreateContext(hdc);
	if(!hgl)
		FVT_WINAPI_FAILED();
	
	if(!wglMakeCurrent(hdc,hgl))
		FVT_WINAPI_FAILED();

	return hgl;
}

HGLRC  glxInitGL(HDC hdc,const PFDT* ppfd,GLXEnvFlag exFlag)
{
	PFDT pfd;
	if(ppfd)
		memcpy(&pfd,ppfd,sizeof(PFDT));
	else
		glxInitPFDStruct(hdc,&pfd,exFlag);
	return fvtiInitGL(hdc,pfd,exFlag);
}
//
//#undef min
//
//void  glxApplyRendContext(const RendContext &rc,int rcm)
//{
//	if(rcm&FVT_RCM_PERSPECTIVE)
//	{
//		glMatrixMode(GL_MODELVIEW);
//		glLoadIdentity();
//	
//		const CGView &v(rc.m_view);
//		Point3f cent(v.m_eye-v.m_vz);
//		gluLookAt(v.m_eye.X(),v.m_eye.Y(),v.m_eye.Z(),cent.X(),cent.Y(),cent.Z(),v.m_vy.X(),v.m_vy.Y(),v.m_vy.Z());
//	}
//
//	if(rcm&FVT_RCM_PROJECTION)
//	{
//		glMatrixMode(GL_PROJECTION);
//		glLoadIdentity();
//		gluPerspective(rc.m_view.m_fovy,rc.m_view.m_aspect>0? rc.m_view.m_aspect:1.0f,rc.m_view.m_near,rc.m_view.m_far);
//	}
//	if(rcm&FVT_RCM_VIEWPORT)
//		glViewport(rc.m_viewPort.X(),rc.m_viewPort.Y(),rc.m_viewPort.Width(),rc.m_viewPort.Height());
//
//	if(rcm&FVT_RCM_LIGHT)
//	{
//		const int MAX_LIGHT=7;
//
//		int nLight=std::min(MAX_LIGHT,(int)rc.m_vLight.size());
//
//		glEnable(GL_LIGHTING);
//
//		for(int i=nLight;i<MAX_LIGHT;++i) //disable unnecessary light.
//			glDisable(GL_LIGHT0+i);
//
//		for(int i=0;i<nLight;++i)
//		{
//			const LightSource &ls(rc.m_vLight[i]);
//
//			const int id=GL_LIGHT0+i;
//
//			glEnable(id);
//
//			glLightfv(id,GL_POSITION,&ls.m_pos[0]);
//
//			glLightfv(id,GL_AMBIENT,&ls.m_ambient[0]);
//			glLightfv(id,GL_DIFFUSE,&ls.m_diffuse[0]);
//			glLightfv(id,GL_SPECULAR,&ls.m_specular[0]);
//		}
//	}
//}
//
//void   glxSetMaterial(const float pMat[12],int face)
//{
//	if(pMat)
//	{
//		glMaterialfv(face,GL_AMBIENT,&pMat[0]);
//		glMaterialfv(face,GL_DIFFUSE,&pMat[4]);
//		glMaterialfv(face,GL_SPECULAR,&pMat[8]);
//	}
//}
//void   glxRendMesh(const float (*pVtx)[3],const float (*pNormal)[3],int nVtx,
//						   const int   (*pFace)[3],const int    *pFaceMat,int nFace,
//						   const float  (*pMat)[12],int matStride, //(ambient,diffuse,specular rgba)
//						   const float  pDefMat[12], //default material when face material is not specified.
//						   int elemType
//						   )
//{
//	if(!pFaceMat)
//		glxSetMaterial(pDefMat,GL_FRONT);
//
//	if(matStride<=0)
//		matStride=sizeof(float)*12;
//
//	int mi0=-1;
//
//	for(int i=0;i<nFace;++i)
//	{
//		if(pFaceMat)
//		{
//			const int mix=pFaceMat[i];
//			if(mix>=0)
//			{
//				if(mix!=mi0)
//				{
//					const float *pmx=(float*)((char*)pMat+mix*matStride);
//					glxSetMaterial(pmx,GL_FRONT);
//				}
//			}
//			else
//			{
//				if(mi0>=0)
//					glxSetMaterial(pDefMat,GL_FRONT);
//			}
//		}
//		glBegin(elemType);
//		for(int vi=0;vi<3;++vi)
//		{
//			const int mi=pFace[i][vi];
//			if(pNormal)
//				glNormal3fv(&pNormal[mi][0]);
//			glVertex3fv(&pVtx[mi][0]);
//		}
//		glEnd();
//	}
//}
//
//
//void  glxTransformDepth(const float * pIn,float *pOut, int count,int mode,
//								 float vNear,float vFar,float scale,float shift)
//{
//	if(pIn&&pOut)
//	{
//		if(mode==FVT_TD_TO_WORLD)
//		{
//			const float a=-2*vNear*vFar,b=vFar-vNear,c=-(vFar+vNear);
//			for(int i=0;i<count;++i,++pIn,++pOut)
//			{
//				float z=*pIn*2-1;
//				z=a/(z*b+c);
//				*pOut=(z*scale+shift);
//			}
//		}
//		else
//			if(mode==FVT_TD_TO_GL)
//			{
//				const float a=vFar/(vFar-vNear),b=a*vNear;
//				
//				for(int i=0;i<count;++i)
//				{
//					float z=(a-b/pIn[i])*scale+shift;
//					if(z<0)
//						z=0;
//					else
//						if(z>1.0f)
//							z=1.0f;
//					pOut[i]=z;
//				}
//			}
//			else
//			{
//				FVT_ERROR(FVT_EC_INVALID_ARGUMENT,"");
//			}
//	}
//}
//
//_FVT_END
//
