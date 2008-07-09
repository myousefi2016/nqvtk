#pragma once

#include "Renderer.h"
#include "Styles/ShadowMap.h"

namespace NQVTK
{
	class ShadowMappingRenderer : public Renderer
	{
	public:
		typedef Renderer Superclass;

		ShadowMappingRenderer() : Renderer() 
		{
			shadowBuffer = 0;
			shadowStyle = new NQVTK::Styles::ShadowMap();
			shadowRenderer = new NQVTK::Renderer();
		}

		virtual ~ShadowMappingRenderer() 
		{
			if (shadowBuffer) delete shadowBuffer;
			delete shadowStyle;
			// Clear the shadow renderables first
			shadowRenderer->SetRenderables(std::vector<Renderable*>());
			delete shadowRenderer;
		}

		virtual bool Initialize()
		{
			if (!Superclass::Initialize()) return false;
			
			// Set up the shadow renderer
			shadowRenderer->SetStyle(shadowStyle);
			return shadowRenderer->Initialize();
		}

		virtual void Resize(int w, int h)
		{
			// Resize buffers managed by the parent class
			Superclass::Resize(w, h);

			// Resize the shadow renderer
			shadowRenderer->Resize(512, 512);

			// Resize or recreate shadow buffer
			if (!shadowBuffer)
			{
				shadowBuffer = shadowStyle->CreateShadowBufferFBO(512, 512);
				shadowRenderer->SetTarget(shadowBuffer);
			}
			else
			{
				//if (!shadowBuffer->Resize(w, h)) qDebug("WARNING! shadowBuffer resize failed!");
			}
		}

		virtual void Draw()
		{
			// Synchronize renderer state
			shadowRenderer->SetRenderables(renderables);
			shadowRenderer->GetCamera()->rotateX = camera->rotateX;
			shadowRenderer->GetCamera()->rotateY = camera->rotateY - 45.0;
			shadowRenderer->maxLayers = maxLayers;

			// Draw the shadow map
			shadowRenderer->Draw();
			
			// Get the shadow map
			GLTexture *shadowMap = shadowBuffer->GetTexture2D(GL_COLOR_ATTACHMENT0_EXT);
			tm->AddTexture("shadowMap", shadowMap, false);

			// Get the modelview and projection matrices for the light's camera
			shadowRenderer->GetCamera()->Draw();
			float shadowNear = shadowRenderer->GetCamera()->nearZ;
			float shadowFar = shadowRenderer->GetCamera()->farZ;
			float lmvm[16];
			float lpm[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, lmvm);
			glGetFloatv(GL_PROJECTION_MATRIX, lpm);
			// Draw the camera to get the object modelview transform
			DrawCamera();
			// Get the object modelview matrix and its inverse
			float mvm[16];
			float inv[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, mvm);
			glGetFloatv(GL_PROJECTION_MATRIX, inv);
			Matrix4x4Inverse(mvm, inv);
			// Load the third texture matrix with the transform for shadow mapping
			// (the first two are currently used for object transforms)
			// TODO: this assumes two renderables, it's probably better to use custom uniforms
			glActiveTexture(GL_TEXTURE3);
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			// - Screen coordinates to texture coordinates
			glTranslated(0.5, 0.5, 0.5);
			glScaled(0.5, 0.5, 0.5);
			// - Add the light's projection and modelview matrices
			glMultMatrixf(lpm);
			glMultMatrixf(lmvm);
			// - Finally, add the inverse modelview transform
			glMultMatrixf(inv);
			glMatrixMode(GL_MODELVIEW);

			// Set some extra shadow parameters
			scribe->Start();
			scribe->SetUniform1f("shadowNearPlane", shadowNear);
			scribe->SetUniform1f("shadowFarPlane", shadowFar);
			scribe->SetUniformMatrix4fv("shadowMVM", 1, lmvm);
			scribe->SetUniformMatrix4fv("shadowPM", 1, lpm);
			scribe->Stop();

			// Draw the normal pass
			Superclass::Draw();

			// DEBUG: show shadow buffer
			glDisable(GL_DEPTH_TEST);
			TestDrawTexture(shadowMap, 0.5, 1.0, 0.5, 1.0);
			glEnable(GL_DEPTH_TEST);
		}

	protected:
		GLFramebuffer *shadowBuffer;
		NQVTK::Styles::ShadowMap *shadowStyle;
		NQVTK::Renderer *shadowRenderer;

		void Matrix4x4Inverse(const float (&mat)[16], float (&inv)[16]) 
		{
			// TODO: properly invert matrix, this only works for affine matrices
			// Transpose 3x3 top left 
			for (int i = 0; i < 3; ++i) 
			{
				for (int j = 0; j < 3; ++j) 
				{
					inv[i + j * 4] = mat[j + i * 4];
				}
			}
			// Rotate negated translation vector
			inv[12] = -mat[12]*inv[ 0] - mat[13]*inv[ 4] - mat[14]*inv[ 8];
			inv[13] = -mat[12]*inv[ 1] - mat[13]*inv[ 5] - mat[14]*inv[ 9];
			inv[14] = -mat[12]*inv[ 2] - mat[13]*inv[ 6] - mat[14]*inv[10];
			// Copy remaining elements
			inv[ 3] = mat[ 3];
			inv[ 7] = mat[ 7];
			inv[11] = mat[11];
			inv[15] = mat[15];
		}
	};
}
