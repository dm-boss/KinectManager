#include "kinectmanager.h"
#include <QtWidgets/QApplication>

#include <iostream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	KinectManager w;
	w.show();

	return a.exec();
}
