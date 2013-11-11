#include "kinectmanager.h"

KinectManager::KinectManager(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	initKinect();

	// create qglwidgets
	colorView = new StreamViewer(this);
	depthView = new StreamViewer(this);
	m = Separate;

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(colorView);
	layout->addWidget(depthView);

	ui.centralWidget->setLayout(layout);

	connect(ui.actionRun, SIGNAL(triggered()), this, SLOT(startKinect()));
	connect(ui.action_Screen_shot, SIGNAL(triggered()), this, SLOT(takeScreenshot()));
	connect(ui.actionColor_Over, SIGNAL(triggered()), this, SLOT(switchMode()));
	connect(&timer, SIGNAL(timeout()), this, SLOT(updateStream()));
}

KinectManager::~KinectManager()
{
	sensor->NuiShutdown();

	delete[] rgbdata;
	delete[] depthdata;
}

bool KinectManager::initKinect() {
	// Get a working kinect sensor
	int numSensors;
	if (NuiGetSensorCount(&numSensors) < 0 || numSensors < 1) return false;
	if (NuiCreateSensorByIndex(0, &sensor) < 0) return false;

	// Initialize sensor
	sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH);

	sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,            // Depth camera or rgb camera?
		NUI_IMAGE_RESOLUTION_640x480,    // Image resolution
		0,      // Image stream flags, e.g. near mode
		2,      // Number of frames to buffer
		NULL,   // Event handle
		&rgbStream);

	sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_DEPTH,            // Depth camera or rgb camera?
		NUI_IMAGE_RESOLUTION_640x480,    // Image resolution
		0,      // Image stream flags, e.g. near mode
		2,      // Number of frames to buffer
		NULL,   // Event handle
		&depthStream);

	rgbdata = new unsigned char[width * height * 4];
	depthdata = new unsigned char[width * height * 4];

	return sensor;
}

bool KinectManager::getRGBData() {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return false;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	texture->LockRect(0, &LockedRect, NULL, 0);

	if (LockedRect.Pitch != 0)
	{
		const BYTE* curr = (const BYTE*) LockedRect.pBits;
		const BYTE* dataEnd = curr + (width*height)*4;

		unsigned char* dest = &(rgbdata[0]);

		while (curr < dataEnd) {
			*dest++ = *curr++;
			*dest++ = *curr++;
			*dest++ = *curr++;
			*dest++ = 0xff;
			curr++;
		}
	}

	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);
	return true;
}

bool KinectManager::getDepthData() {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;

	if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &imageFrame) < 0) return false;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	texture->LockRect(0, &LockedRect, NULL, 0);

	if (LockedRect.Pitch != 0)
	{
		const USHORT* curr = (const USHORT*) LockedRect.pBits;
		const USHORT* dataEnd = curr + (width*height);

		unsigned char* dest = &(depthdata[0]);

		for(int i=0, didx=0, dstidx=0;i<height;i++) {
			for(int j=0;j<width;j++, didx++, dstidx+=4) {
				USHORT depth = NuiDepthPixelToDepth(curr[didx]);
				
				if( m == ColorOver ) {
					long x, y;

					NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
						NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, NULL,
						j, i, depth<<3, &x, &y);

					unsigned char r, g, b;
					if (x < 0 || y < 0 || x > width || y > height) {
						r = g = b = 0;
					}
					else {
						const BYTE* rgb = rgbdata + (x + width*y)*4;
						r = rgb[2]; g = rgb[1]; b = rgb[0];
					}

					dest[dstidx] = b;
					dest[dstidx+1] = g;
					dest[dstidx+2] = r;
					dest[dstidx+3] = 0xff;
				}
				else{
					dest[dstidx] = 0;
					dest[dstidx+1] = (BYTE) (depth/256);
					dest[dstidx+2] = (BYTE) (depth%256);
					dest[dstidx+3] = 0xff;
				}
			}
		}
	}

	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(depthStream, &imageFrame);
	return true;
}

QImage toQImage(unsigned char* data, int w, int h)
{
	QImage qimg(w, h, QImage::Format_ARGB32);
	for(int i=0, idx=0;i<h;i++)
	{
		for(int j=0;j<w;j++, idx+=4)
		{
			unsigned char r = data[idx+2];
			unsigned char g = data[idx+1];
			unsigned char b = data[idx];
			unsigned char a = 255;
			QRgb qp = qRgba(r, g, b, a);
			qimg.setPixel(j, i, qp);
		}
	}
	return qimg;
}

void KinectManager::startKinect() {
	timer.start(40);		
}

void KinectManager::switchMode() {
	m = Mode((m+1)%2);
}

void KinectManager::updateStream() {
	if( !getRGBData() ) {
		throw "Unable to get RGB data!";
	}

	if( !getDepthData() ) {
		throw "Unable to get RGB data!";
	}

	colorView->bindStreamData(rgbdata);
	depthView->bindStreamData(depthdata);	
}

void KinectManager::takeScreenshot() {
	QImage rgbImg, dImg;
	if( !getRGBData() ) {
		throw "Unable to get RGB data!";
	}

	if( !getDepthData() ) {
		throw "Unable to get RGB data!";
	}

	colorView->bindStreamData(rgbdata);
	depthView->bindStreamData(depthdata);	

	// create QImages from the data
	rgbImg = toQImage(rgbdata, width, height);
	dImg = toQImage(depthdata, width, height);

	rgbImg.save("rgb.png");		
	dImg.save("depth.png");
}