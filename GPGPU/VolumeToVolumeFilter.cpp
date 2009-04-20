#pragma once

#include "VolumeToVolumeFilter.h"

#include "GLBlaat/GL.h"
#include "GLBlaat/GLProgram.h"
#include "GLBlaat/GLTexture3D.h"
#include "GLBlaat/GLTextureManager.h"
#include "GLBlaat/GLFramebuffer.h"
#include "GLBlaat/GLRenderTexture3DLayer.h"

#include "Shaders.h"

#include <cassert>
#include <iostream>

namespace NQVTK
{
	namespace GPGPU
	{
		// --------------------------------------------------------------------
		VolumeToVolumeFilter::VolumeToVolumeFilter()
			: tm(0)
		{
		}

		// --------------------------------------------------------------------
		bool VolumeToVolumeFilter::Setup(GLTexture3D *input)
		{
            tm = GLTextureManager::New();

			assert(input);
			this->input = input;

			// TODO: program could be a parameter
			std::cout << "Creating filter..." << std::endl;
			program = CreateProgram();
			if (!program)
			{
				std::cerr << "Could not create filter program!" << std::endl;
				return false;
			}

			return true;
		}

		// --------------------------------------------------------------------
		GLTexture3D *VolumeToVolumeFilter::Execute()
		{
			// NOTE: hardcoded for seperable convolution for testing purposes
			// TODO: use inheritance to implement more complicated filters
			// such as convolution

			// TODO: create non-imagedata volume with metadata (+ paramset)

			// We use seperability of the kernel to perform convolution in 
			// three passes. In order to keep the shader simple we rotate the 
			// dimensions of the texture for each pass. This way, each pass 
			// convolves along its input's x-axis and writes the result in the 
			// y/z planes, which are actually the x/y planes for the next pass.

			// We'll need a few helpers for this, with rotated dimensions.
			GLTexture3D *helperYZX = GLTexture3D::New(
				input->GetHeight(), input->GetDepth(), input->GetWidth(), 
				input->GetInternalFormat(), 
				input->GetDataFormat(), input->GetDataType(), 0);

			GLTexture3D *helperZXY = GLTexture3D::New(
				input->GetDepth(), input->GetWidth(), input->GetHeight(), 
				input->GetInternalFormat(), 
				input->GetDataFormat(), input->GetDataType(), 0);

			if (!helperYZX || !helperZXY) 
			{
				std::cerr << "Could not create helper textures!" << std::endl;
				return 0;
			}

			// Create output texture
			GLTexture3D *output = GLTexture3D::New(
				input->GetWidth(), input->GetHeight(), input->GetDepth(), 
				input->GetInternalFormat(), 
				input->GetDataFormat(), input->GetDataType(), 0);

			if (!output)
			{
				std::cerr << "Could not create output texture!" << std::endl;
				return 0;
			}

			// Prepare GL state
			glPushAttrib(GL_ALL_ATTRIB_BITS);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			// Execute convolution passes
			ExecutePass(input, helperYZX);
			ExecutePass(helperYZX, helperZXY);
			ExecutePass(helperZXY, output);

			glPopAttrib();

			return output;
		}

		// --------------------------------------------------------------------------
		GLProgram *VolumeToVolumeFilter::CreateProgram()
		{
			GLProgram *program = GLProgram::New();
			if (!program) return 0;
			bool res = program->AddVertexShader(
				Shaders::GenericPainterVS);
			if (res) res = program->AddFragmentShader(
				Shaders::ConvolutionFilter);
			if (res) res = program->Link();
			std::cout << program->GetInfoLogs() << std::endl;
			if (!res)
			{
				delete program;
				return 0;
			}
			return program;
		}

		// --------------------------------------------------------------------------
		void VolumeToVolumeFilter::ExecutePass(
			GLTexture3D *input, GLTexture3D *output)
		{
			// Set the input
			tm->AddTexture("volume", input, false);

			// Prepare an FBO for the output
			GLFramebuffer *fbo = GLFramebuffer::New(
				output->GetWidth(), output->GetHeight());

			// Run over the slices of the output
			// We render 4 slices simultaneously
			int numSlices = output->GetDepth();
			for (int slice = 0; slice < numSlices; slice += 4)
			{
				// Attach the slices
				for (int i = 0; i < 4; ++i)
				{
					GLRenderTexture3DLayer *rt = 
						GLRenderTexture3DLayer::New(output, slice + i);
					GLRendertarget *rtOld = fbo->AttachRendertarget(
						GL_COLOR_ATTACHMENT0_EXT + i, rt);
					delete rtOld;
				}
				// Check the FBO
				if (!fbo->IsOk())
				{
					std::cerr << "FBO for output volume not ok!" << std::endl;
					delete fbo;
					return;
				}

				// FBO should still be bound after this

				// Start the program
				program->Start();
				// TODO: pass volume metadata for spacing etc.
				program->SetUniform3f("volumeDims", 
					static_cast<float>(output->GetWidth()), 
					static_cast<float>(output->GetHeight()), 
					static_cast<float>(output->GetDepth()));
				program->SetUniform1i("slice", slice);
				program->SetUniform1f("scale", 1.0f);
				
				tm->SetupProgram(program);
				tm->Unbind();

				// Render a full screen quad to process the slices
				glColor3d(1.0, 1.0, 1.0);
				glBegin(GL_QUADS);
				glVertex3d(-1.0, -1.0, 0.0);
				glVertex3d(1.0, -1.0, 0.0);
				glVertex3d(1.0, 1.0, 0.0);
				glVertex3d(-1.0, 1.0, 0.0);
				glEnd();

				program->Stop();
			}

			// Clean up
			delete fbo;
		}
	}
}
