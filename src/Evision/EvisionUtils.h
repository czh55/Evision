#pragma once
#include <qimage.h>
#include "opencv2\core\core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <QGraphicsView>
#include <QGraphicsScene>

/*
 * ʵ�ù��߷���
 */
class EvisionUtils
{
public:
	EvisionUtils();
	~EvisionUtils();
	/*
	 * cv::Matת��ΪQImage
	 */
	static QImage cvMat2QImage(const cv::Mat& mat);

	/*
	 * QImageת��Ϊcv::Mat
	 */
	static cv::Mat QImage2cvMat(QImage image);
	/*
	 * ����������idle image
	 */
	static QImage getDefaultImage();
	/*
	 * ��img��ʾ��view��
	 */
	static void ShowImageOnUi(cv::Mat& img, QGraphicsScene*sense, QGraphicsView* view);
	/*
	 * ��˫Ŀ�궨�õ������в���д���ļ�
	 * 1.�ļ���							std::string filename
	 * 2.�������1(��Խ������������)	cv::Mat& cameraMatrix1
	 * 3.����ϵ��1						cv::Mat& distCoeffs1
	 * 4.�������2						cv::Mat& cameraMatrix2
	 * 5.����ϵ��2						cv::Mat& distCoeffs2
	 * 6.�������֮�����ת				cv::Mat& R
	 * 7.�������֮���ƽ��				cv::Mat& T
	 * 8.���ʾ���						cv::Mat& E
	 * 9.��������						cv::Mat& F
	 * 10.ͼƬ�ߴ�						cv::Size& imageSize,
	 * 11.��תӳ�����1                 cv::Mat& R1,
	 * 12.ͶӰӳ�����1                 cv::Mat& P1,
	 * 13.��תӳ�����2                 cv::Mat& R2,
	 * 14.ͶӰӳ�����2                 cv::Mat& P2,
	 * 15.��άӳ�����                  cv::Mat& Q,
	 * 16.�������roi1                  cv::Rect& roi1,
	 * 17.�������roi2                  cv::Rect& roi2
	 * ����ֵ:
	 *	�ɹ�:true,ʧ�ܺͳ���:false
	 */
	static bool write_AllCameraParams(std::string& filename,
		cv::Mat& cameraMatrix1,cv::Mat& distCoeffs1,cv::Mat& cameraMatrix2,cv::Mat& distCoeffs2,
		cv::Mat& R,cv::Mat& T,cv::Size& imageSize,cv::Mat& R1,cv::Mat& P1,cv::Mat& R2,cv::Mat& P2,
		cv::Mat& Q,cv::Rect& roi1,cv::Rect& roi2);

	/*
	 * ���ļ��ж�ȡ˫Ŀ�궨�õ��Ĳ���
	 * 1.�ļ���							std::string filename
	 * 2.�������1(��Խ������������)	cv::Mat* cameraMatrix1
	 * 3.����ϵ��1						cv::Mat* distCoeffs1
	 * 4.�������2						cv::Mat* cameraMatrix2
	 * 5.����ϵ��2						cv::Mat* distCoeffs2
	 * 6.�������֮�����ת				cv::Mat* R
	 * 7.�������֮���ƽ��				cv::Mat* T
	 * 8.���ʾ���						cv::Mat* E
	 * 9.��������						cv::Mat* F
	 * 10.ͼƬ�ߴ�						cv::Size* imageSize,
	 * 11.��תӳ�����1                 cv::Mat* R1,
	 * 12.ͶӰӳ�����1                 cv::Mat* P1,
	 * 13.��תӳ�����2                 cv::Mat* R2,
	 * 14.ͶӰӳ�����2                 cv::Mat* P2,
	 * 15.��άӳ�����                  cv::Mat* Q,
	 * 16.�������roi1                  cv::Rect* roi1,
	 * 17.�������roi2                  cv::Rect* roi2
	 * ����ֵ:
	 *	�ɹ�:true,ʧ�ܺͳ���:false
	 */
	static bool read_AllCameraParams(std::string& filename,
		cv::Mat* cameraMatrix1,cv::Mat* distCoeffs1,cv::Mat* cameraMatrix2,cv::Mat* distCoeffs2,
		cv::Mat* R,cv::Mat* T,cv::Mat* E,cv::Mat* F,cv::Size* imageSize,cv::Mat* R1,cv::Mat* P1,cv::Mat* R2,cv::Mat* P2,
		cv::Mat* Q,cv::Rect* roi1,cv::Rect* roi2);

	/*
	 * ��ȡƥ������Ĳ���
	 */
	static bool read_ParamsForStereoMatch(std::string& filename,
		cv::Mat* cameraMatrix1,cv::Mat* distCoeffs1,cv::Mat* cameraMatrix2,cv::Mat* distCoeffs2, 
		cv::Mat* R1,cv::Mat* P1,cv::Mat* R2,cv::Mat* P2,cv::Mat* Q, cv::Rect* roi1,cv::Rect* roi2);
	/*
	 * ��ȡУ������Ĳ���
	 */
	static bool read_ParamsForStereoRectify(std::string& filename,cv::Mat* cameraMatrix1,
		cv::Mat* distCoeffs1,cv::Mat* cameraMatrix2,cv::Mat* distCoeffs2,
		cv::Mat* R1,cv::Mat* P1,cv::Mat* R2,cv::Mat* P2,cv::Rect* roi1,cv::Rect* roi2);
#ifdef WITH_PCL
	/*
	 * ����PCD����
	 */
	static void createAndSavePointCloud(cv::Mat &disparity, cv::Mat &leftImage, cv::Mat &Q, std::string filename);
#endif
	/*
	 *��ԭʼ�Ӳ�����ת��Ϊ�ʺ���ʾ�ʹ洢ΪͼƬ�ĻҶ��Ӳ�ͼ
	 */
	template <typename T>
	static void getGrayDisparity(const cv::Mat& disp, cv::Mat& grayDisp, bool stretch = true);
};
template <typename T>
void EvisionUtils::getGrayDisparity(const cv::Mat& disp, cv::Mat& grayDisp, bool stretch)
{
	cv::Size imgSize = disp.size();
	cv::Mat output(imgSize, CV_8UC3);
	T min, max;

	if (stretch)
	{
		min = (std::numeric_limits<T>::max());
		max = 0;
		for (size_t h = 0; h < imgSize.height; h++)
		{
			for (size_t w = 0; w < imgSize.width; w++)
			{
				T disparity = disp.at<T>(h, w);

				if (disparity < min && disparity >= 0)
					min = disparity;
				else if (disparity > max)
					max = disparity;
			}
		}
	}

	for (size_t h = 0; h < imgSize.height; h++)
	{
		for (size_t w = 0; w < imgSize.width; w++)
		{
			cv::Vec3b color;
			T disparity = disp.at<T>(h, w);

			if (disparity >= 0)
			{
				if (stretch)
					disparity = (255 / (max - min)) * (disparity - min);

				color[0] = (uchar)disparity;
				color[1] = (uchar)disparity;
				color[2] = (uchar)disparity;

			}
			else
			{
				color[0] = 0;
				color[1] = 0;
				color[2] = 0;
			}

			output.at<cv::Vec3b>(h, w) = color;
		}
	}

	grayDisp = output.clone();
}