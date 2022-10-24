
#include"videoseg.h"
#include"opencv2/highgui.hpp"
using namespace cv;

namespace gkde {

	/* Dynamic (global) Kernel Density Estimation
	*/
	class vsf_dynamic_kde
	{
	public:
		class _CImp;
	private:
		_CImp* m_imp;
	public:
		vsf_dynamic_kde();

		~vsf_dynamic_kde();

		/*将当前直方图中各网格点的密度进行缩放

		通常0<=scale<1，可用于衰减之前的样本在模型中的权重
		*/
		//void scale_density(double scale);

		/*添加新的样本颜色

		  @img...: 源图像，FI_8UC3的格式

		  @mask : 标记像素类型的掩码， 0=B, 255=F, 127=U
		*/
		void add_new_samples(const uchar* img, int width, int height, int istep, const uchar* mask, int mstep);

		/*根据像素颜色，计算像素属于前景和背景的概率密度

		　@nbf : 用于返回像素在前景和背景直方图中的像素数（未归一化），为32SC2的图像数据，int[2]={B,F};
		*/
		void get_density(const uchar* img, int width, int height, int istep, int* nbf, int nstride);

		void get_density(const uchar* color, int* nbf);

		/*获取背景和前景的样本总数，可用于归一化由@get_density返回的密度（但通常没必要）
		*/
		void get_number_of_samples(int* nb, int* nf);
	};


	typedef int _NType;

	struct dkde_tab_item
	{
		_NType m_nbf[2];
	};

	class vsf_dynamic_kde::_CImp
	{
	public:
		dkde_tab_item* m_tab;
		_NType     m_nf, m_nb;
	public:
		_CImp()
			:m_tab(NULL), m_nf(0), m_nb(0)
		{
		}

		~_CImp()
		{
			delete[]m_tab;
		}
	};

	typedef vsf_dynamic_kde::_CImp _gcm_data;


	const int CLR_RSB = 2;
	const int TAB_WIDTH = (1 << (8 - CLR_RSB)) + 2; //+2, border
	const int TAB_WIDTH_2 = TAB_WIDTH * TAB_WIDTH;
	const int TAB_SIZE = TAB_WIDTH * TAB_WIDTH_2;
	const int TAB_BEG_DIFF = TAB_WIDTH_2 + TAB_WIDTH + 1;

	inline int _color_index(const uchar* pix)
	{
		return (int(pix[0]) >> CLR_RSB) * TAB_WIDTH_2 + (int(pix[1]) >> CLR_RSB) * TAB_WIDTH + (int(pix[2]) >> CLR_RSB) + TAB_BEG_DIFF;
	}

	static void _construct_tab(_gcm_data* gcm)
	{
		gcm->m_tab = new dkde_tab_item[TAB_SIZE];
		memset(gcm->m_tab, 0, sizeof(dkde_tab_item) * TAB_SIZE);

		gcm->m_nf = gcm->m_nb = 0;
	}

	//static void _scale_density(_gcm_data* gcm, double scale)
	//{
	//	for (int i = 0; i < TAB_SIZE; ++i)
	//	{
	//		gcm->m_tab[i].m_nbf[0] *= scale;
	//		gcm->m_tab[i].m_nbf[1] *= scale;
	//	}
	//
	//	gcm->m_nf *= scale;
	//	gcm->m_nb *= scale;
	//}

	static void _add_new_samples(_gcm_data* gcm, const uchar* img, int width, int height, int istep, const uchar* mask, int mstep)
	{
		int nf = 0, nb = 0;

		for (int yi = 0; yi < height; ++yi, img += istep, mask += mstep)
		{
			const uchar* pix = img;

			for (int xi = 0; xi < width; ++xi, pix += 3)
			{
				if (mask[xi] != 127)
				{
					const int idx = _color_index(pix);

					assert(mask[xi] == 0 || mask[xi] == 1);

					if (mask[xi] == 255)
					{
						gcm->m_tab[idx].m_nbf[1] += 1;
						++nf;
					}
					else
					{
						gcm->m_tab[idx].m_nbf[0] += 1;
						++nb;
					}
				}
			}
		}

		gcm->m_nf += nf;
		gcm->m_nb += nb;
	}

	static int g_DIDX[27] = { 0 };

	static void _init_didx()
	{
		if (g_DIDX[0] == 0)
		{
			for (int i = -1; i < 2; ++i)
			{
				for (int j = -1; j < 2; ++j)
				{
					for (int k = -1; k < 2; ++k)
					{
						g_DIDX[(i + 1) * 9 + (j + 1) * 3 + k + 1] = TAB_WIDTH_2 * i + TAB_WIDTH * j + k;
					}
				}
			}
		}
	}

	inline void _get_density(_gcm_data* gcm, const uchar* pix, int* nbf)
	{
		const dkde_tab_item* ptx = gcm->m_tab + _color_index(pix);

		double nb = 0, nf = 0;

		for (int i = 0; i < 27; ++i)
		{
			const dkde_tab_item* pt = ptx + g_DIDX[i];

			nb += pt->m_nbf[0];
			nf += pt->m_nbf[1];
		}

		nbf[0] = (int)(nb + 0.5);
		nbf[1] = (int)(nf + 0.5);
	}

	static void _get_density(_gcm_data* gcm, const uchar* img, int width, int height, int istep, int* nbf, int nstride)
	{
		for (int yi = 0; yi < height; ++yi, img += istep, nbf += nstride)
		{
			const uchar* pix = img;
			int* nx = nbf;

			for (int xi = 0; xi < width; ++xi, pix += 3, nx += 2)
			{
				_get_density(gcm, pix, nx);
			}
		}
	}


	vsf_dynamic_kde::vsf_dynamic_kde()
	{
		m_imp = new _CImp;
		_construct_tab(m_imp);

		_init_didx();
	}

	vsf_dynamic_kde::~vsf_dynamic_kde()
	{
		delete m_imp;
	}

	//void vsf_dynamic_kde::scale_density(double scale)
	//{
	//	_scale_density(m_imp, scale);
	//}

	void vsf_dynamic_kde::add_new_samples(const uchar* img, int width, int height, int istep, const uchar* mask, int mstep)
	{
		_add_new_samples(m_imp, img, width, height, istep, mask, mstep);
	}

	void vsf_dynamic_kde::get_density(const uchar* img, int width, int height, int istep, int* nbf, int nstride)
	{
		_get_density(m_imp, img, width, height, istep, nbf, nstride);
	}

	void vsf_dynamic_kde::get_density(const uchar* color, int* nbf)
	{
		_get_density(m_imp, color, nbf);
	}

	void vsf_dynamic_kde::get_number_of_samples(int* nb, int* nf)
	{
		if (nf)
			*nf = (int)(m_imp->m_nf + 0.5);

		if (nb)
			*nb = (int)(m_imp->m_nb + 0.5);
	}
}

cv::Mat1b vseg_get_trimap(const cv::Mat1b& mask, int dilateSize)
{
	if (dilateSize <= 0)
	{
		Rect roi = cv::get_mask_roi(DWHS(mask), 127);
		dilateSize = __max(roi.width, roi.height) / 2;
	}
	Mat1b dmask;
	cv::maxFilter(mask, dmask, dilateSize | 1);
	for_each_2(DWHN1(mask), DN1(dmask), [](uchar m, uchar& dm) {
		uchar d = dm - m;
		dm = d > 127 ? 0 : m > 127 ? 255 : 127;
		});
	return dmask;
}

cv::Mat1b vseg_global(const cv::Mat3b& ref, const cv::Mat1b& refTrimap, const cv::Mat3b& tar)
{
	const int MIN_SAMPLES = 15;

	gkde::vsf_dynamic_kde kde;
	kde.add_new_samples(DWHS(ref), DS(refTrimap));

	Mat2i nbf(tar.size());
	kde.get_density(DWHS(tar), DN(nbf));

	Mat1b prob(tar.size());
	for_each_2(DWHNC(nbf), DN1(prob), [MIN_SAMPLES](const int* n, uchar& p) {
		int c = n[0] + n[1];
		p = c < MIN_SAMPLES ? 127 : 255*n[1] / c;
		});
	return prob;
}

//===========================================================================================


namespace lkde {
	const int CLR_RSB = 2;
	const int TAB_WIDTH = (1 << (8 - CLR_RSB)) + 2; //+2, border
	const int TAB_WIDTH_2 = TAB_WIDTH * TAB_WIDTH;
	const int TAB_BEG_DIFF = TAB_WIDTH_2 + TAB_WIDTH + 1;

	inline int tcp_color_index(const uchar* pix)
	{
		return (int(pix[0]) >> CLR_RSB) * TAB_WIDTH_2 + (int(pix[1]) >> CLR_RSB) * TAB_WIDTH + (int(pix[2]) >> CLR_RSB) + TAB_BEG_DIFF;
	}

	static void tcp_calc_index(const uchar* img, int width, int height, int istep, int* index, int istride)
	{
		for (int yi = 0; yi < height; ++yi, img += istep, index += istride)
		{
			const uchar* pix = img;

			for (int xi = 0; xi < width; ++xi, pix += 3)
			{
				index[xi] = tcp_color_index(pix);
			}
		}
	}

	static void tcp_threshold_mask(const uchar* mask0, int width, int height, int mstep0, uchar* mask, int mstep, uchar T0, uchar T1)
	{
		for (int yi = 0; yi < height; ++yi, mask0 += mstep0, mask += mstep)
		{
			for (int xi = 0; xi < width; ++xi)
			{
				mask[xi] = mask0[xi] > T1 ? 1 : mask0[xi] <= T0 ? 0 : 2;
			}
		}
	}

	struct TabItem
	{
		ushort nbf[3];
	};

	static void tcp_tab_ins(const int* index, int width, int height, int istride, const uchar* mask, int mstep, TabItem* ptab)
	{
		for (int yi = 0; yi < height; ++yi, index += istride, mask += mstep)
		{
			for (int xi = 0; xi < width; ++xi)
			{
				assert(mask[xi] <= 2);

				++ptab[index[xi]].nbf[mask[xi]];
			}
		}
	}

	static void tcp_tab_del(const int* index, int width, int height, int istride, const uchar* mask, int mstep, TabItem* ptab)
	{
		for (int yi = 0; yi < height; ++yi, index += istride, mask += mstep)
		{
			for (int xi = 0; xi < width; ++xi)
			{
				assert(mask[xi] <= 2);

				--ptab[index[xi]].nbf[mask[xi]];
			}
		}
	}

	static void tcp_tab_ins(const int* index, const uchar* mask, int n, TabItem* ptab)
	{
		for (int i = 0; i < n; ++i)
		{
			assert(mask[i] <= 2);

			++ptab[index[i]].nbf[mask[i]];
		}
	}

	static void tcp_tab_del(const int* index, const uchar* mask, int n, TabItem* ptab)
	{
		for (int i = 0; i < n; ++i)
		{
			assert(mask[i] <= 2);

			--ptab[index[i]].nbf[mask[i]];
		}
	}

	static int g_DIDX[27] = { 0 };

	static void tcp_init_didx()
	{
		if (g_DIDX[0] == 0)
		{
			for (int i = -1; i < 2; ++i)
			{
				for (int j = -1; j < 2; ++j)
				{
					for (int k = -1; k < 2; ++k)
					{
						g_DIDX[(i + 1) * 9 + (j + 1) * 3 + k + 1] = TAB_WIDTH_2 * i + TAB_WIDTH * j + k;
					}
				}
			}
		}
	}

	static void tcp_get_prob(const TabItem* ptab, const uchar* px, int* nbf)
	{
		int id = tcp_color_index(px);

		int nb = 0, nf = 0;

		const TabItem* pix = ptab + id;

		for (int i = 0; i < 27; ++i)
		{
			const TabItem* ti = pix + g_DIDX[i];

			nb += ti->nbf[0];
			nf += ti->nbf[1];
		}

		nbf[0] = nb;
		nbf[1] = nf;
	}

	static void tcp_exec(const int* index, int width, int height, int istride, const uchar* mask, int mstep,
		const uchar* img, int istep, int* nbf, int nstride, int hwsz
	)
	{
		const int tabsz = TAB_WIDTH_2 * TAB_WIDTH + 1; //one more for index -1
		const int wsz = 2 * hwsz + 1;

		TabItem* ptabx = new TabItem[tabsz], * ptab = ptabx + 1;
		memset(ptabx, 0, sizeof(TabItem) * tabsz);

		const int* piy = index - hwsz * istride - hwsz;
		const uchar* pmy = mask - hwsz * mstep - hwsz, * pimy = img;
		int* ppy = nbf;

		tcp_tab_ins(piy, wsz, wsz, istride, pmy, mstep, ptab);

		tcp_init_didx();

		int  wistride = istride * wsz, wmstep = mstep * wsz;

		for (int xi = 0; xi < width; ++xi)
		{
			tcp_get_prob(ptab, pimy, ppy);

			if (xi & 1)
			{
				for (int yi = 1; yi < height; ++yi)
				{
					pimy -= istep, pmy -= mstep, piy -= istride, ppy -= nstride;

					tcp_tab_del(piy + wistride, pmy + wmstep, wsz, ptab);
					tcp_tab_ins(piy, pmy, wsz, ptab);

					tcp_get_prob(ptab, pimy, ppy);
				}
			}
			else
			{
				for (int yi = 1; yi < height; ++yi)
				{
					tcp_tab_del(piy, pmy, wsz, ptab);
					tcp_tab_ins(piy + wistride, pmy + wmstep, wsz, ptab);

					pimy += istep, pmy += mstep, piy += istride, ppy += nstride;

					tcp_get_prob(ptab, pimy, ppy);
				}
			}

			if (xi != width - 1)
			{
				tcp_tab_del(piy, 1, wsz, istride, pmy, mstep, ptab);
				tcp_tab_ins(piy + wsz, 1, wsz, istride, pmy + wsz, mstep, ptab);

				pimy += 3, pmy += 1, piy += 1, ppy += 2;
			}
		}

		delete[]ptabx;
	}


	static void tcp_get_ROI(int width, int height, const int bw[], int hwsz, int xywh[])
	{
		int x0 = hwsz - __min(bw[0], hwsz), y0 = hwsz - __min(bw[1], hwsz);
		int x1 = hwsz + width + __min(bw[2], hwsz);
		int y1 = hwsz + height + __min(bw[3], hwsz);

		xywh[0] = x0; xywh[1] = y0;
		xywh[2] = x1 - x0;
		xywh[3] = y1 - y0;
	}


	static void vs_trans_local_kde_impl(const uchar* img0, int width, int height, int istep0, const uchar* mask0, int mstep0,
		const uchar* img1, int istep1, const int borderLTRB[], int hwsz,
		int* nbf, int nstride
	)
	{
		const int W = width + 2 * hwsz, H = height + 2 * hwsz;

		int* index = new int[W * H];
		uchar* mask = new uchar[W * H];

		memset(index, 0xff, sizeof(int) * W * H);
		memset(mask, 0, sizeof(uchar) * W * H);

		int ROI[4]; //ROI relative to the extended region with hwsz border (W*H)
		tcp_get_ROI(width, height, borderLTRB, hwsz, ROI);

		tcp_calc_index(img0 + (-hwsz + ROI[1]) * istep0 + (-hwsz + ROI[0]) * 3, ROI[2], ROI[3], istep0, index + ROI[1] * W + ROI[0], W);

		cv::memcpy_2d(mask0 + (-hwsz + ROI[1]) * mstep0 + (-hwsz + ROI[0]), ROI[2], ROI[3], mstep0, mask + ROI[1] * W + ROI[0], W);

		tcp_exec(index + hwsz * W + hwsz, width, height, W, mask + hwsz * W + hwsz, W, img1, istep1, nbf, nstride, hwsz);

		delete[]index;
		delete[]mask;
	}


	void vsf_trans_local_kde(const uchar* img0, int width, int height, int istep0, const uchar* mask0, int mstep0,
		const uchar* img1, int istep1, const int borderLTRB[], int hwsz,
		int* nbf, int nstride
	)
	{
		int defBorder[] = { 0,0,0,0 };

		if (!borderLTRB)
			borderLTRB = defBorder;

		vs_trans_local_kde_impl(img0, width, height, istep0, mask0, mstep0, img1, istep1, borderLTRB, hwsz, nbf, nstride);
	}

}


cv::Mat1b vseg_local(const cv::Mat3b& ref, const cv::Mat1b& refMask, const cv::Mat3b& tar, int localWindowSize)
{
	Mat1b prob = Mat1b::zeros(tar.size());

	const int wsz = localWindowSize | 1, hwsz=wsz/2;
	const int MIN_SAMPLES = 3;

	Rect roi = cv::get_mask_roi(DWHS(refMask), 127);
	if (!roi.empty())
	{
		rectAppend(roi, wsz, wsz, wsz, wsz);
		roi = rectOverlapped(roi, Rect(0, 0, ref.cols, ref.rows));
		int border[] = { roi.x,roi.y,ref.cols - (roi.x + roi.width),ref.rows - (roi.y + roi.height) };

		Mat1b smask;
		int wsz = 9;
		boxFilter(refMask, smask, -1, Size(wsz, wsz));
		for_each_1(DWHN1(smask), [](uchar& m) {
			m = m < 5 ? 0 : m>250 ? 1 : 2;
			});
		imshow("smask", smask);

		Mat1b trimap(refMask.size());
		//cv::threshold(DWHS(refMask), DS(trimap), 127, 0, 1);
		trimap = smask;

		Mat2i nbf(roi.size());
		lkde::vsf_trans_local_kde(DWHS(ref(roi)), DS(trimap(roi)), DS(tar(roi)), border, hwsz, DN(nbf));

		for_each_2(DWHNC(nbf), DN1(prob(roi)), [MIN_SAMPLES](const int* n, uchar& p) {
			int c = n[0] + n[1];
			p = c < MIN_SAMPLES ? 127 : 255*n[1] / c;
			});
	}
	return prob;
}

//======================================================================================================================

void vseg_remove_unknown_regions(Mat1b& prob, uchar T0, uchar T1, double rT)
{
	Mat1b mask(prob.size());
	cv::threshold(DWHS(prob), DS(mask), T0, T1, 0, 127, 255);

	Mat1b dmask(prob.size());
	//maxFilter(mask, dmask, 3);
	boxFilter(mask, dmask, CV_8U, Size(3, 3));

	Mat1i cc(mask.size());
	int ncc = cc_seg_n4c1(DWHSC(mask), DN(cc));
	struct CCInfo
	{
		uchar m;
		uchar mx;
		int  size;
		int  pos, neg;
	};

	std::unique_ptr<CCInfo[]> _cci(new CCInfo[ncc]);
	CCInfo* cci = _cci.get();
	memset(cci, 0, sizeof(CCInfo)* ncc);
	for_each_3(DWHN1(cc), DN1(mask), DN1(dmask), [cci](int i, uchar m, uchar dm) {
		//if (m != 0)
		{
			cci[i].size++;
			cci[i].m = m;
			if (m != dm)
			{
				if (m < dm)
					cci[i].pos++;
				else
					cci[i].neg++;
			}
		}});

	for (int i = 0; i < ncc; ++i)
	{
		auto& c = cci[i];
		c.mx = c.m;
		if (c.m == 127)
		{
			double r = c.pos / (double(c.pos) + c.neg + 1e-3);

			if (r>rT)
				c.mx = 255;
			else if (r<1-rT)
				c.mx = 0;
		}
		/*if (c.size < 100)
		{
			c.mx = c.pos > c.neg ? 255 : 0;
		}*/
	}
	for_each_2(DWHN1(prob), DN1(cc), [cci](uchar& m, int i) {
		if (cci[i].m != cci[i].mx)
			m = cci[i].mx;
		});
}

void vseg_remove_small_regions(cv::Mat1b& prob, int minRegionSize, uchar T)
{
	Mat1b mask(prob.size());
	for_each_2(DWHN1(prob), DN1(mask), [T](uchar p, uchar& m) {
		m = p > T ? 255 : 0;
		});
	Mat1i cc(mask.size());
	int ncc = cc_seg_n8c1(DWHN1(mask), DN(cc));
	struct CCInfo
	{
		uchar m;
		int  size;
	};

	std::unique_ptr<CCInfo[]> _cci(new CCInfo[ncc]);
	CCInfo* cci = _cci.get();
	memset(cci, 0, sizeof(CCInfo) * ncc);
	for_each_2(DWHN1(cc), DN1(mask), [cci](int i, uchar m) {
		{
			cci[i].size++;
			cci[i].m = m;
		}});

	if (minRegionSize < 0) //find the max fg region size
	{
		for (int i = 0; i < ncc; ++i)
			if (cci[i].m == 255 && cci[i].size > minRegionSize)
				minRegionSize = cci[i].size;
	}
	for (int i = 0; i < ncc; ++i)
		if (cci[i].m == 0 || cci[i].size < minRegionSize)
			cci[i].size = 0;

	for_each_2(DWHN1(prob), DN1(cc), [cci, minRegionSize, T](uchar& m, int i) {
		if(cci[i].size==0)
			m = 0;
		});
}

inline Mat1b findMaxForeground(const Mat1b& prob)
{
	const int OPENING_WSZ = 1;

	Mat1b mask(prob.size());
	for_each_2(DWHN1(prob), DN1(mask), [](uchar p, uchar& m) {
		m = p <= 127 ? 0 : 255;
		});
	Mat1b tmask;
	if (OPENING_WSZ > 1)
		minFilter(mask, tmask, OPENING_WSZ);
	else
		tmask = mask.clone();

	Mat1i cc(mask.size());
	int ncc = cc_seg_n4c1(DWHSC(tmask), DN(cc));
	struct CCInfo
	{
		int size;
		//uchar val;
	};
	std::unique_ptr<CCInfo[]> _cci(new CCInfo[ncc]);
	CCInfo* cci = _cci.get();
	memset(cci, 0, sizeof(CCInfo) * ncc);
	for_each_2(DWHN1(cc), DN1(tmask), [cci](int i, uchar val) {
		if (val != 0)
		{
			cci[i].size++;
			//cci[i].val = val;
		}});
	int maxSize = std::max_element(cci, cci + ncc, [](const CCInfo& a, const CCInfo& b) {
		return a.size < b.size;
		})->size;
	for_each_2(DWHN1(tmask), DN1(cc), [maxSize, cci](uchar& m, int i) {
		if (m > 0 && cci[i].size != maxSize)
			m = 0;
		});

	if (OPENING_WSZ > 1)
		maxFilter(tmask, tmask, OPENING_WSZ);
	//imshow("tmask", tmask);

	return tmask;
}



//======================================================================================================================


class BITMAP {
public:
	int w, h;
	int* data;
	BITMAP(int w_, int h_) :w(w_), h(h_) { data = new int[w * h]; }

	BITMAP(Mat3b img)
	{
		w = img.cols;
		h = img.rows;
		data = new int[img.rows * img.cols];
		Mat rgba;
		cvtColor(img, rgba, CV_BGR2BGRA);
		cv::memcpy_2d(rgba.data, 4 * rgba.cols, rgba.rows, rgba.step, data, sizeof(int) * img.cols);
	}

	~BITMAP() { delete[] data; }
	int* operator[](int y) { return &data[y * w]; }
};

int patch_w = 3;
int pm_iters = 5;
int rs_max = INT_MAX;

#define XY_TO_INT(x, y) (((y)<<12)|(x))
#define INT_TO_X(v) ((v)&((1<<12)-1))
#define INT_TO_Y(v) ((v)>>12)

/* Measure distance between 2 patches with upper left corners (ax, ay) and (bx, by), terminating early if we exceed a cutoff distance.
   You could implement your own descriptor here. */
int dist(BITMAP* a, BITMAP* b, int ax, int ay, int bx, int by, int cutoff = INT_MAX) {
	int ans = 0;
	for (int dy = 0; dy < patch_w; dy++) {
		int* arow = &(*a)[ay + dy][ax];
		int* brow = &(*b)[by + dy][bx];
		for (int dx = 0; dx < patch_w; dx++) {
			int ac = arow[dx];
			int bc = brow[dx];
			int dr = (ac & 255) - (bc & 255);
			int dg = ((ac >> 8) & 255) - ((bc >> 8) & 255);
			int db = (ac >> 16) - (bc >> 16);
			ans += dr * dr + dg * dg + db * db;
		}
		if (ans >= cutoff) { return cutoff; }
	}
	return ans;
}

void improve_guess(BITMAP* a, BITMAP* b, int ax, int ay, int& xbest, int& ybest, int& dbest, int bx, int by) {
	int d = dist(a, b, ax, ay, bx, by, dbest);
	if (d < dbest) {
		dbest = d;
		xbest = bx;
		ybest = by;
	}
}

/* Match image a to image b, returning the nearest neighbor field mapping a => b coords, stored in an RGB 24-bit image as (by<<12)|bx. */
void patchmatch(BITMAP* a, BITMAP* b, BITMAP*& ann, BITMAP*& annd) {
	/* Initialize with random nearest neighbor field (NNF). */
	ann = new BITMAP(a->w, a->h);
	annd = new BITMAP(a->w, a->h);
	int aew = a->w - patch_w + 1, aeh = a->h - patch_w + 1;       /* Effective width and height (possible upper left corners of patches). */
	int bew = b->w - patch_w + 1, beh = b->h - patch_w + 1;
	memset(ann->data, 0, sizeof(int) * a->w * a->h);
	memset(annd->data, 0, sizeof(int) * a->w * a->h);
	for (int ay = 0; ay < aeh; ay++) {
		for (int ax = 0; ax < aew; ax++) {
			int bx = ax; //rand() % bew;
			int by = ay;// rand() % beh;
			(*ann)[ay][ax] = XY_TO_INT(bx, by);
			(*annd)[ay][ax] = dist(a, b, ax, ay, bx, by);
		}
	}
	for (int iter = 0; iter < pm_iters; iter++) {
		/* In each iteration, improve the NNF, by looping in scanline or reverse-scanline order. */
		int ystart = 0, yend = aeh, ychange = 1;
		int xstart = 0, xend = aew, xchange = 1;
		if (iter % 2 == 1) {
			xstart = xend - 1; xend = -1; xchange = -1;
			ystart = yend - 1; yend = -1; ychange = -1;
		}
		for (int ay = ystart; ay != yend; ay += ychange) {
			for (int ax = xstart; ax != xend; ax += xchange) {
				/* Current (best) guess. */
				int v = (*ann)[ay][ax];
				int xbest = INT_TO_X(v), ybest = INT_TO_Y(v);
				int dbest = (*annd)[ay][ax];

				/* Propagation: Improve current guess by trying instead correspondences from left and above (below and right on odd iterations). */
				if ((unsigned)(ax - xchange) < (unsigned)aew) {
					int vp = (*ann)[ay][ax - xchange];
					int xp = INT_TO_X(vp) + xchange, yp = INT_TO_Y(vp);
					if ((unsigned)xp < (unsigned)bew) {
						improve_guess(a, b, ax, ay, xbest, ybest, dbest, xp, yp);
					}
				}

				if ((unsigned)(ay - ychange) < (unsigned)aeh) {
					int vp = (*ann)[ay - ychange][ax];
					int xp = INT_TO_X(vp), yp = INT_TO_Y(vp) + ychange;
					if ((unsigned)yp < (unsigned)beh) {
						improve_guess(a, b, ax, ay, xbest, ybest, dbest, xp, yp);
					}
				}

				/* Random search: Improve current guess by searching in boxes of exponentially decreasing size around the current best guess. */
				int rs_start = rs_max;
				if (rs_start > MAX(b->w, b->h)) { rs_start = MAX(b->w, b->h); }
				for (int mag = rs_start; mag >= 1; mag /= 2) {
					/* Sampling window */
					int xmin = MAX(xbest - mag, 0), xmax = MIN(xbest + mag + 1, bew);
					int ymin = MAX(ybest - mag, 0), ymax = MIN(ybest + mag + 1, beh);
					int xp = xmin + rand() % (xmax - xmin);
					int yp = ymin + rand() % (ymax - ymin);
					improve_guess(a, b, ax, ay, xbest, ybest, dbest, xp, yp);
				}

				(*ann)[ay][ax] = XY_TO_INT(xbest, ybest);
				(*annd)[ay][ax] = dbest;
			}
		}
	}
}

cv::Mat1b vseg_patchmatch(const cv::Mat3b& ref, const cv::Mat1b& refMask, const cv::Mat3b& tar, int patchSize)
{
	BITMAP _ref(ref), _tar(tar);
	BITMAP* ann, * annd;
	patchmatch(&_tar, &_ref, ann, annd);

	Mat1b prob(tar.size());
	for_each_2(DWHN1(prob), ann->data,ann->w,ccn1(), [&refMask](uchar& p, const int m) {
		p = refMask(INT_TO_Y(m), INT_TO_X(m));
		});
	delete ann;
	delete annd;

	return prob;
}



//cv::Mat1b vseg_patchmatch0(const cv::Mat3b& ref, const cv::Mat1b& refMask, const cv::Mat3b& tar, int patchSize)
//{
//	PatchMatch pm(patchSize|1);
//	Mat1i dist;
//	Mat2i match;
//	pm.match(tar, ref, dist, match);
//
//	Mat1b prob(tar.size());
//	for_each_2(DWHN1(prob), DNC(match), [&refMask](uchar& p, const int* m) {
//		p = refMask(m[1], m[0]);
//		});
//	return prob;
//}

