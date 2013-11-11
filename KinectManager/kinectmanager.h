#ifndef KINECTMANAGER_H
#define KINECTMANAGER_H

#include <QtWidgets/QMainWindow>
#include <QtOpenGL/QGLWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtCore/QTimer>
#include "ui_kinectmanager.h"

#include <Windows.h>
#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>

#include "StreamViewer.h"

class KinectManager : public QMainWindow
{
	Q_OBJECT

public:
	KinectManager(QWidget *parent = 0);
	~KinectManager();
	
public slots:
	void startKinect();
	void takeScreenshot();

protected:
	bool initKinect();

	bool getRGBData();
	bool getDepthData();

private:
	Ui::KinectManagerClass ui;

	// Kinect variables
	HANDLE rgbStream;              // The identifier of the Kinect's RGB Camera
	HANDLE depthStream;
	INuiSensor* sensor;            // The kinect sensor

	static const int width = 640;
	static const int height = 480;

	unsigned char *rgbdata;
	unsigned char *depthdata;


private slots:
	void switchMode();
	void updateStream();

private:
	enum Mode{
		Separate = 0,
		ColorOver
	} m;
	QTimer timer;
	StreamViewer *colorView;
	StreamViewer *depthView;
};

#endif // KINECTMANAGER_H
