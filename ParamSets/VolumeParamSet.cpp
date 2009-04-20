#pragma once

#include "VolumeParamSet.h"

#include "Rendering/ImageDataTexture3D.h"

#include "GLBlaat/GLProgram.h"

namespace NQVTK
{
	// ------------------------------------------------------------------------
	VolumeParamSet::VolumeParamSet(
		ImageDataTexture3D *volume)
	{
		this->volume = volume;
	}

	// ------------------------------------------------------------------------
	VolumeParamSet::~VolumeParamSet()
	{
		delete volume;
	}

	// ------------------------------------------------------------------------
	void VolumeParamSet::SetupProgram(GLProgram *program)
	{
		if (volume)
		{
			program->SetUniform1i("hasVolume", 1);
			program->SetUniform1f("volumeDataShift", 
				static_cast<float>(volume->GetDataShift()));
			program->SetUniform1f("volumeDataScale", 
				static_cast<float>(volume->GetDataScale()));
			Vector3 origin = volume->GetOrigin();
			program->SetUniform3f("volumeOrigin", 
				static_cast<float>(origin.x), 
				static_cast<float>(origin.y), 
				static_cast<float>(origin.z));
			Vector3 size = volume->GetOriginalSize();
			program->SetUniform3f("volumeSize", 
				static_cast<float>(size.x), 
				static_cast<float>(size.y), 
				static_cast<float>(size.z));
		}
		else
		{
			program->SetUniform1i("hasVolume", 0);
		}
	}

	// ------------------------------------------------------------------------
	void VolumeParamSet::SetupProgramArrays(
		GLProgram *program, int objectId)
	{
		program->SetUniform1f(
			GetArrayName("volumeDataShift", objectId), 
			static_cast<float>(volume->GetDataShift()));
		program->SetUniform1f(
			GetArrayName("volumeDataScale", objectId), 
			static_cast<float>(volume->GetDataScale()));
		NQVTK::Vector3 origin = volume->GetOrigin();
		program->SetUniform3f(
			GetArrayName("volumeOrigin", objectId), 
			static_cast<float>(origin.x), 
			static_cast<float>(origin.y), 
			static_cast<float>(origin.z));
		NQVTK::Vector3 size = volume->GetOriginalSize();
		program->SetUniform3f(
			GetArrayName("volumeSize", objectId), 
			static_cast<float>(size.x), 
			static_cast<float>(size.y), 
			static_cast<float>(size.z));
		program->SetUniform3f(
			GetArrayName("volumeSpacing", objectId), 
			static_cast<float>(
				size.x / static_cast<double>(volume->GetWidth() - 1)), 
			static_cast<float>(
				size.y / static_cast<double>(volume->GetHeight() - 1)), 
			static_cast<float>(
				size.z / static_cast<double>(volume->GetDepth() - 1)));
	}
}