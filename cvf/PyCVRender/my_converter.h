# ifndef __NDARRAY_CONVERTER_H__
# define __NDARRAY_CONVERTER_H__

#include <Python.h>
#include <opencv2/core/core.hpp>


class NDArrayConverter {
public:
	// must call this first, or the other routines don't work!
	static bool init_numpy();

	static bool toMat(PyObject* o, cv::Mat &m);
	static PyObject* toNDArray(const cv::Mat& mat);
};

#include <pybind11/pybind11.h>

inline pybind11::handle toHandle(const cv::Mat &mat)
{
	return pybind11::handle(NDArrayConverter::toNDArray(mat));
}

//
// Define the type converter
//



namespace pybind11 {
	namespace detail {

		template <> struct type_caster<cv::Mat> {
		public:

			PYBIND11_TYPE_CASTER(cv::Mat, _("numpy.ndarray"));

			bool load(handle src, bool) {
				return NDArrayConverter::toMat(src.ptr(), value);
			}

			static handle cast(const cv::Mat &m, return_value_policy, handle defval) {
				return handle(NDArrayConverter::toNDArray(m));
			}
		};

		template<typename Tp>
		struct type_caster<cv::Mat_<Tp>>{
		public:

			PYBIND11_TYPE_CASTER(cv::Mat_<Tp>, _("numpy.ndarray"));

			bool load(handle src, bool) {
				return NDArrayConverter::toMat(src.ptr(), value);
			}

			static handle cast(const cv::Mat_<Tp> &m, return_value_policy, handle defval) {
				return handle(NDArrayConverter::toNDArray(m));
			}
		};

		template<typename _Tp, int m, int n>
		struct  type_caster<cv::Matx<_Tp, m, n>> {
		public:
			typedef cv::Matx<_Tp, m, n> _Matx;

			PYBIND11_TYPE_CASTER(_Matx, _("numpy.ndarray"));

			bool load(handle src, bool) {
				cv::Mat t;
				if (!NDArrayConverter::toMat(src.ptr(), t))
					return false;
				CV_Assert(t.rows == m && t.cols == n);
				value = _Matx(t);
				return true;
			}
			static handle cast(const _Matx &v, return_value_policy, handle defval) {
				return handle(NDArrayConverter::toNDArray(cv::Mat(v)));
			}
		};

		template<typename _AnyT, typename _ArrayValT, int _ArraySize>
		struct any_cast_with_array
		{
		public:
			PYBIND11_TYPE_CASTER(_AnyT, _("list"));

			typedef std::array<_ArrayValT, _ArraySize> _ArrayT;

			pybind11::detail::type_caster<_ArrayT> _caster;

			bool load(handle src, bool convert) {
				if (_caster.load(src, convert))
				{
					static_assert(sizeof(_AnyT) == sizeof(_ArrayT), "invalid cast with unmatched size");
					this->value = reinterpret_cast<const _AnyT&>(*(_ArrayT*)_caster);
					return true;
				}
				return false;
			}
			static handle cast(const _AnyT &v, return_value_policy rvp, handle defval) {
				return pybind11::detail::type_caster<_ArrayT>::cast(reinterpret_cast<const _ArrayT&>(v), rvp, defval);
			}
		};

		template<>
		struct type_caster<cv::Size> 
			:public any_cast_with_array<cv::Size,int,2>
		{
		};
		template<>
		struct type_caster<cv::Rect>
			:public any_cast_with_array<cv::Rect, int, 4>
		{
		};

		template<typename _ValT, int _Size>
		struct type_caster<cv::Vec<_ValT,_Size>>
			:public any_cast_with_array<cv::Vec<_ValT,_Size>,_ValT,_Size>
		{};
	}
} // namespace pybind11::detail

# endif


