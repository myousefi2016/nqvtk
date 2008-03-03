#pragma once

#include <QGLWidget>

#include "Rendering/Renderable.h"
#include "Rendering/Camera.h"

class NQVTKWidget : public QGLWidget {
	Q_OBJECT

public:
	// TODO: add full QGLWidget constructors
	NQVTKWidget(QWidget *parent = 0);
	virtual ~NQVTKWidget();

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

private:
	void timerEvent(QTimerEvent *event);
	void keyPressEvent(QKeyEvent *event);

	NQVTK::Renderable *renderable;
	NQVTK::Camera *camera;
};
