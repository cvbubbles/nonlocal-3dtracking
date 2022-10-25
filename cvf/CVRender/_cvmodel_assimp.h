#pragma once

#include"_cvrender.h"



template<typename _OpT>
void _forNodeVetices(const aiScene *sc, const struct aiNode* nd, aiMatrix4x4 &prevT, _OpT &op)
{
	aiMatrix4x4 T = prevT;
	aiMultiplyMatrix4(&T, &nd->mTransformation);

	for (uint n = 0; n < nd->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		std::unique_ptr<char[]> _vmask(new char[mesh->mNumVertices]);
		char *vmask = _vmask.get();
		memset(vmask, 0, mesh->mNumVertices);

		for (uint t = 0; t < mesh->mNumFaces; ++t)
		{
			const struct aiFace* face = &mesh->mFaces[t];
			for (uint i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				vmask[face->mIndices[i]] = 1;
			}
		}
		for (uint i = 0; i < mesh->mNumVertices; ++i)
		{
			if (vmask[i] != 0)
			{
				aiVector3D v = mesh->mVertices[i];
				aiTransformVecByMatrix4(&v, &T);
				op(v);
			}
		}
	}

	for (uint n = 0; n < nd->mNumChildren; ++n)
	{
		_forNodeVetices(sc, nd->mChildren[n], T, op);
	}
}

template<typename _OpT>
void _forAllVetices(const aiScene *sc, _OpT op)
{
	aiMatrix4x4 T;
	aiIdentityMatrix4(&T);
	_forNodeVetices(sc, sc->mRootNode, T, op);
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

//void _CVRModel::_loadExtFile(const std::string &file, ff::ArgSet &args)
//{
//	cv::FileStorage xfs(file, FileStorage::READ);
//
//	Mat pose0;
//	xfs["pose0"] >> pose0;
//	this->setSceneTransformation(Matx44f(pose0), false);
//}

template<typename _ValT>
inline bool readStorage(cv::FileStorage &fs, const std::string &var, _ValT &val) {
	if (!fs.isOpened())
		return false;

	try {
		fs[var] >> val;
	}
	catch (...)
	{
		return false;
	}
	return true;
}

void _CVRModel::load(const std::string &file, int postProLevel, const std::string &options)
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

	_sceneTransformInFile = scene->mRootNode->mTransformation;
	//_sceneTransform = cvrm::I();

	ff::CommandArgSet args(options);

	cv::FileStorage xfs;
	std::string extFile = args.getd<std::string>("extFile", "");
	if (extFile.empty())
		extFile = ff::ReplacePathElem(file, "yml", ff::RPE_FILE_EXTENTION);
	if (ff::pathExist(extFile))
		xfs.open(extFile, FileStorage::READ);

	if (args.getd<bool>("setPose0", false))
	{
		Mat pose0;
		if (!readStorage(xfs, "pose0", pose0))
			pose0 = Mat(this->calcStdPose());

		this->setSceneTransformation(Matx44f(pose0), false);
	}

	this->_updateSceneInfo();

	loadTextureImages(scene, ff::GetDirectory(file), vTex);
}

void get_bounding_box(const aiScene *scene, aiVector3D* min, aiVector3D* max)
{
	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;

	/*double c[3] = { 0,0,0 };
	int n = 0;*/

	_forAllVetices(scene, [min, max/*, &c, &n*/](aiVector3D &tmp) {
		min->x = __min(min->x, tmp.x);
		min->y = __min(min->y, tmp.y);
		min->z = __min(min->z, tmp.z);

		max->x = __max(max->x, tmp.x);
		max->y = __max(max->y, tmp.y);
		max->z = __max(max->z, tmp.z);

		//c[0] += tmp.x; c[1] += tmp.y; c[2] += tmp.z;
		//++n;
	});
}

void _CVRModel::_updateSceneInfo()
{
	get_bounding_box(scene, &scene_min, &scene_max);
	scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
	scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
	scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
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
					glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1.0 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
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
	//_initGL();
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


void _getAllVetices(const aiScene *sc, std::vector<Vec3f> &vtx)
{
	uint nMaxVertices = 0;
	for (uint i = 0; i < sc->mNumMeshes; ++i)
		nMaxVertices += sc->mMeshes[i]->mNumVertices;

	vtx.clear();
	vtx.reserve(nMaxVertices);

	_forAllVetices(sc, [&vtx](aiVector3D &v) {
		vtx.push_back(Vec3f(v.x, v.y, v.z));
	});
}

const std::vector<Vec3f>& _CVRModel::getVertices()
{
	if (_vertices.empty())
		_getAllVetices(scene, _vertices);
	return _vertices;
}

#if 1
void _getMeshEx(const aiScene *sc, const struct aiNode* nd, aiMatrix4x4 &prevT, CVRMesh &gmesh, bool hasNormals)
{
	aiMatrix4x4 T = prevT;
	aiMultiplyMatrix4(&T, &nd->mTransformation);

	aiMatrix3x3 R = (aiMatrix3x3)T;

	const uint maxFaceIndices = gmesh.faces.size();

	for (uint n = 0; n < nd->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		int idxBase = (int)gmesh.vertices.size();

		for (uint i = 0; i < mesh->mNumVertices; ++i)
		{
			aiVector3D v = mesh->mVertices[i];
			aiTransformVecByMatrix4(&v, &T);
			gmesh.vertices.push_back(Vec3f(v.x, v.y, v.z));

			if (hasNormals)
			{
				v = mesh->mNormals[i];
				aiTransformVecByMatrix3(&v, &R);
				gmesh.normals.push_back(Vec3f(v.x, v.y, v.z));
			}
		}

		gmesh.verticesMask.resize(idxBase + mesh->mNumVertices, 0);
		for (uint t = 0; t < mesh->mNumFaces; ++t)
		{
			const struct aiFace* face = &mesh->mFaces[t];
			if (face->mNumIndices < maxFaceIndices)
			{
				auto &f = gmesh.faces[face->mNumIndices];
				for (uint i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
				{
					int j = idxBase + (int)face->mIndices[i];
					f.indices.push_back(j);
					gmesh.verticesMask[j] = 1;
				}
			}
		}
	}

	for (uint n = 0; n < nd->mNumChildren; ++n)
	{
		_getMeshEx(sc, nd->mChildren[n], T, gmesh, hasNormals);
	}
}
void _getMesh(const aiScene *sc, CVRMesh &mesh)
{
	uint nMaxVertices = 0;
	uint nMaxFaces = 0;
	bool hasNormals = true;
	for (uint i = 0; i < sc->mNumMeshes; ++i)
	{
		nMaxVertices += sc->mMeshes[i]->mNumVertices;
		nMaxFaces += sc->mMeshes[i]->mNumFaces;
		hasNormals &= sc->mMeshes[i]->HasNormals();
	}

	mesh.clear();
	mesh.vertices.reserve(nMaxVertices);
	mesh.faces.resize(16);
	mesh.faces[3].indices.reserve(3 * nMaxFaces);

	aiMatrix4x4 T;
	aiIdentityMatrix4(&T);
	_getMeshEx(sc, sc->mRootNode, T, mesh, hasNormals);
}

void _CVRModel::getMesh(CVRMesh &mesh, int flags)
{
	_getMesh(this->scene, mesh);
}
#endif

Matx44f _CVRModel::calcStdPose()
{
	const std::vector<Vec3f>  &vtx = this->getVertices();

	Mat mvtx(vtx.size(), 3, CV_32FC1, (void*)&vtx[0]);
	cv::PCA pca(mvtx, noArray(), PCA::DATA_AS_ROW);

	Vec3f mean = pca.mean;
	Matx33f ev = pca.eigenvectors;

	//swap x-y so that the vertical directional is the longest dimension
	Vec3f vx(-ev(1, 0), -ev(1, 1), -ev(1, 2));
	Matx44f R = cvrm::rotate(&vx[0], &ev(0, 0), &ev(2, 0));
	if (determinant(R) < 0)
	{//reverse the direction of Z axis
		for (int i = 0; i < 4; ++i)
			R(i, 2) *= -1;
	}
	//std::cout << "det=" << determinant(R) << std::endl;

	return cvrm::translate(-mean[0], -mean[1], -mean[2])*R;
}


void _transformNodeVetices(aiScene *sc, aiNode* nd, aiMatrix4x4 &prevT)
{
	aiMatrix4x4 T = prevT;
	aiMultiplyMatrix4(&T, &nd->mTransformation);

	for (uint n = 0; n < nd->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		std::unique_ptr<char[]> _vmask(new char[mesh->mNumVertices]);
		char *vmask = _vmask.get();
		memset(vmask, 0, mesh->mNumVertices);

		for (uint t = 0; t < mesh->mNumFaces; ++t)
		{
			const struct aiFace* face = &mesh->mFaces[t];
			for (uint i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				vmask[face->mIndices[i]] = 1;
			}
		}
		for (uint i = 0; i < mesh->mNumVertices; ++i)
		{
		//	if (vmask[i] != 0)
			{
				aiVector3D &v = mesh->mVertices[i];
				aiTransformVecByMatrix4(&v, &T);
			}
		}

		if (mesh->mNormals)
		{
			aiMatrix3x3 R(T.a1, T.a2, T.a3, T.b1, T.b2, T.b3, T.c1, T.c2, T.c3);

			for (uint i = 0; i < mesh->mNumVertices; ++i)
			{
				aiTransformVecByMatrix3(&mesh->mNormals[i], &R);
			}
		}
	}
	
	aiIdentityMatrix4(&nd->mTransformation); // set node transformation as identity

	for (uint n = 0; n < nd->mNumChildren; ++n)
	{
		_transformNodeVetices(sc, nd->mChildren[n], T);
	}
}

void _transformAllVetices(aiScene *sc, aiMatrix4x4 &T)
{
	_transformNodeVetices(sc, sc->mRootNode, T);
}

void _CVRModel::setSceneTransformation(const Matx44f &trans, bool updateSceneInfo)
{
	aiMatrix4x4 m;
	static_assert(sizeof(m) == sizeof(trans), "");
	memcpy(&m, &trans, sizeof(m));
	m.Transpose();

	_transformAllVetices(scene, m);
	//aiIdentityMatrix4(&this->_sceneTransform);
	aiIdentityMatrix4(&this->_sceneTransformInFile);


	//aiMultiplyMatrix4(&m, &_sceneTransformInFile);
	//scene->mRootNode->mTransformation = m;

	if (updateSceneInfo)
		this->_updateSceneInfo();

	_vertices.clear();
}



