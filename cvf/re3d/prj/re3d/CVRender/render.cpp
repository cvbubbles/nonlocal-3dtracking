
#include"cvrender.h"
#include"GL/glut.h"
using namespace cv;
#include"GL/freeglut.h"
#include"opencv2/imgproc.hpp"
using namespace cv;
#include<time.h>

#if 0
class CVRImpl
{
	CVRCamera *camera;
	int		  renderWidth, renderHeight;
	float     nearP, farP;
	CVRModel  *model;
	cv::Mat   dimg;
public:
	static CVRImpl gobj;
public:
	void _displayCB()
	{
		if (!camera||!model)
			return;

		{
			// clear buffer
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT); // for GL_DRAW_BUFFER and GL_READ_BUFFER
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);

			//glEnable(GL_NORMALIZE);

			glDisable(GL_CULL_FACE);

			// draw the model
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(camera->getProjectionIntrinsic(renderWidth, renderHeight, nearP, farP));

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glLoadMatrixf(camera->getModelviewExtrinsic());

			glPushMatrix();

			
			// draw object
			//glmDraw(model->_model, GLM_MATERIAL | GLM_TEXTURE | GLM_SMOOTH);
			model->render();
			

			glPopMatrix();
			glPopAttrib(); // GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT

			

			dimg.create(Size(renderWidth, renderHeight),CV_8UC3);
			glReadBuffer(GL_BACK);
			time_t beg = clock();
			glReadPixels(0, 0, renderWidth, renderHeight, GL_RGB, GL_UNSIGNED_BYTE, dimg.data);
			printf("%dms\n", int(clock() - beg));
			cvtColor(dimg, dimg, CV_BGR2RGB);
			flip(dimg, dimg, 0);

			

		//	getRGBABuffer();
		//	getDepthBuffer();
		}

		// draw
		//glutSwapBuffers();
	}
	static void displayCB()
	{
		//gobj._displayCB();
	}
	void _reshapeCB(int width, int height)
	{
		if (!camera)
			return;
		//screenWidth = width;
		//screenHeight = height;

		// set viewport to be the entire window
		glViewport(0, 0, (GLsizei)renderWidth, (GLsizei)renderHeight);

		// set perspective viewing frustum
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(camera->getProjectionIntrinsic(renderWidth, renderHeight, nearP, farP));

		// switch to modelview matrix in order to set scene
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	static void reshapeCB(int width, int height)
	{
		gobj._reshapeCB(width, height);
	}
	static void idleCB()
	{
		//glutPostRedisplay();
	}
public:
	CVRImpl()
	{}

	void _initLights()
	{
		// set up light colors (ambient, diffuse, specular)
		GLfloat lightKa[] = { .2f, .2f, .2f, 1.0f };  // ambient light
		GLfloat lightKd[] = { .7f, .7f, .7f, 1.0f };  // diffuse light
		GLfloat lightKs[] = { 1, 1, 1, 1 };           // specular light
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

		// position the light
		float lightPos[4] = { 0, 0, 50, 1 }; // positional light
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
	}

	void _initGL()
	{
		glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
	//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);      // 4-byte pixel alignment
	//	glPixelStorei(GL_PACK_ALIGNMENT, 1);      // 4-byte pixel alignment

												  // enable /disable features
	//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	//	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_DEPTH_TEST);
	//	glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
	//	glEnable(GL_CULL_FACE);

		// track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
	//	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	//	glEnable(GL_COLOR_MATERIAL);

	//	glClearColor(0, 0, 0, 0);                   // background color
	//	glClearDepth(1.0f);                         // 0 is near, 1 is far
	//	glDepthFunc(GL_LEQUAL);

	//	_initLights();
	}

	void init(int width, int height)
	{
		int argc = 1;
		char *argv = "CVRender";

		static bool inited = false;
		if (!inited)
		{
			glutInit(&argc, &argv);
			inited = true;
		}
		
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
		glutInitWindowSize(width, height);
		int wid=glutCreateWindow(argv);
		//glutHideWindow();
		//glutPostWindowRedisplay(wid);
		

		renderWidth = width;
		renderHeight = height;
		nearP = 1;
		farP = 1000;

		glutDisplayFunc(displayCB);
		//glutIdleFunc(idleCB);                       // redraw whenever system is idle
		//glutReshapeFunc(reshapeCB);
		glutMainLoopEvent();

		_initGL();

		
	}

	Mat render(CVRModel *model, CVRCamera &camera)
	{
		this->model = model;
		this->camera = &camera;

		//glutPostRedisplay();
		_displayCB();

		return dimg;
	}
public:

};

CVRImpl CVRImpl::gobj;

void CVRender::init(int width, int height)
{
	CVRImpl::gobj.init(width, height);
}

Mat CVRender::render(CVRModel *model, CVRCamera &camera)
{
	return CVRImpl::gobj.render(model, camera);
}







#endif
