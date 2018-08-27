#include "Calibrater.h"
#include <ctime>
#include <calib3d/calib3d.hpp>
#include <imgproc/imgproc.hpp>
#include <bemapiset.h>

Calibrater::Calibrater(QObject *parent)
{
	
}

Calibrater::~Calibrater()
{
}
//�ֶ���ʼ��
void Calibrater::initialize()
{
	//�ڴ˴�Ӧ�ö��������Խ��г�ʼ��
	cornerDatas= * new CornerDatas;
	stereoParams= * new StereoParams;
	remapMatrixs= * new RemapMatrixs;
}

Calibrater * Calibrater::getInstance()
{
	static Calibrater singleton;
	return &singleton;
}

int Calibrater::initCornerData(int nImages, cv::Size imageSize, cv::Size boardSize, float squareWidth)
{
	cornerDatas.imageSize = imageSize;
	cornerDatas.nImages = nImages;	
	cornerDatas.boardSize = boardSize;
	cornerDatas.nPoints = nImages * boardSize.width * boardSize.height;
	cornerDatas.nPointsPerImage = boardSize.width * boardSize.height;
	cornerDatas.objectPoints.resize(nImages, std::vector<cv::Point3f>(cornerDatas.nPointsPerImage, cv::Point3f(0, 0, 0)));
	cornerDatas.imagePoints1.resize(nImages, std::vector<cv::Point2f>(cornerDatas.nPointsPerImage, cv::Point2f(0, 0)));
	cornerDatas.imagePoints2.resize(nImages, std::vector<cv::Point2f>(cornerDatas.nPointsPerImage, cv::Point2f(0, 0)));

	//�������̽ǵ����������ֵ
	int i, j, k, n;
	for (i = 0; i < nImages; i++)
	{
		n = 0;
		for (j = 0; j < boardSize.height; j++)
			for (k = 0; k < boardSize.width; k++)
				cornerDatas.objectPoints[i][n++] = cv::Point3f(j*squareWidth, k*squareWidth, 0);
	}

	return 1;
}

int Calibrater::resizeCornerData(int nImages)
{
	cornerDatas.nImages = nImages;
	cornerDatas.nPoints = nImages * cornerDatas.nPointsPerImage;
	cornerDatas.objectPoints.resize(nImages);
	cornerDatas.imagePoints1.resize(nImages);
	cornerDatas.imagePoints2.resize(nImages);

	return 1;
}

int Calibrater::loadCornerData(const char * filename, CornerDatas & cornerDatas)
{
	cv::FileStorage fs(filename, cv::FileStorage::READ);

	//1 ����ļ���ʧ��,����0
	if (fs.isOpened() == false) {
		return 0;
	}

	fs["nPoints"] >> cornerDatas.nPoints;
	fs["nImages"] >> cornerDatas.nImages;
	fs["nPointsPerImage"] >> cornerDatas.nPointsPerImage;

	//2 ����ļ�������ȷ�Ľǵ������ļ�,����0 
	if (cornerDatas.nPoints <= 0 || cornerDatas.nImages <= 0 || cornerDatas.nPointsPerImage <= 0) {
		fs.release();
		return 0;
	}
	//3 ��֤���,��ʼ��ȡ
	cv::FileNodeIterator it = fs["imageSize"].begin();
	it >> cornerDatas.imageSize.width >> cornerDatas.imageSize.height;

	cv::FileNodeIterator bt = fs["boardSize"].begin();
	bt >> cornerDatas.boardSize.width >> cornerDatas.boardSize.height;

	for (int i = 0; i<cornerDatas.nImages; i++)
	{
		std::stringstream imagename;
		imagename << "image" << i;

		cv::FileNode img = fs[imagename.str()];
		std::vector<cv::Point3f> ov;
		std::vector<cv::Point2f> iv1, iv2;
		for (int j = 0; j<cornerDatas.nPointsPerImage; j++)
		{
			std::stringstream nodename;
			nodename << "node" << j;

			cv::FileNode pnt = img[nodename.str()];
			cv::Point3f op;
			cv::Point2f ip1, ip2;
			cv::FileNodeIterator ot = pnt["objectPoints"].begin();
			ot >> op.x >> op.y >> op.z;
			cv::FileNodeIterator it1 = pnt["imagePoints1"].begin();
			it1 >> ip1.x >> ip1.y;
			cv::FileNodeIterator it2 = pnt["imagePoints2"].begin();
			it2 >> ip2.x >> ip2.y;

			iv1.push_back(ip1);
			iv2.push_back(ip2);
			ov.push_back(op);
		}
		cornerDatas.imagePoints1.push_back(iv1);
		cornerDatas.imagePoints2.push_back(iv2);
		cornerDatas.objectPoints.push_back(ov);
	}

	fs.release();
	return 1;
}

int Calibrater::saveCornerData(const char * filename)
{
	cv::FileStorage fs(filename, cv::FileStorage::WRITE);
	if (fs.isOpened())
	{
		time_t rawtime;
		time(&rawtime);
		fs << "calibrationDate" << asctime(localtime(&rawtime));

		fs << "nPoints" << cornerDatas.nPoints;
		fs << "nImages" << cornerDatas.nImages;
		fs << "nPointsPerImage" << cornerDatas.nPointsPerImage;

		fs << "imageSize" << "[" << cornerDatas.imageSize.width << cornerDatas.imageSize.height << "]";

		fs << "boardSize" << "[" << cornerDatas.boardSize.width << cornerDatas.boardSize.height << "]";

		for (int i = 0; i<cornerDatas.nImages; i++)
		{
			std::stringstream imagename;
			imagename << "image" << i;

			fs << imagename.str() << "{";

			for (int j = 0; j<cornerDatas.nPointsPerImage; j++)
			{
				std::stringstream nodename;
				nodename << "node" << j;

				fs << nodename.str() << "{";

				cv::Point3f op = cornerDatas.objectPoints[i][j];
				cv::Point2f ip1 = cornerDatas.imagePoints1[i][j];
				cv::Point2f ip2 = cornerDatas.imagePoints2[i][j];

				fs << "objectPoints" << "[:";
				fs << op.x << op.y << op.z << "]";

				fs << "imagePoints1" << "[:";
				fs << ip1.x << ip1.y << "]";

				fs << "imagePoints2" << "[:";
				fs << ip2.x << ip2.y << "]";

				fs << "}";
			}

			fs << "}";
		}

		fs.release();
		return 1;
	}
	else
	{
		return 0;
	}
}

int Calibrater::detectCorners(cv::Mat & img1, cv::Mat & img2, int imageCount)
{
	//bool stereoMode = true;
	//if (img2.empty())
	//{
	//	stereoMode = false;
	//}

	// ��ȡ��ǰ���̶�Ӧ�Ľǵ������Ӿ���
	std::vector<cv::Point2f>& corners1 = cornerDatas.imagePoints1[imageCount];
	std::vector<cv::Point2f>& corners2 = cornerDatas.imagePoints2[imageCount];

	// Ѱ�����̼���ǵ�
	bool found1 = false;
	bool found2 = true;
	//	int flags = CV_CALIB_CB_ADAPTIVE_THRESH + CV_CALIB_CB_NORMALIZE_IMAGE + CV_CALIB_CB_FAST_CHECK;CV_CALIB_CB_FILTER_QUADS 
	int flags = CV_CALIB_CB_ADAPTIVE_THRESH + CV_CALIB_CB_NORMALIZE_IMAGE;
	found1 = findChessboardCorners(img1, cornerDatas.boardSize, corners1, flags);
	//if (stereoMode)
	found2 = findChessboardCorners(img2, cornerDatas.boardSize, corners2, flags);

	// ��������ͼ���ɹ���⵽���нǵ�
	// �򽫼�⵽�Ľǵ����꾫ȷ��
	if (found1 && found2)
	{
		//ת��Ϊ�Ҷ�ͼ
		cv::Mat gray1, gray2;
		cvtColor(img1, gray1, CV_RGB2GRAY);
		//if (stereoMode)
		cvtColor(img2, gray2, CV_RGB2GRAY);

		//����ǵ�ľ�ȷ����
		cv::Size regionSize(11, 11);
		cornerSubPix(gray1, corners1, regionSize, cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.05));
		//if (stereoMode)
		cornerSubPix(gray2, corners2, regionSize, cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.05));
	}

	// ��ʾ��⵽�Ľǵ�
	drawChessboardCorners(img1, cornerDatas.boardSize, corners1, found1);
	//if (stereoMode)
	drawChessboardCorners(img2, cornerDatas.boardSize, corners2, found2);

	// ��ʾ��ǰgood_board����Ŀ
	char info[10];
	sprintf_s(info, "%02d/%02d", imageCount + 1, cornerDatas.nImages);
	std::string text = info;
	showText(img1, text);
	//if (stereoMode)
	showText(img2, text);

	//if (stereoMode)
	return (found1 && found2) ? 1 : 0;
	//else
	//return found1 ? 1 : 0;
}

int Calibrater::loadCameraParams(const char * filename, CameraParams & cameraParams)
{
	cv::FileStorage fs(filename, cv::FileStorage::READ);
	if (fs.isOpened())
	{
		cv::FileNodeIterator it = fs["imageSize"].begin();
		it >> cameraParams.imageSize.width >> cameraParams.imageSize.height;

		fs["cameraMatrix"] >> cameraParams.cameraMatrix;
		fs["distortionCoefficients"] >> cameraParams.distortionCoefficients;
		fs["flags"] >> cameraParams.flags;

		int nImages = 0;
		fs["nImages"] >> nImages;

		for (int i = 0; i < nImages; i++)
		{
			char matName[50];
			sprintf_s(matName, "rotaionMatrix_%d", i);

			cv::Mat rotMat;
			fs[matName] >> rotMat;
			cameraParams.rotations.push_back(rotMat);
		}

		for (int i = 0; i < nImages; i++)
		{
			char matName[50];
			sprintf_s(matName, "translationMatrix_%d", i);

			cv::Mat tranMat;
			fs[matName] >> tranMat;
			cameraParams.translations.push_back(tranMat);
		}

		fs.release();
		return 1;
	}
	else
	{
		return 0;
	}
}

int Calibrater::saveCameraParams(const CameraParams & cameraParams, const char * filename)
{
	std::string filename_ = filename;

	//����ǰʱ�������ļ���
	if (filename_ == "" || filename_ == "cameraParams.yml")
	{
		int strLen = 20;
		char *pCurrTime = (char*)malloc(sizeof(char)*strLen);
		memset(pCurrTime, 0, sizeof(char)*strLen);
		time_t now;
		time(&now);
		strftime(pCurrTime, strLen, "%Y_%m_%d_%H_%M_%S_", localtime(&now));

		filename_ = pCurrTime;
		filename_ += "cameraParams.yml";
	}

	//д������
	cv::FileStorage fs(filename_.c_str(), cv::FileStorage::WRITE);
	if (fs.isOpened())
	{
		time_t rawtime;
		time(&rawtime);
		fs << "calibrationDate" << asctime(localtime(&rawtime));

		char flagText[1024];
		sprintf_s(flagText, "flags: %s%s%s%s%s",
			cameraParams.flags & CV_CALIB_FIX_K3 ? "fix_k3" : "",
			cameraParams.flags & CV_CALIB_USE_INTRINSIC_GUESS ? " + use_intrinsic_guess" : "",
			cameraParams.flags & CV_CALIB_FIX_ASPECT_RATIO ? " + fix_aspect_ratio" : "",
			cameraParams.flags & CV_CALIB_FIX_PRINCIPAL_POINT ? " + fix_principal_point" : "",
			cameraParams.flags & CV_CALIB_ZERO_TANGENT_DIST ? " + zero_tangent_dist" : "");
		cvWriteComment(*fs, flagText, 0);

		fs << "flags" << cameraParams.flags;

		fs << "imageSize" << "[" << cameraParams.imageSize.width << cameraParams.imageSize.height << "]";

		fs << "cameraMatrix" << cameraParams.cameraMatrix;
		fs << "distortionCoefficients" << cameraParams.distortionCoefficients;

		int nImages = cameraParams.rotations.size();
		fs << "nImages" << nImages;
		for (UINT i = 0; i < nImages; i++)
		{
			char matName[50];
			sprintf_s(matName, "rotaionMatrix_%d", i);

			fs << matName << cameraParams.rotations[i];
		}
		for (UINT i = 0; i < nImages; i++)
		{
			char matName[50];
			sprintf_s(matName, "translationMatrix_%d", i);

			fs << matName << cameraParams.translations[i];
		}

		fs.release();
		return 1;
	}
	else
	{
		return 0;
	}
}

int Calibrater::calibrateSingleCamera(CornerDatas & cornerDatas, CameraParams & cameraParams)
{
	cameraParams.imageSize = cornerDatas.imageSize;

	/***
	*	ִ�е�Ŀ����
	*/
	cv::calibrateCamera(
		cornerDatas.objectPoints,
		cornerDatas.imagePoints1,
		cornerDatas.imageSize,
		cameraParams.cameraMatrix,
		cameraParams.distortionCoefficients,
		cameraParams.rotations,
		cameraParams.translations,
		cameraParams.flags
	);

	return 1;
}

int Calibrater::calibrateStereoCamera()
{
	if (false)//��δ����δ��ִ��
	{
		/***
		*	ִ�е�Ŀ����
		*/
		cv::calibrateCamera(
			cornerDatas.objectPoints,
			cornerDatas.imagePoints1,
			cornerDatas.imageSize,
			stereoParams.cameraParams1.cameraMatrix,
			stereoParams.cameraParams1.distortionCoefficients,
			stereoParams.cameraParams1.rotations,
			stereoParams.cameraParams1.translations,
			stereoParams.cameraParams1.flags
		);

		cv::calibrateCamera(
			cornerDatas.objectPoints,
			cornerDatas.imagePoints2,
			cornerDatas.imageSize,
			stereoParams.cameraParams2.cameraMatrix,
			stereoParams.cameraParams2.distortionCoefficients,
			stereoParams.cameraParams2.rotations,
			stereoParams.cameraParams2.translations,
			stereoParams.cameraParams2.flags
		);

		/***
		*	���浥Ŀ������������
		*/
		saveCameraParams(stereoParams.cameraParams1, "cameraParams_left.yml"/*����Ϊ�ɱ��������ļ�ȷ��*/);
		saveCameraParams(stereoParams.cameraParams2, "cameraParams_right.yml"/*����Ϊ�ɱ��������ļ�ȷ��*/);
	}

	stereoParams.imageSize = cornerDatas.imageSize;

	stereoCalibrate(
		cornerDatas.objectPoints,
		cornerDatas.imagePoints1,
		cornerDatas.imagePoints2,
		stereoParams.cameraParams1.cameraMatrix,
		stereoParams.cameraParams1.distortionCoefficients,
		stereoParams.cameraParams2.cameraMatrix,
		stereoParams.cameraParams2.distortionCoefficients,
		cornerDatas.imageSize,
		stereoParams.rotation,
		stereoParams.translation,
		stereoParams.essential,
		stereoParams.foundational,
		cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 1e-6),
		stereoParams.flags
		+ CV_CALIB_FIX_PRINCIPAL_POINT + CV_CALIB_FIX_FOCAL_LENGTH + CV_CALIB_SAME_FOCAL_LENGTH + CV_CALIB_FIX_ASPECT_RATIO
		+ CV_CALIB_ZERO_TANGENT_DIST + CV_CALIB_FIX_K1 + CV_CALIB_FIX_K2 + CV_CALIB_FIX_K3
	);
	//CV_CALIB_FIX_PRINCIPAL_POINT
	//CV_CALIB_FIX_FOCAL_LENGTH 
	//CV_CALIB_SAME_FOCAL_LENGTH
	//CV_CALIB_FIX_ASPECT_RATIO
	//CV_CALIB_ZERO_TANGENT_DIST 
	//CV_CALIB_FIX_K1
	//CV_CALIB_FIX_K2
	//CV_CALIB_FIX_K3
	return 1;
}

int Calibrater::getCameraCalibrateError(std::vector<std::vector<cv::Point3f>>& _objectPoints, std::vector<std::vector<cv::Point2f>>& _imagePoints, CameraParams & cameraParams, double err)
{
	cv::Mat imagePoints2;
	int totalPoints = 0;
	double totalErr = 0;

	size_t nImages = _objectPoints.size();

	for (int i = 0; i < nImages; i++)
	{
		// ��ȡ��ǰ���̶�Ӧ�Ľǵ������Ӿ���
		std::vector<cv::Point3f>& objectPoints = _objectPoints[i];
		std::vector<cv::Point2f>& imagePoints = _imagePoints[i];
		totalPoints += objectPoints.size();

		// ������ͶӰ�������
		projectPoints(
			objectPoints,
			cameraParams.rotations[i],
			cameraParams.translations[i],
			cameraParams.cameraMatrix,
			cameraParams.distortionCoefficients,
			imagePoints2);

		// ������ͶӰ���
		double erri = norm(imagePoints, imagePoints2, CV_L2);
		totalErr += erri * erri;
	}

	// ƽ������ͶӰ���
	err = std::sqrt(totalErr / totalPoints);

	return 1;
}

int Calibrater::getStereoCalibrateError(double & err)
{
	// ���öԼ���Լ���������У��Ч��


	std::vector<cv::Vec3f> epilines[2];
	std::vector<std::vector<cv::Point2f> > imagePoints[2];
	cv::Mat cameraMatrix[2], distCoeffs[2];
	int npoints = 0;
	int i, j, k;

	imagePoints[0] = cornerDatas.imagePoints1;
	imagePoints[1] = cornerDatas.imagePoints2;
	cameraMatrix[0] = stereoParams.cameraParams1.cameraMatrix;
	cameraMatrix[1] = stereoParams.cameraParams2.cameraMatrix;
	distCoeffs[0] = stereoParams.cameraParams1.distortionCoefficients;
	distCoeffs[1] = stereoParams.cameraParams2.distortionCoefficients;

	for (i = 0; i < cornerDatas.nImages; i++)
	{
		int npt = (int)imagePoints[0][i].size();
		cv::Mat imgpt[2];

		for (k = 0; k < 2; k++)
		{
			imgpt[k] = cv::Mat(imagePoints[k][i]);
			// ����У��������̽ǵ�����
			undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], cv::Mat(), cameraMatrix[k]);
			// ����Լ���
			computeCorrespondEpilines(imgpt[k], k + 1, stereoParams.foundational, epilines[k]);
		}

		// ����Լ������
		for (j = 0; j < npt; j++)
		{
			double errij =
				fabs(imagePoints[0][i][j].x * epilines[1][j][0] +
					imagePoints[0][i][j].y * epilines[1][j][1] + epilines[1][j][2]) +
				fabs(imagePoints[1][i][j].x * epilines[0][j][0] +
					imagePoints[1][i][j].y * epilines[0][j][1] + epilines[0][j][2]);
			err += errij;
		}
		npoints += npt;
	}
	err /= npoints;

	return 1;
}

int Calibrater::rectifySingleCamera(CameraParams & cameraParams, RemapMatrixs & remapMatrixs)
{
	cv::initUndistortRectifyMap(
		cameraParams.cameraMatrix,
		cameraParams.distortionCoefficients,
		cv::Mat(),
		getOptimalNewCameraMatrix(
			cameraParams.cameraMatrix,
			cameraParams.distortionCoefficients,
			cameraParams.imageSize, 1, cameraParams.imageSize, 0),
		cameraParams.imageSize,
		CV_16SC2,
		remapMatrixs.mX1,
		remapMatrixs.mY1);

	return 1;
}

int Calibrater::rectifyStereoCamera(RECTIFYMETHOD method)
{
	//��ʼ��
	remapMatrixs.mX1 = cv::Mat(stereoParams.imageSize, CV_32FC1);
	remapMatrixs.mY1 = cv::Mat(stereoParams.imageSize, CV_32FC1);
	remapMatrixs.mX2 = cv::Mat(stereoParams.imageSize, CV_32FC1);
	remapMatrixs.mY2 = cv::Mat(stereoParams.imageSize, CV_32FC1);

	cv::Mat R1, R2, P1, P2, Q;
	cv::Rect roi1, roi2;
	double alpha = -1;

	//ִ��˫ĿУ��
	stereoRectify(
		stereoParams.cameraParams1.cameraMatrix,
		stereoParams.cameraParams1.distortionCoefficients,
		stereoParams.cameraParams2.cameraMatrix,
		stereoParams.cameraParams2.distortionCoefficients,
		stereoParams.imageSize,
		stereoParams.rotation,
		stereoParams.translation,
		R1, R2, P1, P2, Q,
		CV_CALIB_ZERO_DISPARITY,
		alpha,
		stereoParams.imageSize,
		&roi1, &roi2);

	//ʹ��HARTLEY�����Ķ��⴦��
	if (method == RECTIFY_HARTLEY)
	{
		cv::Mat F, H1, H2;
		F = findFundamentalMat(
			cornerDatas.imagePoints1,
			cornerDatas.imagePoints2,
			cv::FM_8POINT, 0, 0);
		stereoRectifyUncalibrated(
			cornerDatas.imagePoints1,
			cornerDatas.imagePoints2,
			F, stereoParams.imageSize, H1, H2, 3);

		R1 = stereoParams.cameraParams1.cameraMatrix.inv() * H1 * stereoParams.cameraParams1.cameraMatrix;
		R2 = stereoParams.cameraParams2.cameraMatrix.inv() * H2 * stereoParams.cameraParams2.cameraMatrix;
		P1 = stereoParams.cameraParams1.cameraMatrix;
		P2 = stereoParams.cameraParams2.cameraMatrix;
	}

	//����ͼ��У�����������ӳ�����
	initUndistortRectifyMap(
		stereoParams.cameraParams1.cameraMatrix,
		stereoParams.cameraParams1.distortionCoefficients,
		R1, P1,
		stereoParams.imageSize,
		CV_32FC1,//CV_16SC2
		remapMatrixs.mX1, remapMatrixs.mY1);

	initUndistortRectifyMap(
		stereoParams.cameraParams2.cameraMatrix,
		stereoParams.cameraParams2.distortionCoefficients,
		R2, P2,
		stereoParams.imageSize,
		CV_32FC1,//CV_16SC2
		remapMatrixs.mX2, remapMatrixs.mY2);

	//�������
	Q.copyTo(remapMatrixs.Q);
	remapMatrixs.roi1 = roi1;
	remapMatrixs.roi2 = roi2;

	return 1;
}

int Calibrater::saveCalibrationDatas(const char * filename, RECTIFYMETHOD method)
{
	cv::FileStorage fs(filename, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		//		AfxMessageBox(_T("�Ѵ�"));
		///		CString str;
		//		str.Format(_T("%c",filename[0]));
		//		AfxMessageBox(str);
		time_t rawtime;
		time(&rawtime);
		fs << "calibrationDate" << asctime(localtime(&rawtime));

		fs << "num_boards" << cornerDatas.nImages;
		fs << "imageSize" << "[" << cornerDatas.imageSize.width << cornerDatas.imageSize.height << "]";

		char flagText[1024];
		sprintf_s(flagText, "flags: %s%s%s%s%s",
			stereoParams.flags & CV_CALIB_USE_INTRINSIC_GUESS ? "+ use_intrinsic_guess" : "",
			stereoParams.flags & CV_CALIB_FIX_ASPECT_RATIO ? " + fix_aspect_ratio" : "",
			stereoParams.flags & CV_CALIB_FIX_PRINCIPAL_POINT ? " + fix_principal_point" : "",
			stereoParams.flags & CV_CALIB_FIX_INTRINSIC ? " + fix_intrinsic" : "",
			stereoParams.flags & CV_CALIB_SAME_FOCAL_LENGTH ? " + same_focal_length" : "");

		cvWriteComment(*fs, flagText, 0);

		fs << "stereoCalibrateFlags" << stereoParams.flags;
		fs << "leftCameraMatrix" << stereoParams.cameraParams1.cameraMatrix;
		fs << "leftDistortCoefficients" << stereoParams.cameraParams1.distortionCoefficients;
		fs << "rightCameraMatrix" << stereoParams.cameraParams2.cameraMatrix;
		fs << "rightDistortCoefficients" << stereoParams.cameraParams2.distortionCoefficients;
		fs << "rotationMatrix" << stereoParams.rotation;
		fs << "translationVector" << stereoParams.translation;
		fs << "foundationalMatrix" << stereoParams.foundational;

		if (method == RECTIFY_BOUGUET)
		{
			fs << "rectifyMethod" << "BOUGUET";
			fs << "leftValidArea" << "[:"
				<< remapMatrixs.roi1.x << remapMatrixs.roi1.y
				<< remapMatrixs.roi1.width << remapMatrixs.roi1.height << "]";
			fs << "rightValidArea" << "[:"
				<< remapMatrixs.roi2.x << remapMatrixs.roi2.y
				<< remapMatrixs.roi2.width << remapMatrixs.roi2.height << "]";
			fs << "QMatrix" << remapMatrixs.Q;
		}
		else
			fs << "rectifyMethod" << "HARTLEY";

		fs << "remapX1" << remapMatrixs.mX1;
		fs << "remapY1" << remapMatrixs.mY1;
		fs << "remapX2" << remapMatrixs.mX2;
		fs << "remapY2" << remapMatrixs.mY2;

		fs.release();
		return 1;
	}
	else
	{
		//AfxMessageBox(_T("û���ļ�"));

	}
	return 0;
}

int Calibrater::remapImage(cv::Mat & img1, cv::Mat & img2, cv::Mat & img1r, cv::Mat & img2r, RemapMatrixs & remapMatrixs)
{
	if (!remapMatrixs.mX1.empty() && !remapMatrixs.mY1.empty())
	{
		cv::remap(img1, img1r, remapMatrixs.mX1, remapMatrixs.mY1, cv::INTER_LINEAR);
	}
	if (!remapMatrixs.mX2.empty() && !remapMatrixs.mY2.empty())
	{
		cv::remap(img2, img2r, remapMatrixs.mX2, remapMatrixs.mY2, cv::INTER_LINEAR);
	}

	return 1;
}

void Calibrater::showText(cv::Mat & img, std::string text)
{
	int fontFace = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
	double fontScale = 1;
	int fontThickness = 2;

	// get text size
	int textBaseline = 0;
	cv::Size textSize = cv::getTextSize(text, fontFace,
		fontScale, fontThickness, &textBaseline);
	textBaseline += fontThickness;

	// put the text at lower right corner
	cv::Point textOrg((img.cols - textSize.width - 10),
		(textSize.height + 10));

	// draw the box
	rectangle(img, textOrg + cv::Point(0, textBaseline),
		textOrg + cv::Point(textSize.width, -textSize.height),
		cv::Scalar(0, 0, 255));

	// ... and the textBaseline first
	line(img, textOrg + cv::Point(0, fontThickness),
		textOrg + cv::Point(textSize.width, fontThickness),
		cv::Scalar(0, 0, 255));

	// then put the text itself
	putText(img, text, textOrg, fontFace, fontScale,
		cv::Scalar::all(255), fontThickness, 8);
}