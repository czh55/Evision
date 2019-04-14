#include "StereoMatchController.h"
#include <QMessageBox>
#include <QFileDialog>
#include "StereoMatch.h"

StereoMatchController::StereoMatchController(QObject *parent)
	: QObject(parent)
{
	m_entity = StereoMatchParamEntity::getInstance();
}

StereoMatchController::~StereoMatchController()
{
}
//����:ƥ��Ĭ�ϲ���
void StereoMatchController::setDefaultMatchParamCommand()
{
	if (m_entity->getBM())
	{
		m_entity->setBM_preFilterType_NORMALIZED(true);
		m_entity->setBM_preFilterSize(5);
		m_entity->setBM_uniquenessRatio(5);
		m_entity->setBM_preFilterCap(31);
		m_entity->setBM_speckleWindowSize(69);
		m_entity->setBM_SADWindowSize(21);
		m_entity->setBM_minDisparity(-1);
		m_entity->setBM_numDisparities(144);
		m_entity->setBM_speckleRange(34);
		m_entity->setBM_disp12MaxDiff(2);
		m_entity->setBM_textureThreshold(50);
	}
	else if (m_entity->getSGBM())
	{
		m_entity->setSGBM_minDisparity(1);
		m_entity->setSGBM_numDisparities(64);
		m_entity->setSGBM_blockSize(3);
		m_entity->setSGBM_P1(8);
		m_entity->setSGBM_P2(32);
		m_entity->setSGBM_disp12MaxDiff(6);
		m_entity->setSGBM_preFilterCap(47);
		m_entity->setSGBM_uniquenessRatio(5);
		m_entity->setSGBM_speckleWindowSize(1);
		m_entity->setSGBM_speckleRange(14);
		m_entity->setSGBM_MODEL_Default(true);
	}
	else if (m_entity->getElas())
	{
		m_entity->setDispMin(0);
		m_entity->setDispMax(255);
		m_entity->setSupportThreshold(0.95);
		m_entity->setSupportTexture(10);
		m_entity->setCandidateStepsize(5);
		m_entity->setInconWindowSize(5);
		m_entity->setInconThreshold(5);
		m_entity->setInconMinSupport(5);
		m_entity->setAddCorners(true);
		m_entity->setGridSize(20);
		m_entity->setBeta(0.02);
		m_entity->setGamma(5);
		m_entity->setSigma(1);
		m_entity->setSradius(3);
		m_entity->setMatchTexture(0);
		m_entity->setLrThreshold(2);
		m_entity->setSpeckleSimThreshold(1);
		m_entity->setSpeckleSize(200);
		m_entity->setIpolGapWidth(5000);
		m_entity->setFilterMedian(true);
		m_entity->setFilterAdaptiveMean(false);
		m_entity->setPostprocessOnlyLeft(false);
		m_entity->setSubSampling(false);
	}
	else if(m_entity->getADCensus())
	{
		m_entity->setDMin(0);
		m_entity->setDMax(60);
		m_entity->setCensusWinH(9);
		m_entity->setCensusWinW(7);
		m_entity->setDefaultBorderCost(0.999);
		m_entity->setLambdaAD(10.0);
		m_entity->setLambdaCensus(30.0);
		m_entity->setAggregatingIterations(4);
		m_entity->setColorThreshold1(20);
		m_entity->setColorThreshold2(6);
		m_entity->setMaxLength1(34);
		m_entity->setMaxLength2(17);
		m_entity->setColorDifference(15);
		m_entity->setPi1(0.1);
		m_entity->setPi2(0.3);
		m_entity->setDispTolerance(0);
		m_entity->setVotingThreshold(20);
		m_entity->setVotingRatioThreshold(4);
		m_entity->setMaxSearchDepth(20);
		m_entity->setCannyThreshold1(20);
		m_entity->setCannyThreshold2(60);
		m_entity->setCannyKernelSize(3);
		m_entity->setBlurKernelSize(3);
	}
	else
	{
		QMessageBox::information(NULL, QStringLiteral("����"), QStringLiteral("û��ѡ��ƥ���㷨"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
}
//����:ƥ��
void StereoMatchController::MatchCommand()
{
	bool ok = false;
	QFileDialog * fileDialog = new QFileDialog();

	fileDialog->setWindowTitle(QStringLiteral("��ѡ��������ͷ�����ͼƬ"));
	fileDialog->setNameFilter(QStringLiteral("ͼƬ�ļ�(*.jpg *.png *.jpeg *.bmp)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		ImageL = fileDialog->selectedFiles().at(0);
		ok = true;
	}
	else
	{
		return;
	}
		
	if (ok)
	{
		ok = false;
		fileDialog->setWindowTitle(QStringLiteral("��ѡ��������ͷ�����ͼƬ"));
		if (fileDialog->exec() == QDialog::Accepted)
		{
			ImageR = fileDialog->selectedFiles().at(0);
			ok = true;
		}
		else
		{
			return;
		}
	}
	ok = (!ImageL.isEmpty() && !ImageR.isEmpty());

	if (ok)
	{
		ok = false;
		if (m_entity->getRectifiedInput()==true)
		{
			//ʹ��У׼�õ�����ͼƬ
			ok = true;
		}
		else
		{
			//ʹ��ûУ׼��ͼƬ
			QFileDialog * fileDialog2 = new QFileDialog();
			fileDialog2->setWindowTitle(QStringLiteral("��ѡ����������ļ�"));
			fileDialog2->setNameFilter(QStringLiteral("YML/XML�ļ�(*.yml *.yaml *.xml)"));
			fileDialog2->setFileMode(QFileDialog::ExistingFile);
			if (fileDialog2->exec() == QDialog::Accepted)
			{
				paramsFile = fileDialog2->selectedFiles().at(0);
				ok = true;
			}
		}
	}
	if (ok) 
	{
		_stereoMatch = new StereoMatch(ImageL.toStdString(),ImageR.toStdString(), paramsFile.toStdString());
		//ע��.��ʱparamsFile.toStdString()�����ǿյ�(��m_entity->getRectifiedInput()==trueʱ)
		if (_stereoMatch->init())
		{
			connect(_stereoMatch, SIGNAL(openMessageBox(QString, QString)), this, SLOT(onOpenMessageBox(QString, QString)));
			_stereoMatch->start();
		}
		else
		{
			QMessageBox::information(NULL, QStringLiteral("����"), QStringLiteral("ƥ���ʼ��ʧ��!"));
			return;
		}
	}

}
//����:ˢ���Ӳ�ͼ
void StereoMatchController::RefreshStereoMatchCommand()
{
	_stereoMatch = new StereoMatch(ImageL.toStdString(),
		ImageR.toStdString(), paramsFile.toStdString());
	if (_stereoMatch->init())
	{
		connect(_stereoMatch, SIGNAL(openMessageBox(QString, QString)), this, SLOT(onOpenMessageBox(QString, QString)));
		_stereoMatch->start();
	}
	else
	{
		QMessageBox::information(NULL, QStringLiteral("����"), QStringLiteral("ƥ���ʼ��ʧ��!"));
	}
}
//����:����
void StereoMatchController::SaveCommand()
{
	if (_stereoMatch != NULL)
	{
		_stereoMatch->Save();
	}
}

//��Ϣ��Ӧ:�����Ի���
void StereoMatchController::onOpenMessageBox(QString title, QString msg)
{
	QMessageBox::information(NULL, title, msg);
}
