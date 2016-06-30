#include "stdafx.h"
#include "PythonDict.h"

#include "Stimulus.h"
#include "Sequence.h"
#include "SpatialFilter.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <limits>

Stimulus::Stimulus():
	name("N/A"),
	sequence(nullptr),
	brief("<no description>"),
	duration(1),
	startingFrame(0),
	spatialFilter(nullptr),
	randomGridWidth(0),
	randomGridHeight(0),
	randomSeed(0),
	freezeRandomsAfterFrame(0),
	particleGridWidth(0),
	particleGridHeight(0),
	gammaSamplesCount(2),
	dynamicRangeMin(0),
	dynamicRangeMax(1),
	dynamicRangeMean(0.5),
	dynamicRangeVar(-0.25),
	erfToneMapping(false),
	usesForwardRendering(false),
	fullScreenTemporalFiltering(false),
	mono(true),
	doesToneMappingInStimulusGenerator(true)
{
	measuredDynRangeMin = std::numeric_limits<double>::quiet_NaN();
	measuredDynRangeMax = std::numeric_limits<double>::quiet_NaN();
	measuredMean  = std::numeric_limits<double>::quiet_NaN();
	measuredVariance  = std::numeric_limits<double>::quiet_NaN();

	gamma[0] = 0.0f;
	gamma[1] = 1.0f;

	for(unsigned int i=0; i<63; i++)
		temporalWeights[i] = 0.0f;
	temporalWeights[63] = 1.0f;
	temporalMemoryLength = 0;
	temporalProcessingStateCount = 0;
	temporalWeightMax = 1.0f;
	temporalWeightMin = 0.0f;

	randomGeneratorShaderSource =
		"	out uvec4 nextElement;																					\n"
		"	void main() { nextElement = uvec4(0u, 0u, 0u, 0u); }													\n"
		;

	temporalFilterFuncSource =
		"float temporalWeight(int i) { if(i==1) return 1.0; else return 0.0; } \n"
		;

	temporalFilterShaderSource = 
		"	uniform sampler2DArray stimulusQueue;																	\n"
		"	uniform int currentSlice;																				\n"
		"	uniform int memoryLength;																				\n"
		"	uniform int queueLength;																				\n"
		"	uniform float dynamicRangeMin;																			\n"
		"	uniform float dynamicRangeMax;																			\n"
		"	uniform float dynamicRangeMean;																			\n"
		"	uniform float dynamicRangeVar;																			\n"
		"	uniform int gammaSampleCount;																			\n"
		"	uniform bool calibrating;																				\n"
		"	in vec2 fTexCoord;																						\n"
		"	out vec4 outcolor;																						\n"
		"	void main() {																							\n"
		"		vec4 a = vec4(0, 0, 0, 0);																			\n"
		"		for(int i=1; i<=memoryLength; i++)	{															\n"
//		"			a.xyz += texture(stimulusQueue, vec3(fTexCoord, (currentSlice+i+1)%queueLength)).xyz * texture(gamma, (i + 128.5) / 256.0).x;		\n"
		"           float w = temporalWeight(i);																	\n"
		"			a.xyz += texture(stimulusQueue, vec3(fTexCoord, (currentSlice+queueLength-i)%queueLength)).xyz * w;	}	\n"
		"		outcolor = a;																						\n"
		"		if(dynamicRangeVar >= 0)																			\n"
		"		{																									\n"
		"			outcolor.r = 1 - 1 / (1 + exp((outcolor.r - dynamicRangeMean)/dynamicRangeVar));					\n"
		"			outcolor.g = 1 - 1 / (1 + exp((outcolor.g - dynamicRangeMean)/dynamicRangeVar));					\n"
		"			outcolor.b = 1 - 1 / (1 + exp((outcolor.b - dynamicRangeMean)/dynamicRangeVar));					\n"
		"			outcolor.w = 1 - 1 / (1 + exp((outcolor.w - dynamicRangeMean)/dynamicRangeVar));					\n"
		"		}																									\n"
		"		else																								\n"
		"		{																									\n"
		"			outcolor.r = (outcolor.r - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"			outcolor.g = (outcolor.g - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"			outcolor.b = (outcolor.b - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
		"			outcolor.w = (outcolor.w - dynamicRangeMin) / (dynamicRangeMax - dynamicRangeMin);				\n"
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
		"	}																										\n"
		;

		temporalFilterPlotVertexShaderSource = 
		"	void main(void) {		 \n"
		"		float value = temporalWeight(gl_VertexID/2+1);											\n"
		"		gl_Position	= gl_ModelViewMatrix * vec4(float((gl_VertexID+1)/2), value, 0.5, 1.0);				\n"
		"	}																										\n"
		;
		temporalFilterPlotFragmentShaderSource = 
		"	void main() {																							\n"
		"		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);															\n"
		"	}																										\n"
		;

		spikeVertexShaderSource = 
		"	uniform float frameInterval;	\n"
		"	uniform int stride;	\n"
		"	uniform int startOffset;	\n"
		"	void main(void) {		 \n"
		"		gl_Position	= gl_ModelViewMatrix * vec4(float(gl_VertexID/3 * stride + startOffset), ((gl_VertexID+2)%3!=0)?0:1, 0.5, 1.0);   \n"
		"	}																										\n"
		;

		spikeFragmentShaderSource = 
		"	void main() {																							\n"
		"		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);															\n"
		"	}																										\n"
		;
}

Stimulus::~Stimulus()
{
}

boost::python::object Stimulus::setGamma(boost::python::object gammaList, bool invert)
{
	using namespace boost::python;
	list l = extract<list>(gammaList);
	gammaSamplesCount = len(l);
	if(gammaSamplesCount > 101)
	{
		gammaSamplesCount = 101;
		PyErr_WarnEx(PyExc_UserWarning, "Only the first 101 gamma samples are used!", 2);
	}
	for(int i=0; i<gammaSamplesCount; i++)
	{
	    gamma[i] = extract<float>(l[i]);
	}

	if(invert)
	{
		gamma[0] = 0;
		float fiGamma[101];
		int g=0;
		for(int ifi=1; ifi<gammaSamplesCount-1; ifi++)
		{
			float f = (float)ifi / (gammaSamplesCount-1);
			while( gamma[g] < f && g < gammaSamplesCount) g++;
			fiGamma[ifi] = ((g - 1) + (f - gamma[g-1]) / (gamma[g] - gamma[g-1])) / (gammaSamplesCount-1);
		}
		fiGamma[0] = 0;
		fiGamma[gammaSamplesCount-1] = 1;
		for(int u=0; u<gammaSamplesCount; u++)
			gamma[u] = fiGamma[u];
	}

	return object();
}

boost::python::object Stimulus::setTemporalWeights(boost::python::object twList, bool fullScreen)
{
	fullScreenTemporalFiltering = fullScreen;
	using namespace boost::python;
	list l = extract<list>(twList);
	uint length = len(l);
	if(length > 1)
		doesToneMappingInStimulusGenerator = false;
	if(length > 64)
	{
		PyErr_Warn(PyExc_Warning, "Temporal weights must be at most 64 values. Not setting temporal filtering weights!");
	}
	else
	{
		int u=0;
		temporalWeightMax = -FLT_MAX;
		temporalWeightMin = FLT_MAX;
		for(int i=64-length; i<64; i++, u++)
		{
			temporalWeights[i] = extract<float>(l[u]);
			temporalWeightMin = std::min(temporalWeightMin, temporalWeights[i]);
			temporalWeightMax = std::max(temporalWeightMax, temporalWeights[i]);
		}
		for(int i=0; i<64-length; i++)
			temporalWeights[i] = 0;
		temporalMemoryLength = length;
	}
	return object();
}

//boost::python::object Stimulus::set(boost::python::object settings)
//{
//	using namespace boost::python;
//	dict d = extract<dict>(settings);
//	Gears::PythonDict pd(d);
//	pd.process( [&](std::string key) {
//		if(		key == "duration"			)	duration = pd.getFloat(key);
//		else if(key == "shaderVariables"	) {
//			pd.forEach(key, [&](object element) {
//				dict dd = extract<dict>(element);
//				Gears::PythonDict funcDict(dd);
//				setShaderVariable( funcDict.getString("name"), funcDict.getFloat("value"));
//			});
//		}
//		else if(key == "shaderColors"	) {
//			pd.forEach(key, [&](object element) {
//				dict dd = extract<dict>(element);
//				Gears::PythonDict funcDict(dd);
//				Gears::Math::float3 v = funcDict.getFloat3("value");
//				setShaderColor( funcDict.getString("name"), -2, v.x, v.y, v.z);
//			});
//		}
//		else if(key == "shaderFunctions"	) {
//			pd.forEach(key, [&](object element) {
//				dict dd = extract<dict>(element);
//				Gears::PythonDict funcDict(dd);
//				setShaderFunction( funcDict.getString("name"), funcDict.getString("src"));
//			});
//		}
//		else if(key == "stimulusGeneratorShaderSource"	)  stimulusGeneratorShaderSource = pd.getString(key);
//	});
//	return object();
//}

float Stimulus::getSpatialPlotMin()
{
	if(spatialFilter)
		return spatialFilter->minimum;
	return 0;
}

float Stimulus::getSpatialPlotMax()
{
	if(spatialFilter)
		return spatialFilter->maximum;
	return 1;
}

float Stimulus::getSpatialPlotWidth ()
{
	if(spatialFilter)
		return spatialFilter->width_um;
	return 200.0;
}

float Stimulus::getSpatialPlotHeight ()
{
	if(spatialFilter)
		return spatialFilter->height_um;
	return 200.0;
}

void Stimulus::raiseSignalOnTick(uint iTick, std::string channel)
{
	SignalEvent e;
	e.clear = false;
	e.channel = channel;
	tickSignals.insert( std::pair<unsigned int, SignalEvent>(iTick, e) );
	stimulusChannels.insert(channel);
}

void Stimulus::clearSignalOnTick(uint iTick, std::string channel)
{
	SignalEvent e;
	e.clear = true;
	e.channel = channel;
	tickSignals.insert( std::pair<unsigned int, SignalEvent>(iTick, e) );
	stimulusChannels.insert(channel);
}

void Stimulus::overrideTickSignals()
{
	tickSignals.clear();
}

//boost::python::object Stimulus::onStart(boost::python::object callback)
//{
//	startCallback = callback;
//	return boost::python::object() ;
//}
//
//boost::python::object Stimulus::onFrame(boost::python::object callback)
//{
//	frameCallback = callback;
//	return boost::python::object() ;
//}
//
//boost::python::object Stimulus::onFinish(boost::python::object callback)
//{
//	finishCallback = callback;
//	return boost::python::object() ;
//}

boost::python::object Stimulus::setJoiner(boost::python::object joiner)
{
	this->joiner = joiner;
	return boost::python::object();
}


float Stimulus::getDuration_s() const
{
	return duration * sequence->getFrameInterval_s();
}

boost::python::object Stimulus::setPythonObject(boost::python::object o)
{
	pythonObject = o;
	return boost::python::object();
}

boost::python::object Stimulus::getPythonObject()
{
	return pythonObject;
}

void Stimulus::setMeasuredDynamics  (
			float measuredDynRangeMin,
			float measuredDynRangeMax,
			float measuredMean,
			float measuredVariance
		) const
{
	const_cast<Stimulus*>(this)->measuredDynRangeMin	= measuredDynRangeMin;
	const_cast<Stimulus*>(this)->measuredDynRangeMax	= measuredDynRangeMax;
	const_cast<Stimulus*>(this)->measuredMean			= measuredMean;
	const_cast<Stimulus*>(this)->measuredVariance		= measuredVariance;
}

void Stimulus::setToneMappingLinear(float min, float max) const
{
	const_cast<Stimulus*>(this)->erfToneMapping = false;
	const_cast<Stimulus*>(this)->dynamicRangeMin	= min;
	const_cast<Stimulus*>(this)->dynamicRangeMax	= max;
}

void Stimulus::setToneMappingErf	 (float mean, float var) const
{
	const_cast<Stimulus*>(this)->erfToneMapping = true;
	const_cast<Stimulus*>(this)->dynamicRangeMean	= mean;
	const_cast<Stimulus*>(this)->dynamicRangeVar	= var;
}

boost::python::object Stimulus::setForwardRenderingCallback(boost::python::object cb)
{
	usesForwardRendering = true;
	this->forwardRenderingCallback = cb;
	return boost::python::object();
}

boost::python::object Stimulus::setTemporalWeightingFunction(std::string func, int memoryLength, bool fullscreen, float minPlot, float maxPlot)
{
	if(memoryLength > 1)
		doesToneMappingInStimulusGenerator = false;
	fullScreenTemporalFiltering = fullscreen;
	temporalMemoryLength = memoryLength;
	temporalFilterFuncSource = func;
	temporalWeightMin = minPlot;
	temporalWeightMax = maxPlot;
	return boost::python::object();
}

void Stimulus::addPass(Pass::P pass)
{
	pass->setStimulus(getSharedPtr());
	pass->joiner();
	passes.push_back(pass);
}

void Stimulus::registerCallback(uint msg, boost::python::object callback)
{
	callbacks[msg].push_back(callback);
}


boost::python::object Stimulus::setLtiMatrix(boost::python::object mList)
{
	fullScreenTemporalFiltering = true;
	using namespace boost::python;
	list l = extract<list>(mList);
	uint length = len(l);
	if(length > 1)
		doesToneMappingInStimulusGenerator = false;
	 
	temporalProcessingStateTransitionMatrix[0] = Gears::Math::float4x4(
		 extract<float>(l[0]),   extract<float>(l[1]),   extract<float>(l[2]),   extract<float>(l[3]),
		 extract<float>(l[4]),   extract<float>(l[5]),   extract<float>(l[6]),   extract<float>(l[7]),
		 extract<float>(l[8]),   extract<float>(l[9]),   extract<float>(l[10]),  extract<float>(l[11]),
		 extract<float>(l[12]),  extract<float>(l[13]),  extract<float>(l[14]),  extract<float>(l[15]))
		;
	temporalProcessingStateCount = 4;
	return boost::python::object();
}