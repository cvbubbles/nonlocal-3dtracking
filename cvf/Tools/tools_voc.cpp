
//#include<Windows.h>
//#include <WinNetWk.h>

#include"EDK/cmds.h"
#include"BFC/portable.h"
#include"BFC/stdf.h"
#include"BFC/err.h"
#include<fstream>
#include<time.h>
using namespace std;
using namespace ff;
#include"appstd.h"

_CMDI_BEG

void loadImageList(const std::vector<std::string> &listFiles, std::vector<std::string> &imageFiles)
{
	std::string dir, file;

	for (auto &flist : listFiles)
	{
		std::ifstream is(flist);
		if (!is)
			throw flist;
		dir = ff::GetDirectory(flist);
		while (is >> file)
			imageFiles.push_back(dir + file);
	}
}

Mat1b getRenderMask(const Mat1f &depth, float eps=1e-6f)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar &m) {
		m = fabs(1.0f - d)<eps ? 0 : 255;
	});
	return mask;
}

template<typename _ValT, int cn, typename _AlphaValT>
inline void alphaBlendX(const Mat_<Vec<_ValT, cn> > &F, const Mat_<_AlphaValT> &alpha, double alphaScale, Mat_<Vec<_ValT, cn> > B)
{
	for_each_4(DWHNC(F), DN1(alpha), DNC(B), DNC(B), [alphaScale](const _ValT *f, _AlphaValT a, const _ValT *b, _ValT *c) {
		double w = a * alphaScale;
		for (int i = 0; i < cn; ++i)
			c[i] = _ValT(b[i] + w*(f[i] - b[i]));

	});
}
Rect randBox(Size imgSize, int minSize, int maxSize)
{
	maxSize = __min(int(__min(imgSize.width, imgSize.height)*0.8), maxSize);
	Rect b;
	while (true)
	{
		int r = minSize + rand() % (maxSize - minSize);
		r /= 2;
		int cx = rand() % imgSize.width, cy = rand() % imgSize.height;
		if (cx - r >= 0 && cy - r >= 0 && cx + r < imgSize.width && cy + r < imgSize.height)
		{
			b = Rect(cx - r, cy - r, r * 2, r * 2);
			break;
		}
	}
	return b;
}
Rect randBox(Size imgSize, int minSize, int maxSize, const Mat1i &dmask, double maxOverlapp=0.2, int maxTry=10)
{
	maxSize = __min(int(__min(imgSize.width, imgSize.height)*0.8), maxSize);
	Rect b;
	double mr = 10;
	int n = 0;
	while (true)
	{
		int r = minSize + rand() % (maxSize - minSize);
		r /= 2;
		int cx = rand() % imgSize.width, cy = rand() % imgSize.height;
		if (cx - r >= 0 && cy - r >= 0 && cx + r < imgSize.width && cy + r < imgSize.height)
		{
			Rect bx = Rect(cx - r, cy - r, r * 2, r * 2);

			int nMasked = 0;
			for_each_1(DWHN1r(dmask, bx), [&nMasked](int &mi) {
				if (mi >= 0)
					++nMasked;
			});

			double r = double(nMasked) / (bx.width*bx.height);
			if (r < mr)
			{
				mr = r;
				b = bx;
			}
			++n;
			if(r< maxOverlapp || n>maxTry)
				break;
		}
	}
	return b;
}

Mat degrade(const Mat &img, float smoothSigma, Size smoothSize, float noiseStd)
{
	Mat imgf;
	img.convertTo(imgf, CV_32F);

	GaussianBlur(imgf, imgf, smoothSize, smoothSigma);

	Mat noise(img.size(), CV_MAKE_TYPE(CV_32F, img.channels()));
	cv::randn(noise, 0, noiseStd);

	imgf += noise;

	Mat dimg;
	imgf.convertTo(dimg, img.depth());
	return dimg;
}
Mat degradeRand(const Mat &img, float maxSmoothSigma, float maxNoiseStd)
{
	float sigma = float(rand()) / RAND_MAX*maxSmoothSigma;
	float size = __max(int(sigma * 6 - 1) | 1, 3);

	return degrade(img, sigma, Size(size, size), float(rand()) / RAND_MAX*maxNoiseStd);
}

void transformF(Mat3b &F, const Mat1b &mask, const Mat3b &B, double saturationR=0.3)
{
	Mat3b Fx, Bx;
	cvtColor(F, Fx, CV_BGR2HSV);
	cvtColor(B, Bx, CV_BGR2HSV);

	Vec3d fm(0,0,0), bm(0,0,0);
	int fn = 0, bn = 0;
	for_each_2(DWHN0(Fx), DN1(mask), [&fm, &fn](const Vec3b &f, uchar mv) {
		if (mv)
			fm += f, ++fn;
	});

	for_each_1(DWHN0(Bx), [&bm, &bn](const Vec3b &b) {
		bm += b, ++bn;
	});

	fm *= 1.0 / fn;
	bm *= 1.0 / bn;

	double ss = pow(bm[1] / fm[1], saturationR), sv = bm[2] / fm[2];

	for_each_1(DWHNC(Fx), [ss, sv](uchar *p) {
		p[1] = uchar(__min(p[1] * ss,255.0) + 0.5);
		p[2] = uchar(__min(p[2] * sv,255.0) + 0.5);
	});
	cvtColor(Fx, F, CV_HSV2BGR);
}

Rect composite(Mat3b F, Mat1b mask, Mat3b B, Rect roi, float maxSmoothSigma=1.0f, float maxNoiseStd=5.0f)
{
	Mat1f fmask;
	mask.convertTo(fmask, CV_32F, 1.0 / 255);
	GaussianBlur(fmask, fmask, Size(3, 3), 1);

	Mat1b tmask(mask.size()), dmask;
	for_each_2(DWHN1(mask), DN1(tmask), [](uchar m, uchar &t) {
		t = m > 127 ? 255 : 0;
	});
	maxFilter(tmask, dmask, 3);

	transformF(F, tmask, B);

	Rect accurateROI=get_mask_roi(DWHN(tmask));

	int hwsz = 2, fstride=stepC(F),tmstride=stepC(tmask);
	Rect r(hwsz, hwsz, F.cols - hwsz * 2, F.rows - hwsz * 2);
	for_each_3(DWHNCr(F,r), DNCr(tmask,r), DN1r(dmask,r), [hwsz,fstride,tmstride](uchar *p0, uchar *tm, uchar dm) {
		if (dm > *tm)
		{
			uchar *p = p0 - fstride*hwsz - 3 * hwsz;
			tm = tm - tmstride*hwsz - hwsz;
			const int wsz = hwsz * 2 + 1;
			int clr[3] = { 0,0,0 };
			int nc = 0;
			for(int y=0; y<wsz;++y, tm+=tmstride, p+=fstride)
				for (int x = 0; x < wsz; ++x)
				{
					if (tm[x] != 0)
					{
						const uchar *c = p + 3 * x;
						clr[0] += c[0]; clr[1] += c[1]; clr[2] += c[2]; ++nc;
					}
				}
			if (nc != 0)
			{
				for (int i = 0; i < 3; ++i)
					p0[i] = clr[i] / nc;
			}
		}
	});

	F=degradeRand(F, maxSmoothSigma, maxNoiseStd);

	//imshow("F", F);
	//imshow("Fm", fmask);
	//GaussianBlur(F, F, Size(3, 3), 1.0);
	alphaBlendX(F, fmask, 1.0, B(roi));
	//GaussianBlur(mask, mask, Size(5, 5), 1.0);
	//alphaBlendX(F, mask, 1.0/255, B(roi));

	return Rect(roi.x + accurateROI.x, roi.y + accurateROI.y, accurateROI.width, accurateROI.height);
}

struct DObject
{
	int  objID;
	std::string name;
	Rect   boundingBox;
};

class Annotations
{
public:
	/*<folder>VOC2012< / folder>
		<filename>2007_000648.jpg< / filename>
		<source>
		<database>The VOC2007 Database< / database>
		<annotation>PASCAL VOC2007< / annotation>
		<image>flickr< / image>
		< / source>*/
	std::string folder;
	std::string database;
	std::string annotation;
	std::string image;
private:
	std::string _imgPath;
	std::string _xmlPath;
public:
	Annotations()
	{}
	Annotations(std::string _folder,	std::string _database, std::string _annotation, std::string _image)
		:folder(_folder),database(_database),annotation(_annotation),image(_image)
	{}
	void setDPath(const std::string &dpath, bool clear = false)
	{
		std::string imgPath = dpath + "/" + "JPEGImages/";
		std::string xmlPath = dpath + "/" + "Annotations";
		
		if (clear)
		{
			removeDirectoryRecursively(imgPath);
			removeDirectoryRecursively(xmlPath);
		}
		if (!pathExist(imgPath))
			makeDirectory(imgPath);
		if (!pathExist(xmlPath))
			makeDirectory(xmlPath);
		_imgPath = imgPath;
		_xmlPath = xmlPath;
	}
	void saveXML(const std::string &xmlFile, const std::string &imgFile, const Mat &img, const std::vector<DObject> &objs)
	{
		static const char *headFormat = 
R"(<annotation>
	<folder>%s</folder>
	<filename>%s</filename>
	<source>
		<database>%s</database>
		<annotation>%s</annotation>
		<image>%s</image>
	</source>
	<size>
		<width>%d</width>
		<height>%d</height>
		<depth>%d</depth>
	</size>
	<segmented>0</segmented>
)";

		FILE *fp = fopen(xmlFile.c_str(), "w");
		if (!fp)
			throw xmlFile;

		fprintf(fp, headFormat, folder.c_str(), ff::GetFileName(imgFile).c_str(), database.c_str(), annotation.c_str(), image.c_str(), img.cols, img.rows, img.channels());

		static const char *objectFormat = 
R"(	<object>
		<name>%s</name>
		<pose>Unspecified</pose>
		<truncated>0</truncated>
		<difficult>0</difficult>
		<bndbox>
			<xmin>%d</xmin>
			<ymin>%d</ymin>
			<xmax>%d</xmax>
			<ymax>%d</ymax>
		</bndbox>
	</object>
)";

		for (auto &obj : objs)
		{
			auto &bb = obj.boundingBox;
			fprintf(fp, objectFormat, obj.name.c_str(), bb.x, bb.y, bb.x + bb.width, bb.y + bb.height);
		}
		fprintf(fp, "</annotation>");

		fclose(fp);
	}
	void save(const std::string &fileBaseName, const Mat &img, const std::vector<DObject> &objs)
	{
		auto imgFile = _imgPath + "/" + fileBaseName + ".jpg";
		imwrite(imgFile, img);

		auto xmlFile = _xmlPath + "/" + fileBaseName + ".xml";
		saveXML(xmlFile, imgFile, img, objs);
	}
};

void saveLabelMap(const std::string &file, const std::vector<std::string> &labels)
{
	const char *fmt =
R"(item {
  name: "%s"
  label: %d
  display_name: "%s"
}
)";

	FILE *fp = fopen(file.c_str(), "w");
	if (!fp)
		throw file;
	fprintf(fp,fmt, "none_of_the_above", 0, "background");
	int id = 0;
	for (auto &s : labels)
		fprintf(fp, fmt, s.c_str(), ++id, s.c_str());
	fclose(fp);
}
//
//class RenderVOC
//	:public ICommand
//{
//public:
//	virtual void exec0(std::string dataDir, std::string args)
//	{
//		dataDir = D_DATA + "/re3d/3ds-model/";
//
//		std::vector<std::string> modelFiles = { "bottle2","plane","Sedan","car1","cup1"};
//		for (auto &f : modelFiles)
//			f = dataDir + f + "/" + f + ".3ds";
//
//		std::vector<std::string> bgImageList = {
//			R"(F:\store\datasets\seg_dataset\ADE20K_2016_07_26\images\list.txt)",
//			R"(F:\dataset\flickr30k_images\list.txt)"
//		};
//
//		std::vector<std::string>  bgImageFiles;
//		loadImageList(bgImageList, bgImageFiles);
//
//		std::vector<CVRModel>  models(modelFiles.size());
//		//std::vector<CVRender>  renders(modelFiles.size());
//		for (size_t i = 0; i < modelFiles.size(); ++i)
//		{
//			models[i].load(modelFiles[i]);
//			//renders[i] = CVRender(models[i]);
//		}
//
//		std::vector<int>  modelIndex;
//		for (int i = 0; i < (int)models.size(); ++i)
//			modelIndex.push_back(i);
//
//		int N = 100;
//		for (int n = 0; n < N;)
//		{
//			Mat bgImg = imread(bgImageFiles[rand() % bgImageFiles.size()]);
//			if (bgImg.empty())
//				continue;
//			else
//				++n;
//
//			double scale = 800.0/__max(bgImg.rows, bgImg.cols);
//			Size dsize(int(bgImg.cols*scale), int(bgImg.rows*scale));
//			resize(bgImg, bgImg, dsize);
//
//			std::random_shuffle(modelIndex.begin(), modelIndex.end());
//			int nModels = rand() % (int)modelIndex.size()+1;
//
//			const float eyeZ = 4.0f;
//			CVRMats mats(bgImg.size(), 1.5, eyeZ);
//
//			CVRProjector prj(mats.mView, mats.mProjection, bgImg.size());
//
//			CVRModelArray scene(nModels);
//			for (int i = 0; i < nModels; ++i)
//			{
//				scene[i].model = models[modelIndex[i]];
//				scene[i].mModeli = scene[i].model.getUnitize();
//
//				Rect bb = randBox(bgImg.size(), 100, 400);
//
//				Point3f C=prj.unproject(bb.x + bb.width / 2, bb.y + bb.height / 2, 0.5);
//				Point3f L = prj.unproject(bb.x, bb.y + bb.height / 2, 0.5);
//				Point3f R = prj.unproject(bb.x + bb.width, bb.y + bb.height / 2, 0.5);
//				double scale = norm(L - R) / 2.0;
//				double angle = rand() / 1000.0;
//				scene[i].mModel = cvrm::scale(scale, scale, scale)*cvrm::rotate(angle, Vec3f(1, 0, 0))*cvrm::translate(C.x, C.y, C.z);
//
//				cv::rectangle(bgImg, bb, Scalar(0, 255, 255), 2);
//			}
//
//
//			CVRender render(scene);
//			
//			render.setBgImage(bgImg);
//			auto r=render.exec(mats, bgImg.size());
///*
//			Mat curImg = bgImg.clone();
//			for (int i = 0; i < nModels; ++i)
//			{
//				CVRModel &modeli = models[modelIndex[i]];
//				CVRender &renderi = renders[modelIndex[i]];
//
//				CVRMats mats(modeli,bgImg.size());
//				renderi.setBgImage(curImg);
//				CVRResult r=renderi.exec(mats, bgImg.size());
//				curImg = r.img;
//			}*/
//			imshow("img", r.img);
//			waitKey();
//		}
//	}
//
//	virtual void exec_test_annot(std::string dataDir, std::string args)
//	{
//		//connectNetDrive("\\\\101.76.215.117\\f", "Z:", "fan", "fan123");
//
//		Annotations annot("Re3D","Re3D","VOC 2007","rendered");
//
//		std::vector<std::string> bgImageList = {
//			R"(F:\store\datasets\seg_dataset\ADE20K_2016_07_26\images\list.txt)",
//		};
//
//		std::vector<std::string>  bgImageFiles;
//		loadImageList(bgImageList, bgImageFiles);
//
//		std::string file = bgImageFiles.front();
//		Mat img = imread(file);
//
//		DObject obj;
//		obj.objID = 1;
//		obj.name = "test";
//		obj.boundingBox = Rect(2, 3, 100, 200);
//		std::vector<DObject> objs;
//		objs.push_back(obj);
//		objs.push_back(obj);
//
//		std::string dpath = R"(Z:/local/data/Re3D/Re3Da/)";
//		
//		annot.setDPath(dpath);
//
//		//annot.saveXML("./voc.xml", file, img, objs);
//		annot.save("001", img, objs);
//
//		std::vector<std::string> modelNames = { "bottle2","plane","Sedan","car1","cup1" };
//		saveLabelMap(dpath + "labelmap.prototxt", modelNames);
//	}
//	virtual void exec/*_to_gray*/(std::string dataDir, std::string args)
//	{
//		dataDir = R"(Z:\local\data\Re3D\Re3Dag\JPEGImages\)";
//
//		std::vector<string> files;
//		ff::listFiles(dataDir, files);
//
//		for (auto &f : files)
//		{
//			auto file = dataDir + f;
//			Mat img = imread(file, IMREAD_GRAYSCALE);
//			cvtColor(img, img, CV_GRAY2BGR);
//			imwrite(file, img);
//			printf("%s\n", f.c_str());
//		}
//	}
//	virtual void exec_list(std::string dataDir, std::string args)
//	{
//		dataDir = R"(Z:\local\data\Re3D\Re3Db\)";
//
//		std::string imgDir = "JPEGImages", xmlDir = "Annotations";
//		
//		std::vector<string> files;
//		ff::listFiles(dataDir + imgDir, files);
//
//		auto saveList = [imgDir, xmlDir](const string vfiles[], int count, std::string listFile) {
//			FILE *fp = fopen(listFile.c_str(),"w");
//			if (!fp)
//				throw listFile;
//
//			for (int i = 0; i < count; ++i)
//			{
//				std::string baseName = ff::GetFileName(vfiles[i], false);
//				fprintf(fp, "%s %s\n", (imgDir + "/" + vfiles[i]).c_str(), (xmlDir + "/" + baseName + ".xml").c_str());
//			}
//
//			fclose(fp);
//		};
//
//		auto createTestNameSize = [dataDir,imgDir](const string vfiles[], int count, string dfile) {
//			FILE *fp = fopen(dfile.c_str(), "w");
//			if (!fp)
//				throw dfile;
//
//			string dir = dataDir + imgDir + "/";
//			for (int i = 0; i < count; ++i)
//			{
//				Mat img = imread(dir + vfiles[i]);
//				fprintf(fp, "%s %d %d\n", ff::GetFileName(vfiles[i],false).c_str(), img.rows, img.cols);
//			}
//			fclose(fp);
//		};
//
//		int ntest = files.size() / 5;
//		saveList(&files[0], ntest, dataDir + "test.txt");
//		createTestNameSize(&files[0], ntest, dataDir + "test_name_size.txt");
//		saveList(&files[ntest], files.size() - ntest, dataDir + "trainval.txt");
//	}
//	virtual void exec_run(std::string dataDir, std::string args)
//	{
//		srand((int)time(NULL));
//
//		dataDir = D_DATA + "/re3d/3ds-model/";
//
//		std::vector<std::string> modelNames = { "bottle2","plane","Sedan","car1","cup1"};
//		std::vector<std::string> modelFiles=modelNames;
//		for (auto &f : modelFiles)
//			f = dataDir + f + "/" + f + ".3ds";
//
//		std::vector<std::string> bgImageList = {
//			R"(F:\store\datasets\seg_dataset\ADE20K_2016_07_26\images\list.txt)",
//			R"(F:\dataset\flickr30k_images\list.txt)"
//		};
//
//		std::vector<std::string>  bgImageFiles;
//		loadImageList(bgImageList, bgImageFiles);
//
//		std::vector<CVRModel>  models(modelFiles.size());
//		std::vector<CVRender>  renders(modelFiles.size());
//		for (size_t i = 0; i < modelFiles.size(); ++i)
//		{
//			models[i].load(modelFiles[i]);
//			renders[i] = CVRender(models[i]);
//		}
//
//		//rotation vectors
//		std::vector<Vec3f>  vdirs;
//		cvrm::sampleSphere(vdirs, 1000);
//
//		std::vector<int>  modelIndex;
//		for (int i = 0; i < (int)models.size(); ++i)
//			modelIndex.push_back(i);
//
//		Annotations annot("Re3D", "Re3D", "VOC 2007", "rendered");
//
//		std::string dpath = R"(Z:\local\data\Re3D\Re3Db\)";
//		annot.setDPath(dpath, true);
//
//		saveLabelMap(dpath + "labelmap.txt", modelNames);
//
//		int N = 20000;
//		for (int n = 0; n < N;)
//		{
//			Mat bgImg = imread(bgImageFiles[rand() % bgImageFiles.size()]);
//			if (bgImg.empty())
//				continue;
//			else
//				++n;
//
//			double scale = 800.0 / __max(bgImg.rows, bgImg.cols);
//			Size dsize(int(bgImg.cols*scale), int(bgImg.rows*scale));
//			resize(bgImg, bgImg, dsize);
//
//			std::random_shuffle(modelIndex.begin(), modelIndex.end());
//			int nModels = rand() % (int)modelIndex.size() + 1;
//
//			Mat3b dimg = bgImg.clone();
//			std::vector<DObject> objs;
//			for (int i = 0; i < nModels; ++i)
//			{
//				int mi = modelIndex[i];
//				CVRModel &modeli = models[mi];
//				CVRender &renderi = renders[mi];
//
//				Rect bb = randBox(bgImg.size(), 100, 400);
//				Size bbSize(bb.width, bb.height);
//
//				CVRMats mats(modeli, bbSize);
//				mats.mModel = cvrm::rotate(Vec3f(0, 0, 1), vdirs[rand() % vdirs.size()]);
//
//				CVRResult r = renderi.exec(mats, bbSize);
//				Mat1b mask = getRenderMask(r.depth);
//
//				bb=composite(r.img, mask, dimg, bb);
//				//cv::rectangle(dimg, bb, Scalar(0, 255, 255), 2);
//
//				DObject obj;
//				obj.objID = mi;
//				obj.name = modelNames[mi];
//				obj.boundingBox = bb;
//				objs.push_back(obj);
//			}
//
//			std::string fileBaseName = ff::StrFormat("%05d", n);
//			annot.save(fileBaseName, dimg, objs);
//
//			printf("%s\n", fileBaseName.c_str());
//
//			//imshow("img", dimg);
//			//waitKey();
//		}
//	}
//
//
//	template<typename _FilterT>
//	static void genImageList(const std::string &dir, const std::string &listFile, _FilterT isIncluded, bool recursive=true)
//	{
//		std::vector<string> files;
//		ff::listFiles(dir, files, recursive);
//		
//		std::ofstream os(listFile.c_str());
//		if (!os)
//			throw listFile;
//
//		for (auto &f : files)
//		{
//			if (isIncluded(f))
//				os << f << endl;
//		}
//	}
//
//	virtual void exec2(std::string dataDir, std::string args)
//	{
//		auto isJPG = [](const std::string &f) {
//			return f.size() >= 4 && stricmp(f.c_str() + f.size() - 4, ".jpg") == 0;
//		};
//
//		dataDir = R"(F:\store\datasets\seg_dataset\ADE20K_2016_07_26\images\)";
//		genImageList(dataDir, dataDir + "list.txt", isJPG);
//
//		dataDir = R"(F:\dataset\flickr30k_images\)";
//		genImageList(dataDir, dataDir + "list.txt", isJPG);
//	}
//	virtual Object* clone()
//	{
//		return new RenderVOC(*this);
//	}
//};
//
//REGISTER_CLASS(RenderVOC)

void _loadModelFiles(const std::string &listFile, std::vector<std::string> &modelNames, std::vector<std::string> &modelFiles)
{
	std::string dir = ff::GetDirectory(listFile);
	std::ifstream is(listFile);
	if (!is)
		FF_EXCEPTION1("file open failed");
	
	modelNames.clear();
	modelFiles.clear();
	std::string name, file;
	while (is >> name >> file)
	{
		modelNames.push_back(name);
		modelFiles.push_back(ff::CatDirectory(dir, file));
	}
}

static void renderVOCImages(const std::string &listFile, const std::vector<std::string> &bgImageList, const std::string &tarDataDir, int nImages, int minObjsPerImage, int maxObjsPerImage)
{
	srand((int)time(NULL));

	//load model list
	std::vector<std::string> modelNames;
	std::vector<std::string> modelFiles;
	_loadModelFiles(listFile, modelNames, modelFiles);

	//load background image list
	std::vector<std::string>  bgImageFiles;
	loadImageList(bgImageList, bgImageFiles);

	//load 3D models
	std::vector<CVRModel>  models(modelFiles.size());
	std::vector<CVRender>  renders(modelFiles.size());
	for (size_t i = 0; i < modelFiles.size(); ++i)
	{
		printf("%d: loading %s\n", i+1, modelFiles[i].c_str());
		models[i].load(modelFiles[i]);
		renders[i] = CVRender(models[i]);
	}

	//uniform sampling the sphere directions
	std::vector<Vec3f>  vdirs;
	cvrm::sampleSphere(vdirs, 1000);

	//gen index list of models
	std::vector<int>  modelIndex;
	for (int i = 0; i < (int)models.size(); ++i)
		modelIndex.push_back(i);
	if (models.size() < maxObjsPerImage)
		maxObjsPerImage = (int)models.size();

	Annotations annot("Re3D", "Re3D", "VOC 2007", "rendered");

	std::string dpath = tarDataDir;
	annot.setDPath(dpath, true);

	//save label map and label list
	saveLabelMap(dpath + "labelmap.txt", modelNames);

	{
		std::ofstream os(tarDataDir + "/label_list.txt");
		if (!os)
			FF_EXCEPTION1("can't open label_list file");

		for (auto &label : modelNames)
			os << label << endl;
	}

	for (int n = 0; n < nImages;)
	{
		Mat bgImg = imread(bgImageFiles[rand() % bgImageFiles.size()]);
		if (bgImg.empty())
			continue;
		else
			++n;

		//scale background image to a standard size
		double scale = 800.0 / __max(bgImg.rows, bgImg.cols);
		Size dsize(int(bgImg.cols*scale), int(bgImg.rows*scale));
		resize(bgImg, bgImg, dsize);

		//random select models
		std::random_shuffle(modelIndex.begin(), modelIndex.end());
		int nModels = rand() % (maxObjsPerImage-minObjsPerImage+1) + minObjsPerImage;

		Mat3b dimg = bgImg.clone();
		//mask of objects
		Mat1i dmask(bgImg.size());
		setMem(dmask, 0xFF); //-1 for background pixels

		std::vector<DObject> objs;
		for (int i = 0; i < nModels; ++i)
		{
			int mi = modelIndex[i];
			CVRModel &modeli = models[mi];
			CVRender &renderi = renders[mi];

			//random gen a bounding box to place the object
			Rect bb = randBox(bgImg.size(), 100, 400, dmask);
			Size bbSize(bb.width, bb.height);

			CVRMats mats(modeli, bbSize);
			//set model rotation
			mats.mModel = cvrm::rotate(Vec3f(0, 0, 1), vdirs[rand() % vdirs.size()]);
			//render the model
			CVRResult r = renderi.exec(mats, bbSize);
			if (r.img.size() != bbSize)
			{
				printf("Error: render failed\n");
				continue;
			}
			//get object mask from the depth map
			Mat1b mask = getRenderMask(r.depth);

			//clip the object ROI
			Rect imgROI = rectOverlapped(bb, Rect(0, 0, dimg.cols, dimg.rows));
			if (imgROI.width <= 0 || imgROI.height <= 0)
				continue;
			Rect objROI = Rect(imgROI.x - bb.x, imgROI.y - bb.y, imgROI.width, imgROI.height);
			//merge object mask to @dmask
			for_each_2(DWHN1r(dmask, imgROI), DN1r(mask, objROI), [mi](int &i, uchar m) {
				if (m)
					i = mi;
			});
			if(imgROI!=bb)
			{
				printf("invalid bb\n");
			}

			//composite the current object
			bb = composite(r.img, mask, dimg, bb);

			DObject obj;
			obj.objID = mi;
			obj.name = modelNames[mi];
			obj.boundingBox = bb;
			objs.push_back(obj);
		}

		//save annotation file
		std::string fileBaseName = ff::StrFormat("%05d", n);
		annot.save(fileBaseName, dimg, objs);

		printf("%s, nm=%d\n", fileBaseName.c_str(), nModels);
	}
}
void genVOCList(std::string dataDir, int nVal)
{
	//dataDir = R"(Z:\local\data\Re3D\Re3Db\)";

	std::string imgDir = "JPEGImages", xmlDir = "Annotations";

	std::vector<string> files;
	ff::listFiles(dataDir + "/" + imgDir, files);

	auto saveList = [imgDir, xmlDir](const string vfiles[], int count, std::string listFile) {
		FILE *fp = fopen(listFile.c_str(), "w");
		if (!fp)
			throw listFile;

		for (int i = 0; i < count; ++i)
		{
			std::string baseName = ff::GetFileName(vfiles[i], false);
			fprintf(fp, "%s %s\n", (imgDir + "/" + vfiles[i]).c_str(), (xmlDir + "/" + baseName + ".xml").c_str());
		}

		fclose(fp);
	};

	auto createTestNameSize = [dataDir, imgDir](const string vfiles[], int count, string dfile) {
		FILE *fp = fopen(dfile.c_str(), "w");
		if (!fp)
			throw dfile;

		string dir = dataDir + imgDir + "/";
		for (int i = 0; i < count; ++i)
		{
			Mat img = imread(dir + vfiles[i]);
			fprintf(fp, "%s %d %d\n", ff::GetFileName(vfiles[i], false).c_str(), img.rows, img.cols);
		}
		fclose(fp);
	};

	//int ntest = int(files.size() * ratioOfTestImages);
	saveList(&files[0], nVal, dataDir + "val.txt");
	createTestNameSize(&files[0], nVal, dataDir + "test_name_size.txt");
	saveList(&files[nVal], files.size() - nVal, dataDir + "train.txt");
}


void render_3d_models_as_VOC_dataset()
{
#if 0
	//directory for the rendered dataset
	std::string tarDataDir = R"(/fan/local/paddle/PaddleDetection/dataset/re3d25d/)";
	//list of 3D model files
	std::string listFile = R"(/fan/SDUicloudCache/re3d/re3d25.txt)";
	//max./min. number of objects to be rendered in an image
	int maxObjectPerImage = 25, minObjectPerImage = 24;
#endif
	
	std::string tarDataDir = R"(/fan/local/paddle/PaddleDetection/dataset/ycb21a/)";
	std::string listFile = R"(/fan/store/datasets/BOP/ycbv_models/models_fine/list.txt)";
	int maxObjectPerImage = 21, minObjectPerImage = 20;

	//list of background images
	//the background image is randomly selected from this list
	std::vector<std::string> bgImageList = {
		R"(/fan/store/datasets/flickr30k_images/list.txt)",
		R"(/fan/store/datasets/seg_dataset/ADE20K_2016_07_26/images/list.txt)"
	};

	//number of images to render
	int nImages = 1000;
	//number of images for the validation set
	int nVal = 200;


	//render images 
	renderVOCImages(listFile, bgImageList, tarDataDir, nImages, minObjectPerImage, maxObjectPerImage);
	//gen VOC list files
	genVOCList(tarDataDir, nVal);
}

CMD_BEG()
CMD0("tools.voc.render_3d_models_as_detection_dataset", render_3d_models_as_VOC_dataset)
CMD_END()

_CMDI_END

