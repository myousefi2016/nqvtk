#pragma once

#include "RenderStyle.h"

#include "GLBlaat/GL.h"
#include "GLBlaat/GLFramebuffer.h"
#include "GLBlaat/GLProgram.h"
#include "GLBlaat/GLUtility.h"

#include <cassert>

namespace NQVTK
{
	namespace Styles
	{
		class IBIS : public NQVTK::RenderStyle
		{
		public:
			typedef NQVTK::RenderStyle Superclass;

			IBIS() : depthBuffer(0), infoBuffer(0), 
				colors(0), normals(0), infoCurrent(0), infoPrevious(0) { }
			virtual ~IBIS() { }

			virtual GLFramebuffer *CreateFBO(int w, int h)
			{
				GLFramebuffer *fbo = GLFramebuffer::New(w, h);
				fbo->CreateDepthTextureRectangle();
				int nBufs = 3;
				GLenum bufs[] = {
					GL_COLOR_ATTACHMENT0_EXT, 
					GL_COLOR_ATTACHMENT1_EXT, 
					GL_COLOR_ATTACHMENT2_EXT
				};
				for (int i = 0; i < nBufs; ++i)
				{
					fbo->CreateColorTextureRectangle(bufs[i]);
					GLUtility::SetDefaultColorTextureParameters(
						fbo->GetTexture2D(bufs[i]));
				}
				glDrawBuffers(nBufs, bufs);
				if (!fbo->IsOk()) qDebug("WARNING! fbo not ok!");
				fbo->Unbind();

				return fbo;
			}

			virtual GLProgram *CreateScribe()
			{
				GLProgram *scribe = GLProgram::New();
				bool res = scribe->AddVertexShader(
					"uniform float farPlane;"
					"uniform float nearPlane;"
					"varying vec3 normal;"
					"varying vec4 color;"
					"varying float depthInCamera;"
					// Shader main function
					"void main() {"
					"  normal = normalize(gl_NormalMatrix * gl_Normal);"
					"  color = gl_Color;"
					"  vec4 pos = gl_ModelViewMatrix * gl_Vertex;"
					"  float depthRange = (farPlane - nearPlane);"
					"  depthInCamera = (-pos.z / pos.w - nearPlane) / depthRange;"
					"  gl_Position = ftransform();"
					"}");
				if (res) res = scribe->AddFragmentShader(
					"#extension GL_ARB_texture_rectangle : enable\n"
					"uniform sampler2DRectShadow depthBuffer;"
					"uniform sampler2DRect infoBuffer;"
					"uniform int layer;"
					"uniform int objectId;"
					"varying vec3 normal;"
					"varying vec4 color;"
					"varying float depthInCamera;"
					// Rounds a float to the nearest integer
					"float round(float x) {"
					"  return floor(x + 0.5);"
					"}"
					// Encodes a bit set in a float, range [0..1]
					"float setBit(float byte, int bit, bool on) {"
					"  float f = 2.0;"
					"  int N = 8;"
					"  float max = pow(f, float(N)) - 1.0;"
					"  byte = round(byte * max);"
					"  float bf = pow(f, float(bit));"
					"  float b = fract(byte / bf);"
					"  float af = bf * f;"
					"  float a = floor(byte / af);"
					"  float r = bf * b + af * a;"
					"  if (on) r += f / 2.0 * bf;"
					"  return r / max;"
					"}"
					// Gets a single bit from a float-encoded bit set
					"bool getBit(float byte, int bit) {"
					"  float f = 2.0;"
					"  float N = 8.0;"
					"  if (bit > int(N)) return false;"
					"  float mask = round(byte * (pow(f, N) - 1.0)) / f;"
					"  int i;"
					"  for (i = 0; i <= bit - 1; ++i) {"
					"    mask = floor(mask) / f;"
					"  }"
					"  return (fract(mask) > 0.25);"
					"}"
					// Packs a float in two 8 bit channels
					"vec2 encodeDepth(float depth) {"
					"  depth = clamp(depth, 0.0, 1.0);"
					"  vec2 factors = vec2(1.0, 256.0);"
					"  vec2 result = fract(depth * factors);"
					"  result.x -= result.y / factors.y;"
					"  return result;"
					"}"
					// Shader main function
					"void main() {"
					"  vec4 r0 = gl_FragCoord;"
					// Depth peeling
					"  if (layer > 0) {"
					"    float r1 = shadow2DRect(depthBuffer, r0.xyz).x;"
					"    r1 = r1 - 0.5;"
					"    if (r1 < 0.0) { discard; }"
					"  }"
					// Get the previous info buffer
					"  vec4 prevInfo = vec4(0.0);"
					"  if (layer > 0) {"
					"    prevInfo = texture2DRect(infoBuffer, r0.xy);"
					"  }"
					// Coplanarity peeling
					"  if (getBit(prevInfo.y, objectId) == gl_FrontFacing) {"
					"    discard;"
					"  }"
					// Encode identity
					"  float id = float(objectId) + 1.0;"
					"  if (id < 0.0) id = 0.0;"
					"  float identity = id / 9.0;"
					// Encode in-out mask
					"  float inOutMask = prevInfo.y;"
					"  if (objectId >= 0) {"
					"    inOutMask = setBit(inOutMask, objectId, gl_FrontFacing);"
					"  }"
					// Encode depth
					"  vec2 depthVec = encodeDepth(depthInCamera);"
					// Encode normal
					"  vec3 n = (normalize(normal) + vec3(1.0)) / 2.0;"
					// Store data
					"  gl_FragData[0] = color;"
					"  gl_FragData[1] = vec4(n, 1.0);"
					"  gl_FragData[2] = vec4(identity, inOutMask, depthVec);"
					"}");
				if (res) res = scribe->Link();
				qDebug(scribe->GetInfoLogs().c_str());
				if (!res)
				{
					delete scribe;
					return 0;
				}
				return scribe;
			}

			virtual GLProgram *CreatePainter()
			{
				GLProgram *painter = GLProgram::New();
				bool res = painter->AddVertexShader(
					"void main() {"
					"  gl_Position = gl_Vertex;"
					"}");
				if (res) res = painter->AddFragmentShader(
					"#extension GL_ARB_texture_rectangle : enable\n"
					"uniform sampler2DRect normals;"
					"uniform sampler2DRect colors;"
					"uniform sampler2DRect infoPrevious;"
					"uniform sampler2DRect infoCurrent;"
					"uniform int layer;"
					"uniform float farPlane;"
					"uniform float nearPlane;"
					// Rounds a float to the nearest integer
					"float round(float x) {"
					"  return floor(x + 0.5);"
					"}"
					// Unpacks a float from two 8 bit channels
					"float decodeDepth(vec2 coded) {"
					"  vec2 factors = vec2(1.0, 0.00390625);"
					"  return dot(coded, factors);"
					"}"
					// CSG formula
					"bool CSG(float mask) {"
					"  float f = 2.0;"
					"  float N = 8.0;"
					"  mask = round(mask * (pow(f, N) - 1.0)) / f;"
					"  bool inActor0 = fract(mask) > 0.25;"
					"  mask = floor(mask) / f;"
					"  bool inActor1 = fract(mask) > 0.25;"
					"  mask = floor(mask) / f;"
					"  bool inActor2 = fract(mask) > 0.25;"
					"  return inActor0 && inActor1;"
					"}"
					// CSG formula for fogging volumes
					"bool CSGFog(float mask) {"
					"  float f = 2.0;"
					"  float N = 8.0;"
					"  mask = round(mask * (pow(f, N) - 1.0)) / f;"
					"  bool inActor0 = fract(mask) > 0.25;"
					"  mask = floor(mask) / f;"
					"  bool inActor1 = fract(mask) > 0.25;"
					"  mask = floor(mask) / f;"
					"  bool inActor2 = fract(mask) > 0.25;"
					"  return inActor0 || inActor1;"
					"}"
					// Phong shading helper
					"vec3 phongShading(vec3 matColor, vec3 normal) {"
					"  vec3 l = normalize(vec3(gl_LightSource[0].position));"
					"  vec3 n = normalize(normal);"
					"  vec3 h = normalize(gl_LightSource[0].halfVector.xyz);"
					"  float NdotL = dot(n, l);"
					"  float diffuse = 0.5 + 0.5 * min(max(NdotL, 0.0), 1.0);"
					"  float specular = 0.0;"
					"  if (NdotL >= 0.0) {"
					"    specular = pow(max(dot(n, h), 0.0), 64.0);"
					"  }"
					"  return diffuse * matColor + specular * vec3(0.4);"
					"}"
					// Main shader function
					"void main() {"
					"  vec4 r0 = gl_FragCoord;"
					"  vec3 normal = texture2DRect(normals, r0.xy).rgb;"
					"  normal = (2.0 * normal) - vec3(1.0);"
					"  vec4 color = texture2DRect(colors, r0.xy);"
					"  vec4 info0 = texture2DRect(infoCurrent, r0.xy);"
					"  vec4 info1 = texture2DRect(infoPrevious, r0.xy);"
					"  if (layer == 0) info1 = vec4(0.0);"
					// Apply lighting
					"  vec3 litFragment = phongShading(color.rgb, normal);"
					// Apply CSG
					"  float mask0 = info0.y;"
					"  float mask1 = info1.y;"
					"  if (CSG(mask0) != CSG(mask1)) {"
					"    color.a = 1.0;"
					"  } else {"
					"    if (color.a > 0.0) color.a = 1.5 - length(litFragment);"
					"  }"
					// Apply contouring
					"  float contourDepthEpsilon = 0.001;"
					"  vec4 left = texture2DRect(infoCurrent, vec2(r0.x - 1.0, r0.y));"
					"  vec4 top = texture2DRect(infoCurrent, vec2(r0.x, r0.y - 1.0));"
					"  float diffH = abs(decodeDepth(left.zw) - decodeDepth(info0.zw));"
					"  float diffV = abs(decodeDepth(top.zw) - decodeDepth(info0.zw));"
					"  bool contourH = (left.x != info0.x && diffH < contourDepthEpsilon);"
					"  bool contourV = (top.x != info0.x && diffV < contourDepthEpsilon);"
					"  if (contourH || contourV) {"
					"    litFragment = vec3(0.0);"
					"    color.a = 1.0;"
					"  }"
					// Apply fogging
					"  if (CSGFog(mask1)) {"
					"    float depthCueRange = 10.0;"
					"    vec3 fogColor = vec3(1.0, 0.0, 0.2);"
					"    float depthRange = (farPlane - nearPlane);"
					"    float front = decodeDepth(info1.zw) * depthRange;"
					"    float back = decodeDepth(info0.zw) * depthRange;"
					"    float diff = back - front;"
					"    float fogAlpha = 1.0 - "
					"      clamp(exp(-diff / depthCueRange), 0.0, 1.0);"
					"    litFragment = fogColor * fogAlpha + "
					"      litFragment * color.a * (1.0 - fogAlpha);"
					"    color.a = fogAlpha + color.a * (1.0 - fogAlpha);"
					"    litFragment /= color.a;"
					"  }"
					// Pre-multiply colors by alpha
					"  litFragment *= color.a;"
					"  gl_FragColor = vec4(litFragment, color.a);"
					"}");
				if (res) res = painter->Link();
				qDebug(painter->GetInfoLogs().c_str());
				if (!res) 
				{
					delete painter;
					return 0;
				}
				return painter;
			}

			virtual void BindScribeTextures(GLProgram *scribe, 
				GLFramebuffer *previous) 
			{
				assert(!depthBuffer && !infoBuffer);

				// Get the previous layer's info buffer
				infoBuffer = previous->GetTexture2D(GL_COLOR_ATTACHMENT2_EXT);
				assert(infoBuffer);
				glActiveTexture(GL_TEXTURE1);
				infoBuffer->BindToCurrent();
				scribe->UseTexture("infoBuffer", 1);

				// Get the previous layer's depth buffer
				depthBuffer = previous->GetTexture2D(GL_DEPTH_ATTACHMENT_EXT);
				assert(depthBuffer);
				glActiveTexture(GL_TEXTURE0);
				depthBuffer->BindToCurrent();
				GLUtility::SetDefaultDepthTextureParameters(depthBuffer);
				glTexParameteri(depthBuffer->GetTextureTarget(), 
					GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
				scribe->UseTexture("depthBuffer", 0);
			}

			virtual void UnbindScribeTextures() 
			{
				assert(depthBuffer && infoBuffer);

				glActiveTexture(GL_TEXTURE1);
				infoBuffer->UnbindCurrent();
				infoBuffer = 0;
				glActiveTexture(GL_TEXTURE0);
				glTexParameteri(depthBuffer->GetTextureTarget(), 
						GL_TEXTURE_COMPARE_MODE, GL_NONE);
				depthBuffer->UnbindCurrent();
				depthBuffer = 0;
			}

			virtual void BindPainterTextures(GLProgram *painter, 
				GLFramebuffer *current, GLFramebuffer *previous) 
			{
				assert(!colors && !normals && !infoCurrent && !infoPrevious);

				// Previous layer info buffer
				infoPrevious = previous->GetTexture2D(GL_COLOR_ATTACHMENT2_EXT);
				assert(infoPrevious);
				glActiveTexture(GL_TEXTURE3);
				infoPrevious->BindToCurrent();
				painter->UseTexture("infoPrevious", 3);
				// Current layer info buffer
				infoCurrent = current->GetTexture2D(GL_COLOR_ATTACHMENT2_EXT);
				assert(infoCurrent);
				glActiveTexture(GL_TEXTURE2);
				infoCurrent->BindToCurrent();
				painter->UseTexture("infoCurrent", 2);
				// Current layer normals
				normals = current->GetTexture2D(GL_COLOR_ATTACHMENT1_EXT);
				assert(normals);
				glActiveTexture(GL_TEXTURE1);
				normals->BindToCurrent();
				painter->UseTexture("normals", 1);
				// Current layer colors
				colors = current->GetTexture2D(GL_COLOR_ATTACHMENT0_EXT);
				assert(colors);
				glActiveTexture(GL_TEXTURE0);
				colors->BindToCurrent();
				painter->UseTexture("colors", 0);
			}

			virtual void UnbindPainterTextures() 
			{
				assert(colors && normals && infoCurrent && infoPrevious);

				glActiveTexture(GL_TEXTURE3);
				infoPrevious->UnbindCurrent();
				infoPrevious = 0;
				glActiveTexture(GL_TEXTURE2);
				infoCurrent->UnbindCurrent();
				infoCurrent = 0;
				glActiveTexture(GL_TEXTURE1);
				normals->UnbindCurrent();
				normals = 0;
				glActiveTexture(GL_TEXTURE0);
				colors->UnbindCurrent();
				colors = 0;
			}

		protected:
			// Scribe textures
			GLTexture *depthBuffer;
			GLTexture *infoBuffer;
			// Painter textures
			GLTexture *colors;
			GLTexture *normals;
			GLTexture *infoCurrent;
			GLTexture *infoPrevious;

		private:
			// Not implemented
			IBIS(const IBIS&);
			void operator=(const IBIS&);
		};
	}
}
