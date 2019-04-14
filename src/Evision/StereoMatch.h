#pragma once
#include <QThread>
#include <core/matx.hpp>
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/utility.hpp"
#include "EvisionParamEntity.h"
#include "StereoMatchParamEntity.h"


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <atlstr.h> // use STL string instead, although not as convenient...
#include <atltrace.h>
#define TRACE ATLTRACE

#include <iostream>
#include <fstream>
#include <string>
//#include<time.h>

#include "opencv2/features2d.hpp"
#include "opencv2/xfeatures2d.hpp"
// sift is 50 times slower but get 7 times more matched points
// FAST detect more points than SURF
// STAR/MSER generate very few keypoints,
#define DETECTOR_TYPE	"FAST" // FAST,SIFT,SURF,STAR,MSER,GFTT,HARRIS...see the create function
#define DESCRIPTOR_TYPE	"SIFT" // SURF,SIFT,BRIEF,...BRIEF seemed to has bug
#define MATCHER_TYPE	"FlannBased" // BruteForce,FlannBased,BruteForce-L1,...

#define MAXM_FILTER_TH	.8	// threshold used in GetPair
#define HOMO_FILTER_TH	60	// threshold used in GetPair
#define NEAR_FILTER_TH	40	// diff points should have distance more than NEAR_FILTER_TH
/*
 * 
 */
class StereoMatch :public QThread
{
	Q_OBJECT
public:
	StereoMatch(std::string img1_filename, std::string img2_filename, std::string cameraParams_filename);
	~StereoMatch();
	bool init(bool needCameraParamFile=true);
	void run() override;
	void Save();
	enum { STEREO_BM = 0, STEREO_SGBM = 1, STEREO_HH = 2, STEREO_VAR = 3, STEREO_3WAY = 4 };
private:
	enum Algorithm { FEATURE_PT, DENSE };

	//#define PARAM	// algorithm parameters that can be modified
	//PARAM
	Algorithm g_algo = DENSE;//DENSE; // 2 algorithms to select corresponding points and reconstruct 3D scene
	StereoMatchParamEntity * m_entity;
	//������ȡ���ļ�
	std::string img1_filename = "";
	std::string img2_filename = "";
	std::string cameraParams_filename = "";
	//���ڱ�����ļ�
	std::string root = "";						//�ļ�����λ��
	std::string disparity_filename = "";		//�Ӳ�ͼ�ļ���
	std::string disparity_raw_filename = "";	//ԭʼ�Ӳ������ļ���
	std::string point_cloud_filename = "";		//PCD�����ļ���
	//
	cv::Mat img1, img2;
	cv::Size img_size;

	cv::Mat cameraMatrix1;//������ڲ�
	cv::Mat distCoeffs1;//���������ϵ��
	cv::Mat cameraMatrix2;//������ڲ�
	cv::Mat distCoeffs2;//���������ϵ��
	cv::Mat R1, P1, R2, P2,Q;//ӳ�������
	cv::Mat R, T, E, F;
	cv::Rect roi1;
	cv::Rect roi2;

	cv::Mat Raw_Disp_Data;		//ԭʼ�Ӳ�����,���ڱ����xml�ļ�,���ʱʹ��
	cv::Mat Visual_Disp_Data;	//������ʾ�ڽ����Ϻͱ���Ϊpng���Ӳ�ʾ��ͼ

	bool no_display=false;
private:	
	void ADCensusDriver();
	void OpenCVBM();
	void OpenCVSGBM();
	void Elas();

signals:
	void openMessageBox(QString, QString);
};