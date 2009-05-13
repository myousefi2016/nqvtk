#pragma once

#include "Volume.h"

class vtkImageData;

namespace NQVTK
{
	class ImageDataVolume : public Volume
	{
	public:
		typedef Volume Superclass;

		static ImageDataVolume *New(vtkImageData *data);

	protected:
		ImageDataVolume(int width, int height, int depth, 
			int internalformat);
	};
}