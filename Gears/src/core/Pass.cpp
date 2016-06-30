#include "stdafx.h"
#include "PythonDict.h"

#include "Pass.h"
#include "Stimulus.h"
#include "Sequence.h"
#include "PythonDict.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <limits>

Pass::Pass():
	name("N/A"),
	brief("<no description>"),
	stimulus(nullptr),
	duration(1),
	startingFrame(0),
	videoFileName(""),
	rasterizationMode(RasterizationMode::fullscreen)
{
	setShaderFunction("intensity", "vec3 intensity(float time){return vec3(0.0, 0.0, 0.0);}");

	timelineVertexShaderSource = 
	"	uniform float frameInterval;	\n"
	"	uniform int startFrame;	\n"
	"	uniform int stride;	\n"
	"	void main(void) {		 \n"
//	"		vec3 intensityRgb =	vec3(sin((gl_VertexID/2 * stride + startFrame)*frameInterval), 0, 0);											\n"
	"		vec3 intensityRgb =	plottedIntensity((gl_VertexID/2 * stride + startFrame)*frameInterval);											\n"
	"		float maxIntensity = max(intensityRgb.r, max(intensityRgb.g, intensityRgb.b));						\n"
	"		gl_Position	= gl_ModelViewMatrix * vec4(float((gl_VertexID+1)/2  * stride + startFrame), maxIntensity, 0.5, 1.0);				\n"
	"	}																										\n"
	;
	timelineFragmentShaderSource = 
	"	void main() {																							\n"
	"		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);															\n"
	"	}																										\n"
	;
}

Pass::~Pass()
{
}

boost::python::object Pass::setJoiner(boost::python::object joiner)
{
	this->joiner = joiner;
	return boost::python::object();
}

float Pass::getDuration_s() const
{
	return duration * stimulus->getSequence()->getFrameInterval_s();
}

boost::python::object Pass::setPythonObject(boost::python::object o)
{
	pythonObject = o;
	return boost::python::object();
}

boost::python::object Pass::getPythonObject()
{
	return pythonObject;
}


boost::shared_ptr<Stimulus> Pass::getStimulus() const
{
	return stimulus;
}

boost::shared_ptr<Sequence> Pass::getSequence() const
{
	return stimulus->getSequence();
}


void Pass::setPolygonMask(std::string mode, boost::python::object o)
{
	using namespace boost::python;

	if(mode == "fullscreen")
	{
		rasterizationMode = RasterizationMode::fullscreen;
	}
	else if(mode == "triangles")
	{
		extract<list> exl(o);
		if(!exl.check())
		{
			extract<std::string> exs(o);
			if(!exs.check())
			{
				std::stringstream ss;
				ss << "In 'triangle' mode, polygonMask must a list of float2 dicts! e.g. [{'x':1, 'y':1},{'x':0,'y':1},{'x':0,'y':0}]" ;
				PyErr_SetString(PyExc_TypeError, ss.str().c_str());
				boost::python::throw_error_already_set();
			}
		}
		else
		{
			list l = exl();
			polygonMask.clear();

			rasterizationMode = RasterizationMode::triangles;

			for(int i=0; i < len(l); i++)
			{
				{
					extract<dict> pd(l[i]);
					if(!pd.check()) 
					{
						std::stringstream ss;
						ss << "In 'triangle' mode, polygonMask must be a list of float2 dicts! e.g. [{'x':1, 'y':1},{'x':0,'y':1},{'x':0,'y':0}]" ;
						PyErr_SetString(PyExc_TypeError, ss.str().c_str());
						boost::python::throw_error_already_set();
					}
					Gears::PythonDict d(pd);
					polygonMask.push_back( d.asFloat3().xy );
				}

			}
		}
	}
	else if(mode == "quads")
	{
		extract<list> exl(o);
		if(!exl.check())
		{
			extract<std::string> exs(o);
			if(!exs.check())
			{
				std::stringstream ss;
				ss << "In quads mode, polygonMask must be a list of quad dicts! e.g. [{'x':1, 'y':1, 'width':50, 'height':50, 'figure':0}]" ;
				PyErr_SetString(PyExc_TypeError, ss.str().c_str());
				boost::python::throw_error_already_set();
			}
		}
		else
		{
			list l = exl();
			quads.clear();

			rasterizationMode = RasterizationMode::quads;

			for(int i=0; i < len(l); i++)
			{
				{
					extract<dict> pd(l[i]);
					if(!pd.check()) 
					{
						std::stringstream ss;
						ss << "In quads mode, polygonMask must be a list of quad dicts! e.g. [{'x':1, 'y':1, 'width':50, 'height':50, 'figure':0}]" ;						PyErr_SetString(PyExc_TypeError, ss.str().c_str());
						boost::python::throw_error_already_set();
					}
					Gears::PythonDict d(pd);
					QuadData qd;
					qd.x = d.getFloat("x");
					qd.y = d.getFloat("y");
					qd.halfwidth = d.getFloat("width")*0.5;
					qd.halfheight = d.getFloat("height")*0.5;
					qd.figure = d.getFloat("figure");
					qd.motion = d.getFloat("motion");
					quads.push_back( qd );
				}
			}
		}
	}
}

void Pass::setVideo(std::string videoFileName)
{
	this->videoFileName = videoFileName;
}

void Pass::setStimulus(boost::shared_ptr<Stimulus> stimulus)
{
	this->stimulus = stimulus;
	if(stimulus->doesToneMappingInStimulusGenerator)
	{
		setShaderFunction("toneMap", 
		"	uniform sampler1D gamma;																	\n"
		"	uniform float dynamicRangeMin;																			\n"
		"	uniform float dynamicRangeMax;																			\n"
		"	uniform float dynamicRangeMean;																			\n"
		"	uniform float dynamicRangeVar;																			\n"
		"	uniform int gammaSampleCount;																			\n"
		"	uniform bool calibrating;																				\n"
		"	vec3 toneMap(vec3 color){																				\n"
		"	vec3 outcolor = color;																					\n"
		"		if(dynamicRangeVar >= 0)																			\n"
		"		{																									\n"
		"			outcolor.r = 1 - 1 / (1 + exp((outcolor.r - dynamicRangeMean)/dynamicRangeVar));					\n"
		"			outcolor.g = 1 - 1 / (1 + exp((outcolor.g - dynamicRangeMean)/dynamicRangeVar));					\n"
		"			outcolor.b = 1 - 1 / (1 + exp((outcolor.b - dynamicRangeMean)/dynamicRangeVar));					\n"
		"		}																									\n"
		"		else																								\n"
		"		{																									\n"
		"			outcolor.r = (outcolor.r - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"			outcolor.g = (outcolor.g - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"			outcolor.b = (outcolor.b - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"		}																									\n"
		"																											\n"
		"		if(!calibrating)																					\n"
		"		{																									\n"
		"			{																								\n"
		"			outcolor.r = clamp(outcolor.r, 0, 1);															\n"
		"			float gammaIndex = outcolor.r * (gammaSampleCount-1) + 0.5;											\n"
		"			outcolor.r = texture(gamma, gammaIndex/256.0).x;														\n"
		"			}																								\n"
		"			{																								\n"
		"			outcolor.g = clamp(outcolor.g, 0, 1);															\n"
		"			float gammaIndex = outcolor.g * (gammaSampleCount-1) + 0.5;											\n"
		"			outcolor.g = texture(gamma, gammaIndex/256.0).x;														\n"
		"			}																								\n"
		"			{																								\n"
		"			outcolor.b = clamp(outcolor.b, 0, 1);															\n"
		"			float gammaIndex = outcolor.b * (gammaSampleCount-1) + 0.5;											\n"
		"			outcolor.b = texture(gamma, gammaIndex/256.0).x;														\n"
		"			}																								\n"
		"		}																									\n"
		"	return outcolor; }																					\n"
		);
	}
	else
		setShaderFunction("toneMap", "vec3 toneMap(vec3 color){return color;}");

	if(stimulus->temporalProcessingStateCount > 0)
		setShaderFunction("temporalProcess", 
		"	uniform mat4x4 stateTransitionMatrix;								\n"
		"	uniform sampler2DArray temporalProcessingState;												\n"
		"		vec4 temporalProcess(vec3 inputColor, vec2 x){							\n"
		"	return vec4(inputColor.r, texture(temporalProcessingState, vec3(x, 0)).rgb) * stateTransitionMatrix;													\n"
		"	}	");
	else
		setShaderFunction("temporalProcess", "vec4 temporalProcess(vec3 inputColor, vec2 x){return vec4(inputColor, 1);}");
}

void Pass::registerTemporalFunction(std::string functionName, std::string displayName)
{
	temporalShaderFunctions[displayName] = functionName;
}

std::string Pass::getStimulusGeneratorVertexShaderSource(Pass::RasterizationMode mode) const
{
	if(mode == Pass::RasterizationMode::fullscreen)
	{
		std::string s(R"GLSLCODE(
			#version 150

			uniform vec2 patternSizeOnRetina;

			out vec2 pos;
			out vec2 fTexCoord;

			void main(void) {
			   gl_Position	= vec4(float(gl_VertexID / 2)*2.0-1.0, 1.0-float(gl_VertexID % 2)*2.0, 0.5, 1.0);
			   vec2 texCoord = vec2(float(gl_VertexID / 2), float(gl_VertexID % 2));
			   fTexCoord = texCoord;
			   fTexCoord.y = -fTexCoord.y;
			   fTexCoord.y += 1;
			   pos = (texCoord - vec2(0.5, 0.5) ) * patternSizeOnRetina ;
			}
			)GLSLCODE");
		return s;
	}
	if(mode == Pass::RasterizationMode::triangles)
	{
		std::string s(R"GLSLCODE(
			#version 150
			uniform vec2 patternSizeOnRetina;
			uniform sampler2D vertices;
			uniform float time;
			)GLSLCODE");
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
		for(auto gsf : geomShaderFunctions)
			s += gsf.second;
		s += stimulusGeneratorGeometryShaderMotionTransformFunction;
		s += R"GLSLCODE(
			out vec2 pos;
			void main(void) {
				vec2 qpos =  texelFetch(vertices, ivec2(gl_VertexID, 0), 0).xy;
			
				gl_Position = vec4(polygonMotionTransform(time) * vec3(qpos, 1) / patternSizeOnRetina * 2, 0.5, 1);
			}
			)GLSLCODE";
		return s;
	}
	if(mode == Pass::RasterizationMode::quads)
	{
		//std::string s(R"GLSLCODE(
			//#version 150
			//uniform vec2 patternSizeOnRetina;
			//#ifndef GEARS_RANDOMS_RESOURCES
			//#define GEARS_RANDOMS_RESOURCES
			//uniform usampler2D randoms;
			//uniform vec2 cellSize;
			//uniform ivec2 randomGridSize;
			//#endif
			//#ifndef GEARS_PARTICLE_SYSTEM_RESOURCES
			//#define GEARS_PARTICLE_SYSTEM_RESOURCES
			//uniform usampler2D particles;
			//#endif
			//uniform sampler2D quads;
			//uniform float time;
			//)GLSLCODE");
			std::string s(R"GLSLCODE(
				#version 150
				void main(void) {}
				)GLSLCODE");
			return s;
	}
	else
		return "";
}

std::string Pass::getStimulusGeneratorGeometryShaderSource(Pass::RasterizationMode mode) const
{
	if(mode == Pass::RasterizationMode::quads)
	{
		std::string s(R"GLSLCODE(
			#version 150
			uniform vec2 patternSizeOnRetina;
			#ifndef GEARS_RANDOMS_RESOURCES
			#define GEARS_RANDOMS_RESOURCES
			uniform usampler2D randoms;
			uniform vec2 cellSize;
			#endif
			#ifndef GEARS_PARTICLE_SYSTEM_RESOURCES
			#define GEARS_PARTICLE_SYSTEM_RESOURCES
			uniform usampler2D particles;
			#endif
			uniform sampler2D quads;
			uniform float time;
			)GLSLCODE");
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
		for(auto gsf : geomShaderFunctions)
			s += gsf.second;
		s += stimulusGeneratorGeometryShaderMotionTransformFunction;
		s += R"GLSLCODE(
			out vec2 pos;
			out vec2 figmotid;
			void main(void) {
				vec2 qpos =  texelFetch(quads, ivec2(gl_PrimitiveIDIn * 3 + 0, 0), 0).xy;
				vec2 qsize = texelFetch(quads, ivec2(gl_PrimitiveIDIn * 3 + 1, 0), 0).xy;
				figmotid   = texelFetch(quads, ivec2(gl_PrimitiveIDIn * 3 + 2, 0), 0).xy;
				ivec2 randomGridSize = textureSize(randoms, 0);
				ivec2 iid = ivec2(gl_PrimitiveIDIn % randomGridSize.x, gl_PrimitiveIDIn / randomGridSize.x);
			
				pos = qpos + motionTransform(qsize, time, figmotid.y, iid);
				gl_Position = vec4(pos / patternSizeOnRetina * 2, 0, 1);
				pos = qsize;
				EmitVertex();
				pos = qpos + motionTransform(qsize * vec2(-1, 1), time, figmotid.y, iid);
				gl_Position = vec4(pos / patternSizeOnRetina * 2, 0, 1);
				pos = qsize * vec2(-1, 1);
				EmitVertex();
				pos = qpos + motionTransform(qsize * vec2(1, -1), time, figmotid.y, iid);
				gl_Position = vec4(pos / patternSizeOnRetina * 2, 0, 1);
				pos = qsize * vec2(1, -1);
				EmitVertex();
				pos = qpos + motionTransform(qsize * vec2(-1, -1), time, figmotid.y, iid);
				gl_Position = vec4(pos / patternSizeOnRetina * 2, 0, 1);
				pos = qsize * vec2(-1, -1);
				EmitVertex();
				EndPrimitive();
			}
			)GLSLCODE";
		return s;
	}
	else
		return "";
}
