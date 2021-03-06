#include "OrbitCameraInteractor.h"

#include "Rendering/OrbitCamera.h"

#include <cassert>

namespace NQVTK
{
	// ------------------------------------------------------------------------
	OrbitCameraInteractor::OrbitCameraInteractor(OrbitCamera *orbitCam) 
		: CameraInteractor(orbitCam)
	{
		assert(orbitCam);
		lastX = lastY = 0;
	}

	// ------------------------------------------------------------------------
	bool OrbitCameraInteractor::MouseMoveEvent(MouseEvent event)
	{
		bool handled = false;

		OrbitCamera *camera = dynamic_cast<OrbitCamera*>(this->camera);
		assert(camera);

		// Mouse controls camera
		if (event.buttons & MouseEvent::LeftButton)
		{
			// Rotate
			camera->rotateY += event.x - lastX;
			camera->rotateX -= event.y - lastY;
			if (camera->rotateX > 80.0) camera->rotateX = 80.0;
			if (camera->rotateX < -80.0) camera->rotateX = -80.0;
			handled = true;
		}

		if (event.buttons & MouseEvent::RightButton)
		{
			// Zoom
			camera->zoom += (event.y - lastY) * 0.01f;
			if (camera->zoom < 0.05) camera->zoom = 0.05;
			if (camera->zoom > 20.0) camera->zoom = 20.0;
			handled = true;
		}

		if (handled) camera->Update();

		lastX = event.x;
		lastY = event.y;

		if (!handled) return Superclass::MouseMoveEvent(event);
		return handled;
	}
}
