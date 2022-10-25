#pragma once

#include"_cvrender.h"
#include"BFC/portable.h"
#include <wrap/io_trimesh/import.h>

typedef std::shared_ptr<CMeshO>  _MeshPtr;

static void loadTextureImages(_MeshPtr scene, const std::string &modelDir, std::vector<TexImage> &vTex)
{
	vTex.clear();

	for (size_t i = 0; i<scene->textures.size(); i++)
	{
		std::string fileloc = modelDir + "/" + scene->textures[i];	/* Loading of image */
		cv::Mat img = cv::imread(fileloc, IMREAD_ANYCOLOR);

		if (!img.empty()) /* If no error occurred: */
		{
			makeSizePower2(img);

			vTex.push_back({ scene->textures[i],img });
		}
		else
		{
			/* Error occurred */
			printf("error: failed to load %s\n", fileloc.c_str());
		}
	}
}

void _CVRModel::load(const std::string &file, int postProLevel, const std::string &options)
{
	std::shared_ptr<CMeshO> mesh(new CMeshO);
	
	std::string cwd = ff::getCurrentDirectory();
	std::string ddir = ff::GetDirectory(file);
	ff::setCurrentDirectory(ddir);
	int err = 0;
	try
	{
		//mesh->face.EnableWedgeTexCoord();
		mesh->vert.EnableTexCoord();
		//bool b = HasPerWedgeTexCoord(*mesh);
		err = vcg::tri::io::Importer<CMeshO>::Open(*mesh, file.c_str());
	}
	catch(...)
	{
		err = -1;
	}
	ff::setCurrentDirectory(cwd);

	//auto p = mesh->vert[0].cP();

	if (err)
		FF_EXCEPTION(ERR_FILE_OPEN_FAILED, file.c_str());
	else
		this->clear();

	scene = mesh;
	sceneFile = file;

	_sceneTransformInFile = mesh->Tr;
	_sceneTransform = cvrm::I();

	this->_updateSceneInfo();

	loadTextureImages(scene, ff::GetDirectory(file), vTex);
}

void _CVRModel::saveAs(const std::string &file, std::string fmtID, const std::string & options)
{

}

template<typename _OpT>
void _forAllVetices(_MeshPtr sc, _OpT op)
{
	for (auto &m : sc->vert)
		op(m.cP());
}

void get_bounding_box(_MeshPtr scene, Point3f* min, Point3f* max)
{
	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;

	_forAllVetices(scene, [min, max](const vcg::Point3<float> &tmp) {
		min->x = __min(min->x, tmp.X());
		min->y = __min(min->y, tmp.Y());
		min->z = __min(min->z, tmp.Z());

		max->x = __max(max->x, tmp.X());
		max->y = __max(max->y, tmp.Y());
		max->z = __max(max->z, tmp.Z());
	});
}

void _CVRModel::_updateSceneInfo()
{
	get_bounding_box(scene, &scene_min, &scene_max);
	scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
	scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
	scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
}


void _render(_MeshPtr sc, TexMap &mTex, int flags)
{
	unsigned int i;
	unsigned int n = 0, t;

	// draw all meshes assigned to this node
	//for (; n < nd->mNumMeshes; ++n)
	{
	//	const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		glBindTexture(GL_TEXTURE_2D, 0);
		bool hasTexture = false;
		if (!mTex.empty())
		{
			for (auto &t : mTex)
				glBindTexture(GL_TEXTURE_2D, t.second.texID);
			hasTexture = true;
		}
		//bool hasTexture = apply_material(sc->mMaterials[mesh->mMaterialIndex], mTex, flags);
		bool onlyTexture = hasTexture && (flags&CVRM_TEXTURE_NOLIGHTING);

		/*if (mesh->mNormals && (flags&CVRM_ENABLE_LIGHTING) && !onlyTexture)
			glEnable(GL_LIGHTING);
		else*/
			glDisable(GL_LIGHTING);

		/*if (mesh->mColors[0])
			glEnable(GL_COLOR_MATERIAL);
		else*/
			glDisable(GL_COLOR_MATERIAL);

		for (t = 0; t < sc->face.size(); ++t)
		{
			auto &f = sc->face[t];

			glBegin(GL_TRIANGLES);

			for (i = 0; i < 3; i++)		// go through all vertices in face
			{
				//int vertexIndex = f.V(i);	// get group index for current index
														//if (mesh->mColors[0] != NULL)
														//	Color4f(&mesh->mColors[0][vertexIndex]);

				//if (onlyTexture)
					glColor4f(1, 1, 1, 1);
				/*else
				{
					if (mesh->mColors[0])
						glColor4fv((float*)&mesh->mColors[0][vertexIndex]);

					if (mesh->mNormals)
						glNormal3fv(&mesh->mNormals[vertexIndex].x);
				}*/

				//if ((flags&CVRM_ENABLE_TEXTURE) && mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
				//{
				//	glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1.0 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
				//}
					auto p = f.V(i);
				//	auto tc=p->HasTexCoord();
				//auto uv = f.V(i)->cT();
					auto uv = sc->vert[p->Index()].cT();
				//	auto uv = f.WT(i);
				glTexCoord2f(uv.u(), 1 - uv.v());

				auto pt = f.V(i)->cP();
				glVertex3f(pt.X(),pt.Y(),pt.Z());
				
			}
			glEnd();
		}
	}

	//if (glGetError() != 0)
	//	printf("GL error\n");

	// draw all children
	/*for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, mTex, nd->mChildren[n], flags);
	}*/

	//glPopMatrix();
}

void _CVRModel::render(int flags)
{
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
		_render(scene, _texMap, flags);
		glEndList();
		_sceneListRenderFlags = flags;
	}

	glCallList(_sceneList);

	checkGLError();
}

const std::vector<Vec3f>& _CVRModel::getVertices()
{
	return std::vector<Vec3f>();
}

void  _CVRModel::getMesh(CVRMesh &mesh, int flags)
{}

Matx44f _CVRModel::calcStdPose()
{
	return cvrm::I();
}

void  _CVRModel::setSceneTransformation(const Matx44f &trans, bool updateSceneInfo)
{}

