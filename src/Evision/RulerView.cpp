#include "RulerView.h"
#include "EvisionUtils.h"
#include "QGraphicsScene"
#include <QFileDialog>
#include "StereoMatch.h"
#include <QMessageBox>
#include <iostream>
//ֱ��������ͼ,�����ò���
RulerView::RulerView(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.checkBox_RawDispOK->setEnabled(false);
	ui.checkBox_OriginOK->setEnabled(false);
	ui.checkBox_CameraParamOK->setEnabled(false);
	ui.pushButton_start->setEnabled(false);
	sceneL = new QGraphicsScene();
}

RulerView::~RulerView()
{
}
//��O���ڻ�ͼ
void RulerView::printImgToO(cv::Mat value)
{
	sceneL->clear();
	QImage QImage = EvisionUtils::cvMat2QImage(value);
	sceneL->addPixmap(QPixmap::fromImage(QImage));
	ui.customGraphicsView_O->setScene(sceneL);
	ui.customGraphicsView_O->setMinimumSize(QImage.width(), QImage.height());
	ui.customGraphicsView_O->setMaximumSize(QImage.width(), QImage.height());
	ui.customGraphicsView_O->show();
	ui.customGraphicsView_O->update();
}

void RulerView::checkEnable()
{
	if(ui.checkBox_RawDispOK->isChecked()&&ui.checkBox_OriginOK->isChecked()&&ui.checkBox_CameraParamOK->isChecked())
	{
		ui.pushButton_start->setEnabled(true);
	}
}

//��Ӧ����ƶ�
void RulerView::onMouseMove(int x, int y)
{
	ui.lineEdit_ImgX->setText(QString::fromStdString(std::to_string(x)));
	ui.lineEdit_ImgY->setText(QString::fromStdString(std::to_string(y)));

}
//��Ӧ����������
void RulerView::onMouseLButtonDown(int x, int y)
{
	if (started)
	{
		float disp;

		if (RawDisp.type() == CV_32F)
			disp = RawDisp.at<float>(y, x);
		else
			disp = RawDisp.at<uchar>(y, x);

		float vec3DAbs;
		if (disp > 0)
		{
			cv::Point3f vec3D = image3D.at<cv::Point3f>(y, x);
			vec3DAbs = sqrt(pow(vec3D.x, 2) + pow(vec3D.y, 2) + pow(vec3D.z, 2));
		}
		else
		{
			vec3DAbs = -1;
		}
		ui.lineEdit_Res->setText(QString::fromStdString(std::to_string(vec3DAbs)));
	}
}
//��Ӧ����Ҽ�����
void RulerView::onMouseRButtonDown(int x, int y)
{

}
/*
 * ѡ��ԭʼ�Ӳ��ļ�
 */
void RulerView::onSelectRawDispFile()
{
	QFileDialog * fileDialog = new QFileDialog();
	fileDialog->setWindowTitle(QStringLiteral("��ѡ��ԭʼ�Ӳ��ļ�"));
	fileDialog->setNameFilter(QStringLiteral("���л�(*.xml)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		try
		{
			cv::FileStorage fStorage(fileDialog->selectedFiles().at(0).toStdString(), cv::FileStorage::READ);
			fStorage["disp"] >> RawDisp;
			fStorage.release();
		}
		catch (cv::Exception e)
		{
			std::cout << "ԭʼ�Ӳ����ݶ�ȡʧ��!" << e.err << std::endl;
		}
		ui.checkBox_RawDispOK->setChecked(true);
		checkEnable();
	}
}
/*
 * ѡ��ԭͼ
 */
void RulerView::onSelectOriginImg()
{
	QFileDialog * fileDialog = new QFileDialog();
	fileDialog->setWindowTitle(QStringLiteral("��ѡ�����������ѡ�Ӳ�ͼ������ͼ������ͼ"));
	fileDialog->setNameFilter(QStringLiteral("ͼƬ�ļ�(*.jpg *.png *.jpeg *.bmp)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		try
		{
			this->img = cv::imread(fileDialog->selectedFiles().at(0).toStdString());
		}
		catch (cv::Exception e)
		{
			std::cout << "ԭͼ��ȡʧ��!" << e.err << std::endl;
		}
		ui.checkBox_OriginOK->setChecked(true);
		checkEnable();
	}
}
/*
 * ѡ����������ļ�
 */
void RulerView::onSelectCameraParamFile()
{
	QFileDialog * fileDialog = new QFileDialog();
	fileDialog->setWindowTitle(QStringLiteral("��ѡ����������ļ�"));
	fileDialog->setNameFilter(QStringLiteral("���л��ļ�(*.xml *.yml)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		try
		{
			cv::FileStorage fStorage(fileDialog->selectedFiles().at(0).toStdString(), cv::FileStorage::READ);
			fStorage["Q"] >> Q;
			fStorage.release();
			ui.checkBox_CameraParamOK->setChecked(true);
			checkEnable();
		}
		catch (cv::Exception e)
		{
			std::cout << "���������ȡʧ��!" << e.err << std::endl;
		}

	}
}

void RulerView::onStart()
{
	cv::reprojectImageTo3D(RawDisp, image3D, Q);
	cv::Mat resizedImage, resizedDispMap;
	cv::resize(img, resizedImage, cv::Size(), scaleFactor, scaleFactor);
	cv::resize(RawDisp, resizedDispMap, cv::Size(), scaleFactor, scaleFactor);
	img = resizedImage.clone();
	RawDisp = resizedDispMap.clone();
	cv::resize(image3D, image3D, cv::Size(), scaleFactor, scaleFactor);

	if (RawDisp.type() == CV_32F)
	{
		//ADCensus��ԭʼ�Ӳ�������CV_32F,����ֱ����ʾ
		EvisionUtils::getGrayDisparity<float>(RawDisp, disparityGary, true);
	}
	else if(RawDisp.type() == CV_8U)
	{
		//BM,SGBM��ELAS���Ӳ�������CV_8U,����ֱ����ʾ
		disparityGary = RawDisp;
	}
	printImgToO(disparityGary);
	ui.pushButton_selectRawDisp->setEnabled(false);
	ui.pushButton_selectOrigin->setEnabled(false);
	ui.pushButton_selectCameraParam->setEnabled(false);
	started = true;
}

void RulerView::onSwitchImageToShow()
{
	if(DispIsShowing)
	{
		printImgToO(img);
		DispIsShowing = !DispIsShowing;
	}
	else
	{
		printImgToO(disparityGary);
		DispIsShowing = !DispIsShowing;
	}
}
