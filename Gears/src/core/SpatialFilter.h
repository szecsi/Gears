#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include "pointgrid.hpp"
#include "quad.hpp"

#include <string>
#include <map>
#include <list>
#include "Sequence.h"

//! Structure describing a spatial filter logic.
class SpatialFilter
{
	SpatialFilter(){}
public:
	GEARS_SHARED_CREATE(SpatialFilter);

	std::string name;
	std::string kernelFuncSource;
	std::string kernelProfileVertexSource;
	std::string kernelProfileFragmentSource;

	float width_um;
	float height_um;
	float minimum;
	float maximum;

	bool useFft;
	bool separable;
	bool kernelGivenInFrequencyDomain;

	uint horizontalSampleCount;
	uint verticalSampleCount;

	using ShaderVariableMap = std::map<std::string, float>;
	ShaderVariableMap shaderVariables;
	using ShaderColorMap = std::map<std::string, Gears::Math::float3>;
	ShaderColorMap shaderColors;
	using ShaderVectorMap = std::map<std::string, Gears::Math::float2>;
	ShaderVectorMap shaderVectors;
	using ShaderFunctionMap = std::map<std::string, std::string>;
	ShaderFunctionMap shaderFunctions;
	using ShaderFunctionOrder = std::list<std::string>;
	ShaderFunctionOrder shaderFunctionOrder;

	SpatialFilter(std::string name);

	void setShaderVariable(std::string varName, float value)
	{
		shaderVariables[varName] = value;
	}

	void setShaderColor(std::string varName, float all, float r, float g, float b)
	{
		if(all >= -1)
			shaderColors[varName] = Gears::Math::float3(all, all, all);
		else
			shaderColors[varName] = Gears::Math::float3(r, g, b);
	}

	void setShaderVector(std::string varName, float x, float y)
	{
		shaderVectors[varName] = Gears::Math::float2(x, y);
	}

	void setShaderFunction(std::string funcName, std::string source)
	{
		auto i = shaderFunctions.find(name);
		if(i == shaderFunctions.end())
		{
			shaderFunctionOrder.push_back(name);
		}
		shaderFunctions[name] = source;
	}

	std::string getKernelGeneratorShaderSource() const
	{
		std::string s("#version 150\n");
		for(auto& svar : shaderColors)
		{
			s += "uniform vec3 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVectors)
		{
			s += "uniform vec2 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVariables)
		{
			s += "uniform float ";
			s += svar.first;
			s += ";\n";
		}
		for(std::string sfunc : shaderFunctionOrder)
		{
			s += shaderFunctions.find(sfunc)->second;
			s += ";\n";
		}
		return s + kernelFuncSource;
	}

	std::string getProfileVertexShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		for(auto& svar : shaderColors)
		{
			s += "uniform vec3 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVectors)
		{
			s += "uniform vec2 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVariables)
		{
			s += "uniform float ";
			s += svar.first;
			s += ";\n";
		}
		for(std::string sfunc : shaderFunctionOrder)
		{
			s += shaderFunctions.find(sfunc)->second;
			s += "\n";
		}
		return s + kernelProfileVertexSource;
	}

	std::string getProfileFragmentShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		for(auto& svar : shaderColors)
		{
			s += "uniform vec3 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVectors)
		{
			s += "uniform vec2 ";
			s += svar.first;
			s += ";\n";
		}
		for(auto& svar : shaderVariables)
		{
			s += "uniform float ";
			s += svar.first;
			s += ";\n";
		}
		for(std::string sfunc : shaderFunctionOrder)
		{
			s += shaderFunctions.find(sfunc)->second;
			s += "\n";
		}
		return s + kernelProfileFragmentSource;
	}

};