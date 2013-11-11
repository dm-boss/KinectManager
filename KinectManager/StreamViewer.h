#pragma once

#include "Canvas/gl2dcanvas.h"

class StreamViewer : public GL2DCanvas
{
public:
	StreamViewer(QWidget* parent = 0);
	~StreamViewer(void);

	void bindStreamData(const unsigned char* data);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

private:
	GLuint textureId;
};

