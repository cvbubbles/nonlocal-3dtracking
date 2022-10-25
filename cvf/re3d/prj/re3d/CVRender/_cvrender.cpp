
#include"_cvrender.h"

#include"BFC/argv.h"

#include<list>
#include<condition_variable>
#include<mutex>
#include<thread>
#include<chrono>

#if 0
class _CVR_API CVRLock
{
public:
	~CVRLock();
};

typedef std::shared_ptr<CVRLock>  CVRLockPtr;

_CVR_API CVRLockPtr cvrLockGL();

//for using OpenGL in the multi-thread environment
#define CVR_LOCK() CVRLockPtr lockOpenGL=cvrLockGL();


static std::recursive_mutex g_glMutex;

CVRLock::~CVRLock()
{
	g_glMutex.unlock();
}

_CVR_API CVRLockPtr cvrLockGL()
{
	g_glMutex.lock();

	return CVRLockPtr(new CVRLock);
}
#endif

static void _callInitGLUT()
{
	static bool inited = false;
	if (!inited)
	{
		int argc = 1;
		char *argv = "CVRender";

		glutInit(&argc, &argv);
		inited = true;
	}
}

static void displayCB()
{
}

void _initGL()
{
	glShadeModel(GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glEnable(GL_NORMALIZE);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
}

class _CVRDevice
{
public:
	int _id = -1;
	Size _size;
public:
	~_CVRDevice()
	{
		release();
	}
	void release()
	{
		if (_id >= 0)
		{
			glutDestroyWindow(_id);
			_id = -1;
		}
	}
	void create(int width, int height, int flags, const std::string &name)
	{
		//CVR_LOCK();

		_callInitGLUT();

		this->release();
		glutInitDisplayMode(flags&CVRW_GLUT_MASK);
		glutInitWindowSize(width, height);
		_size = Size(width, height);

		_id = glutCreateWindow(name.c_str());
		if (!(flags&CVRW_VISIBLE))
		{
			glutHideWindow();
			glutPostWindowRedisplay(_id);
		}

		glutDisplayFunc(displayCB);
		glutMainLoopEvent();
		_initGL();
	}

	void setCurrent()
	{
		//CVR_LOCK();

		if (_id >= 0 && _id != glutGetWindow())
			glutSetWindow(_id);
	}
	void setSize(int width, int height)
	{
		Size newSize(width, height);
		if (newSize != _size)
		{
		//	CVR_LOCK();

			this->setCurrent();
			glutReshapeWindow(width, height);
			glutMainLoopEvent();

			glViewport(0, 0, width, height);
			_size = newSize;
		}
	}
};

CVRDevice::CVRDevice()
	:_impl(new _CVRDevice)
{
}
CVRDevice::CVRDevice(Size size, int flags, const std::string &name)
	: _impl(new _CVRDevice)
{
	_impl->create(size.width, size.height, flags, name);
}
CVRDevice::~CVRDevice()
{
}

void CVRDevice::create(Size size, int flags, const std::string &name)
{
	_impl->create(size.width, size.height, flags, name);
}

Size CVRDevice::size() const
{
	return _impl->_size;
}
bool CVRDevice::empty() const
{
	return _impl->_id < 0;
}
void CVRDevice::setCurrent()
{
	_impl->setCurrent();
}

void CVRDevice::setSize(int width, int height)
{
	_impl->setSize(width, height);
}


static std::thread::id  g_glThreadID;
static std::list<GLEvent*>  g_glEvents;
static std::mutex g_glEventsMutex;
static std::condition_variable g_glWaitCV;
CVRDevice  theDevice;

static void glEventsProc()
{
	g_glThreadID = std::this_thread::get_id();

	//create the device in the background thread
	theDevice = CVRDevice(Size(10, 10));

	while (true)
	{
		GLEvent *evt = nullptr;
		int nLeft = 0;
		{
			std::lock_guard<std::mutex> lock(g_glEventsMutex);

			if (!g_glEvents.empty())
			{
				evt = g_glEvents.front();
				g_glEvents.pop_front();
				nLeft = (int)g_glEvents.size();
			}
		}

		if (!evt)
		{
			g_glWaitCV.notify_all();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		else
		{
			evt->exec(nLeft);
			evt->release();
		}
	}
}

void cvrWaitFinish()
{
	std::mutex  waitMutex;
	std::unique_lock<std::mutex> lock(waitMutex);
	g_glWaitCV.wait(lock);
}

void cvrPostEvent(GLEvent *evt, bool wait)
{
	static bool threadStarted = false;
	if (!threadStarted)
	{
		std::thread t(glEventsProc);
		t.detach();
		threadStarted = true;
	}
	//if posted from the GL thread itself and required to wait finish, execute directly without post event
	if (wait && std::this_thread::get_id() == g_glThreadID)
	{
		evt->exec((int)g_glEvents.size());
		evt->release();
	}
	else
	{
		{
			std::lock_guard<std::mutex> lock(g_glEventsMutex);

			g_glEvents.push_back(evt);
		}
		if (wait)
			cvrWaitFinish();
	}
}

//=================================================================

/* ---------------------------------------------------------------------------- */
void get_bounding_box_for_node(const aiScene *scene, const  aiNode* nd,
	aiVector3D* min,
	aiVector3D* max,
	aiMatrix4x4* trafo
) {
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const  aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);

			min->x = __min(min->x, tmp.x);
			min->y = __min(min->y, tmp.y);
			min->z = __min(min->z, tmp.z);

			max->x = __max(max->x, tmp.x);
			max->y = __max(max->y, tmp.y);
			max->z = __max(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(scene, nd->mChildren[n], min, max, trafo);
	}
	*trafo = prev;
}

/* ---------------------------------------------------------------------------- */
void get_bounding_box(const aiScene *scene, aiVector3D* min, aiVector3D* max)
{
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene, scene->mRootNode, min, max, &trafo);
}

void makeSizePower2(Mat &img)
{
	int width = img.cols, height = img.rows;

	int xSize2 = width;
	int ySize2 = height;

	{
		double xPow2 = log((double)xSize2) / log(2.0);
		double yPow2 = log((double)ySize2) / log(2.0);

		int ixPow2 = int(xPow2 + 0.5);
		int iyPow2 = int(yPow2 + 0.5);

		if (xPow2 != (double)ixPow2)
			ixPow2++;
		if (yPow2 != (double)iyPow2)
			iyPow2++;

		xSize2 = 1 << ixPow2;
		ySize2 = 1 << iyPow2;
	}
	if (xSize2 != width || ySize2 != height)
		resize(img, img, Size(xSize2, ySize2));
}

static void loadTextureImages(const aiScene* scene, const std::string &modelDir, std::vector<TexImage> &vTex)
{
	vTex.clear();

	/* getTexture Filenames and Numb of Textures */
	for (unsigned int m = 0; m<scene->mNumMaterials; m++)
	{
		aiMaterial *mtl = scene->mMaterials[m];
		int nTex;
		if ((nTex = mtl->GetTextureCount(aiTextureType_DIFFUSE)) > 0)
		{
			aiString path;	// filename
			for (int i = 0; i < nTex; ++i)
			{
				//mtl->RemoveProperty(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, i));



				if (mtl->GetTexture(aiTextureType_DIFFUSE, i, &path) == AI_SUCCESS)
				{
					vTex.push_back({ path.data });
				}
			}
		}
	}

	for (size_t i = 0; i<vTex.size(); i++)
	{
		std::string fileloc = modelDir + "/" + vTex[i].file;	/* Loading of image */
		cv::Mat img = cv::imread(fileloc, IMREAD_ANYCOLOR);

		if (!img.empty()) /* If no error occurred: */
		{
			makeSizePower2(img);

			vTex[i].img = img;
		}
		else
		{
			/* Error occurred */
			printf("error: failed to load %s\n", fileloc.c_str());
		}
	}
}

void _CVRModel::load(const std::string &file, int postProLevel)
{
	uint postPro = postProLevel == 0 ? 0 :
		postProLevel == 1 ? aiProcessPreset_TargetRealtime_Fast :
		postProLevel == 2 ? aiProcessPreset_TargetRealtime_Quality :
		aiProcessPreset_TargetRealtime_MaxQuality;

	aiScene *newScene = (aiScene*)aiImportFile(file.c_str(), postPro);
	if (!newScene)
		FF_EXCEPTION(ERR_FILE_OPEN_FAILED, file.c_str());
	else
		this->clear();

	scene = newScene;
	sceneFile = file;

	get_bounding_box(scene, &scene_min, &scene_max);
	scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
	scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
	scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

	loadTextureImages(scene, ff::GetDirectory(file), vTex);
}

//==================================================================================

static void getSavedTextureName(const std::string &tarModelFile, const std::vector<TexImage> &vTex, std::vector<std::string> &curName, std::vector<std::string> &newName, bool isSTD)
{
	auto getNewFileName = [](const std::string &file, const std::string &prefix, bool single) {
		std::string base = ff::GetFileName(file, false), ext = ff::GetFileExtention(file);
		ff::str2lower(ext);
		if (ext != "jpg")
			ext = "png";
		return prefix + (single ? "" : "_" + base) + "." + ext;
	};

	curName.resize(vTex.size());
	newName.resize(vTex.size());

	if (!vTex.empty())
	{
		for (size_t i = 0; i < vTex.size(); ++i)
			curName[i] = vTex[i].file;

		if (!isSTD)
			newName = curName;
		else
		{
			std::string tarName = ff::GetFileName(tarModelFile, false);

			newName[0] = getNewFileName(curName[0], tarName, vTex.size() == 1 ? true : false);

			for (size_t i = 1; i < vTex.size(); ++i)
			{
				newName[i] = getNewFileName(curName[i], tarName, false);
			}
		}
	}
}

static void setTextureName(aiScene *scene, const std::vector<std::string> &curName, const std::vector<std::string> &newName)
{
	auto getNameIndex = [&curName](const std::string &name) {
		int i = 0;
		for (; i < curName.size(); ++i)
			if (curName[i] == name)
				break;
		return i;
	};

	for (int m = 0; m < scene->mNumMaterials; ++m)
	{
		aiMaterial *mtl = scene->mMaterials[m];
		int nTex;
		if ((nTex = mtl->GetTextureCount(aiTextureType_DIFFUSE)) > 0)
		{
			aiString file;
			for (int i = 0; i < nTex; ++i)
			{
				if (mtl->GetTexture(aiTextureType_DIFFUSE, i, &file) == AI_SUCCESS)
				{
					int idx = getNameIndex(file.data);
					if (uint(idx) < newName.size())
					{
						aiString filex(newName[idx].c_str());
						printf("%s\n", filex.data);
						mtl->AddProperty(&filex, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, i));
					}
				}
			}
		}
	}
}



static void saveTextureImages(const std::string &modelDir, const std::vector<TexImage> &vTex, const std::vector<std::string> &vTexName)
{
	for (size_t i = 0; i<vTex.size(); ++i)
	{
		if (!vTex[i].img.empty())
		{
			std::string fileloc = modelDir + "/" + vTexName[i];
			if (!cv::imwrite(fileloc, vTex[i].img))
				FF_EXCEPTION(ERR_FILE_WRITE_FAILED, fileloc.c_str());
		}
	}
}

void _CVRModel::saveAs(const std::string &file, std::string fmtID, const std::string & options)
{
	if (scene)
	{
		if (fmtID.empty())
		{
			fmtID = ff::GetFileExtention(file);
			ff::str2lower(fmtID);
		}

		ff::ArgSet args(options);
		bool isSTD = args.get<bool>("std");

		std::vector<std::string>  curTexName, newTexName;
		getSavedTextureName(file, this->vTex, curTexName, newTexName, isSTD);
		if (isSTD)
			setTextureName(scene, curTexName, newTexName);

		Assimp::Exporter exp;
		if (AI_SUCCESS != exp.Export(scene, fmtID, file))
		{
			FF_EXCEPTION(ERR_FILE_WRITE_FAILED, file.c_str());
		}

		if (isSTD)
			setTextureName(scene, newTexName, curTexName);

		std::string newDir = ff::GetDirectory(file);

		saveTextureImages(newDir, vTex, newTexName);
	}
}

GLuint loadGLTexture(const Mat3b &img)
{
	GLuint texID;
	glGenTextures(1, &texID);

	//printf("texID=%d, width=%d, height=%d\n", texID, t.img.cols,t.img.rows);

	// Binding of texture name
	glBindTexture(GL_TEXTURE_2D, texID);
	// redefine standard texture values
	// We will use linear interpolation for magnification filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// We will use linear interpolation for minifying filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// Texture specification
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols,
		img.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, img.data);
	// we also want to be able to deal with odd texture dimensions
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

static void loadGLTextures(const std::vector<TexImage> &vTexImages, TexMap &mTex)
{
	mTex.clear();
	for (auto &t : vTexImages)
	{
		if (!t.img.empty()) /* If no error occurred: */
		{
			mTex[t.file].texID = loadGLTexture(t.img);
		}
	}
}

void _CVRModel::_loadTextures()
{
	if (!vTex.empty() && _texMap.empty())
	{
		loadGLTextures(vTex, _texMap);

		checkGLError();
	}
}

void color4_to_float4(const  aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}


bool apply_material(const aiMaterial *mtl, TexMap &mTex, int flags)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	aiColor4D diffuse;
	aiColor4D specular;
	aiColor4D ambient;
	aiColor4D emission;
	ai_real shininess, strength;
	int two_sided;
	int wireframe;
	unsigned int max;	// changed: to unsigned

	int texIndex = 0;
	aiString texPath;	//contains filename of texture

	bool hasTexture = false;

	if ((flags&CVRM_ENABLE_TEXTURE) && AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
	{
		//bind texture
		unsigned int texId = mTex[texPath.data].texID;
		glBindTexture(GL_TEXTURE_2D, texId);

		hasTexture = true;
	}

	if (!(flags&CVRM_ENABLE_MATERIAL))
		return hasTexture;

	set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		color4_to_float4(&diffuse, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
		color4_to_float4(&specular, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

	set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
		color4_to_float4(&ambient, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
		color4_to_float4(&emission, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

	max = 1;
	ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
	max = 1;
	ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
	if ((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
	else {
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
	}

	max = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
		fill_mode = wireframe ? GL_LINE : GL_FILL;
	else
		fill_mode = GL_FILL;
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	max = 1;
	if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	return hasTexture;
}

void Color4f(const aiColor4D *color)
{
	glColor4f(color->r, color->g, color->b, color->a);
}


void recursive_render(const aiScene *sc, TexMap &mTex, const struct aiNode* nd, int flags)
{
	unsigned int i;
	unsigned int n = 0, t;
	aiMatrix4x4 m = nd->mTransformation;

	// update transform
	m.Transpose();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		glBindTexture(GL_TEXTURE_2D, 0);
		bool hasTexture = apply_material(sc->mMaterials[mesh->mMaterialIndex], mTex, flags);
		bool onlyTexture = hasTexture && (flags&CVRM_TEXTURE_NOLIGHTING);

		if (mesh->mNormals && (flags&CVRM_ENABLE_LIGHTING) && !onlyTexture)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);

		if (mesh->mColors[0])
			glEnable(GL_COLOR_MATERIAL);
		else
			glDisable(GL_COLOR_MATERIAL);

		for (t = 0; t < mesh->mNumFaces; ++t)
		{
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch (face->mNumIndices)
			{
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for (i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				int vertexIndex = face->mIndices[i];	// get group index for current index
														//if (mesh->mColors[0] != NULL)
														//	Color4f(&mesh->mColors[0][vertexIndex]);

				if (onlyTexture)
					glColor4f(1, 1, 1, 1);
				else
				{
					if (mesh->mColors[0])
						glColor4fv((float*)&mesh->mColors[0][vertexIndex]);

					if (mesh->mNormals)
						glNormal3fv(&mesh->mNormals[vertexIndex].x);
				}

				if ((flags&CVRM_ENABLE_TEXTURE) && mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
				{
					glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
				}

				glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}
			glEnd();
		}
	}

	//if (glGetError() != 0)
	//	printf("GL error\n");

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, mTex, nd->mChildren[n], flags);
	}

	glPopMatrix();
}


void _CVRModel::render(int flags)
{
#if 1
	if (_sceneList == 0)
	{
		_sceneList = glGenLists(1);
		_sceneListRenderFlags = flags - 1;//make them unequal to trigger re-compile
	}

	if (_sceneListRenderFlags != flags)
	{
		glNewList(_sceneList, GL_COMPILE);
		/* now begin at the root node of the imported data and traverse
		the scenegraph by multiplying subsequent local transforms
		together on GL's matrix stack. */
		recursive_render(scene, _texMap, scene->mRootNode, flags);
		glEndList();
		_sceneListRenderFlags = flags;
	}

	glCallList(_sceneList);

	checkGLError();
#else
	recursive_render(scene, _texMap, scene->mRootNode, flags);
#endif
}

//==========================================================

struct ShowImageEvent
{
	CVRShowModelBase *winData;
	CVRResult  result;
};
static std::list<ShowImageEvent>  g_showImageEvents;
static std::mutex				  g_showImageMutex;

void showImageProc()
{
	while (true)
	{
		ShowImageEvent evt;
		evt.winData = nullptr;

		{
			std::lock_guard<std::mutex> lock(g_showImageMutex);

			if (!g_showImageEvents.empty())
			{
				evt = g_showImageEvents.front();
				g_showImageEvents.pop_front();
			}
		}

		if (evt.winData)
		{
			//if(!evt.result.img.empty())
				evt.winData->present(evt.result);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
		}
	}
}

void postShowImage(CVRShowModelBase *winData)
{
	static bool showImageThreadStarted = false;
	if (!showImageThreadStarted)
	{
		std::thread t(showImageProc);
		t.detach();
		showImageThreadStarted = true;
	}
	{
		std::lock_guard<std::mutex> lock(g_showImageMutex);
		g_showImageEvents.push_back({ winData, winData->currentResult});
	}
}

