#include "EvisionView.h"
#include "QDebug"
#include "QMessageBox"
#include "EvisionUtils.h"
#include "CalibraterView.h"
#include "MatcherView.h"
#include <qevent.h>
#include <qmimedata.h>
#include <QFileDialog>
#include "RulerView.h"
#include "StereoCameraView.h"
#include "CameraView.h"
#include "WatchImageView.h"
#ifdef WITH_CUDA
#include "ObjectDetectionView.h"
#endif

#if (defined WITH_PCL) && (defined WITH_VTK)  
#include "../Evision3dViz/Evision3dViz.h"
#endif
#include "EvisionRectifyView.h"
#include "CreateCameraParamFile.h"

// �������е�
// ulp: units in the last place.
template <typename T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
IsAlmostEqual(T x, T y, int ulp = 2)
{
	// the machine epsilon has to be scaled to the magnitude of the values used
	// and multiplied by the desired precision in ULPs (units in the last place)
	return std::abs(x - y) < std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
		// unless the result is subnormal
		|| std::abs(x - y) < std::numeric_limits<T>::min();
}

//���캯��
EvisionView::EvisionView(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setAcceptDrops(true);
	msgLabel = new QLabel;
	msgLabel->setMinimumSize(msgLabel->sizeHint());
	msgLabel->setAlignment(Qt::AlignHCenter);
	msgLabel->setText(QStringLiteral("����"));
	statusBar()->addWidget(msgLabel);
	statusBar()->setStyleSheet(QString("QStatusBar::item{border: 0px}"));

	m_entity = EvisionParamEntity::getInstance();
	m_controller = new EvisionController();


	connect(m_entity, SIGNAL(paramChanged_StatusBar()), this, SLOT(onParamChanged_StatusBarText()), Qt::QueuedConnection);


	on_action_LogViewSwitch();//Logview��׼��

	std::cout << "Qt Detected:"<< QT_VERSION_MAJOR<<"."<<QT_VERSION_MINOR << "." <<QT_VERSION_PATCH<<std::endl;
#ifdef DEBUG
	std::cout << "Evision is in debug mode and running slowly!" << std::endl;
	std::cout << "Evision�����ڵ���ģʽ,�����ٶ�����." << std::endl;

#else
	std::cout << "Evision is in release mode and running at full speed!" << std::endl;
	std::cout << "Evision�����ڲ���ģʽ,ȫ������." << std::endl;
#endif
}


//��ʾ��Ŀ�����ͼ
void EvisionView::onCamera()
{
	CameraView * _camera = new CameraView();
	ui.mdiArea->addSubWindow(_camera);
	_camera->show();
}
//��ʾ˫Ŀ�����ͼ
void EvisionView::onStereoCamera()
{
	StereoCameraView * _stereoCamera = new StereoCameraView();
	ui.mdiArea->addSubWindow(_stereoCamera);
	_stereoCamera->show();
}

//��ʾ����
void EvisionView::onShowPointCloud()
{
#if (defined WITH_PCL) && (defined WITH_VTK)  
	Evision3dViz  * evision3dViz = new Evision3dViz();
	ui.mdiArea->addSubWindow(evision3dViz);
	evision3dViz->show();
#else
	QMessageBox::information(this, QStringLiteral("�ù���δ����!"), 
		QStringLiteral("������Ŀ����/C++/Ԥ�����������\"WITH_PCL\"��\"WITH_VTK\",���ú�PCL��VTK����,��ȷ��Evision3dVizģ����������!"));
#endif
}
//��ʾ�궨��ͼ
void EvisionView::on_action_calibrate_view()
{
	CalibraterView * m_calibrate = new CalibraterView();
	ui.mdiArea->addSubWindow(m_calibrate);
	m_calibrate->show();
}
//��ʾ������ͼ
void EvisionView::on_action_rectify()
{
	EvisionRectifyView * m_Rectify = new EvisionRectifyView();
	ui.mdiArea->addSubWindow(m_Rectify);
	m_Rectify->show();
}
//��ʾ����ƥ����ͼ
void EvisionView::on_action_stereoMatch_view()
{
	MatcherView * m_matcher = new MatcherView();
	ui.mdiArea->addSubWindow(m_matcher);
	m_matcher->show();
}
//��ʾ����ʽ�����ͼ
void EvisionView::on_action_Measure_view()
{
	RulerView * _Rfinterface = new RulerView();
	ui.mdiArea->addSubWindow(_Rfinterface);
	_Rfinterface->show();
}
//����Ŀ������ͼ
void EvisionView::on_action_ObjectDetection_view()
{
#ifdef WITH_CUDA
	ObjectDetectionView* _ObjectDetectionView = new ObjectDetectionView();
	ui.mdiArea->addSubWindow(_ObjectDetectionView);
	_ObjectDetectionView->show();
#else
	QMessageBox::information(this, QStringLiteral("�ù���δ����!"),
		QStringLiteral("������Ŀ����/C++/Ԥ�����������\"WITH_CUDA\"��ȷ��EvisionObjDetectionģ����������"));
#endif
}
//LogView
void EvisionView::on_action_LogViewSwitch()
{
	logView = LogView::getInstance();
	logView->show();

	old_pos = this->pos();
	old_size = this->size();
	logView->move(*new QPoint(old_pos.x() + 10 + this->frameGeometry().width(), old_pos.y()));
}
//�Ӳ�ת����
void EvisionView::on_action_disp_to_pcd()
{
#ifdef WITH_PCL
	//1.ѡ���Ӳ�
	cv::Mat RawDisp,img, Q;
	bool ok = false;
	QFileDialog * fileDialog = new QFileDialog();
	QString dispFilename;
	fileDialog->setWindowTitle(QStringLiteral("��ѡ��ԭʼ�Ӳ��ļ�"));
	fileDialog->setNameFilter(QStringLiteral("���л�(*.xml)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		try
		{
			dispFilename = fileDialog->selectedFiles().at(0);
			cv::FileStorage fStorage(dispFilename.toStdString(), cv::FileStorage::READ);
			fStorage["disp"] >> RawDisp;
			fStorage.release();
			ok = true;
		}
		catch (cv::Exception e)
		{
			std::cout << "ԭʼ�Ӳ����ݶ�ȡʧ��!" << e.err << std::endl;
		}
	}
	else
	{
		return;
	}
	if (ok)
	{
		ok = false;
		fileDialog->setWindowTitle(QStringLiteral("��ѡ�����������ѡ�Ӳ�ͼ������ͼ������ͼ"));
		fileDialog->setNameFilter(QStringLiteral("ͼƬ�ļ�(*.jpg *.png *.jpeg *.bmp)"));
		fileDialog->setFileMode(QFileDialog::ExistingFile);
		if (fileDialog->exec() == QDialog::Accepted)
		{
			try
			{
				img = cv::imread(fileDialog->selectedFiles().at(0).toStdString());
				ok = true;
			}
			catch (cv::Exception e)
			{
				std::cout << "ԭͼ��ȡʧ��!" << e.err << std::endl;
			}
		}else
		{
			return;
		}
	}else
	{
		return;
	}
	if (ok)
	{
		ok = false;
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
				ok = true;
			}
			catch (cv::Exception e)
			{
				std::cout << "���������ȡʧ��!" << e.err << std::endl;
			}
		}else
		{
			return;
		}
	}else
	{
		return;
	}
	if(ok)
	{
		QFileInfo *_fileInfo = new QFileInfo(dispFilename);//path like F:/test/123.xml
		std::string filename = _fileInfo->absolutePath().toStdString().//F:/test
		append("/").												   //F:/test/
		append(_fileInfo->baseName().toStdString()).				   //F:/test/123
		append(".pcd");												   //F:/test/123.pcd
		EvisionUtils::createAndSavePointCloud(RawDisp, img, Q, filename);
	}
#else
	QMessageBox::information(this, QStringLiteral("�ù���δ����!"), QStringLiteral("������Ŀ����/C++/Ԥ�����������\"WITH_PCL\"�����ú�PCL����"));
#endif
}
/*
 * ������������ļ�
 */
void EvisionView::on_action_create_param()
{
	CreateCameraParamFile * _createCameraParamFile = new CreateCameraParamFile();
	ui.mdiArea->addSubWindow(_createCameraParamFile);
	_createCameraParamFile->show();
}

//״̬������
void EvisionView::onParamChanged_StatusBarText()
{
	msgLabel->setText(m_entity->getStatusBarText());
}

//�ļ����ϵ�����������
void EvisionView::dragEnterEvent(QDragEnterEvent * event)
{
	if (event->mimeData()->hasFormat("text/uri-list"))
	{
		event->acceptProposedAction();
		//QMessageBox::information(NULL, QStringLiteral("��Ϣ"), QStringLiteral("�ļ���������"));
		m_entity->setStatusBarText("Drop the file for open!");
	}
}
//�ļ��ڴ��������ϱ�����
void EvisionView::dropEvent(QDropEvent * event)
{
	m_entity->setStatusBarText(QStringLiteral("����"));
	//QMessageBox::information(NULL, QStringLiteral("��Ϣ"), QStringLiteral("�ļ����ͷ��ڴ�����"));
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
	{
		return;
	}
	else if(urls.size()>1)
	{
		//QMessageBox::information(NULL, QStringLiteral("��Ϣ"), QStringLiteral("����һ���ļ�"));
	}else if(urls.size()==1)
	{
		//QMessageBox::information(NULL, QStringLiteral("��Ϣ"), QStringLiteral("һ���ļ�"));
		//�ļ�����ʶ��ʹ�
		QString file_name = urls[0].toLocalFile();
		QFileInfo fileinfo(file_name);
		if (!fileinfo.isFile())//�����ļ�
		{
			return;
		}
		else
		{
			if (fileinfo.suffix() == "png"|| fileinfo.suffix() == "jpg"||
				fileinfo.suffix() == "jpeg")
			{
				WatchImageView * m_WatchImage = new WatchImageView(file_name);
				ui.mdiArea->addSubWindow(m_WatchImage);
				m_WatchImage->show();
			}
		}
	}
		
}
//����ͷ��¼�
void EvisionView::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_entity->setStatusBarText(QStringLiteral("����"));
	}
}
//�����ƶ��¼�
void EvisionView::moveEvent(QMoveEvent* event)
{
	//QWidget::moveEvent(event);
	QPoint delta = this->pos() - old_pos;
	//��������ڵ��ƶ���
	//�Ӵ��ڽ��е����ƶ�
	logView->move(delta + *new QPoint(old_pos.x() + 10 + this->frameGeometry().width(), old_pos.y()));
	old_pos = this->pos();
}

void EvisionView::resizeEvent(QResizeEvent* event)
{
	if (this->size().width() != old_size.width())
	{
		logView->move(*new QPoint(old_pos.x() + 10 + this->frameGeometry().width(), old_pos.y()));
		old_size = this->size();
	}
}

void EvisionView::changeEvent(QEvent*event)
{
	if (event->type() != QEvent::WindowStateChange) return;
	if (this->windowState() == Qt::WindowMaximized)//���
	{
		//logView->setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
	}
	if (this->windowState() == Qt::WindowMinimized)//��С��
	{
		//logView->setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

	}
}

void EvisionView::closeEvent(QCloseEvent* event)
{
	logView->close();
}

