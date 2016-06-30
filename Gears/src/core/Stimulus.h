#pragma once

#include <algorithm>
#include <string>
#include <map>
#include "math/math.h"
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <iomanip>
#include <list>
#include <set>
#include <vector>
#include "Pass.h"
#include "Event/Base.h"

class Sequence;
class SpatialFilter;

//! A structure that contains all stimulus parameters.
class Stimulus
{

public:
	std::string name;							//< Unique name.
	std::string brief;							//< A short discription of the stimulus.

	boost::shared_ptr<Sequence> sequence;		//< Part of this sequence.

	std::vector< Pass::P > passes;
	void addPass(Pass::P pass);

	boost::python::object joiner;
	boost::python::object setJoiner(boost::python::object joiner);

	unsigned int duration;						//< Stimulus duration [frames].
	unsigned int startingFrame;					//< Stimulus starts at this time in sequence [frames].

	struct SignalEvent
	{
		bool clear;
		std::string channel;
	};
	using SignalMap = std::multimap<unsigned int, SignalEvent >;
	SignalMap tickSignals;
	std::set<std::string> stimulusChannels;

	bool usesForwardRendering;
	boost::python::object forwardRenderingCallback;
	boost::python::object setForwardRenderingCallback(boost::python::object cb);

	std::string spikeVertexShaderSource;			//< Renders timeline representation.
	std::string spikeFragmentShaderSource;

	std::string temporalFilterPlotVertexShaderSource;			//< Renders timeline representation.
	std::string temporalFilterPlotFragmentShaderSource;

	std::string temporalFilterShaderSource;
	std::string temporalFilterFuncSource;
	float temporalWeights[64];
	uint temporalMemoryLength;
	bool mono;
	uint temporalProcessingStateCount;
	bool fullScreenTemporalFiltering;
	float temporalWeightMax;
	float temporalWeightMin;

	Gears::Math::float4x4 temporalProcessingStateTransitionMatrix[4];

	boost::shared_ptr<SpatialFilter> spatialFilter;

	std::string randomGeneratorShaderSource;	//< Prng.
	uint	randomGridWidth;					//<	Number of cells per row in 2D random grid array for random number generation.
	uint	randomGridHeight;					//<	Number of cells per column in of 2D random grid array for random number generation.
	uint	randomSeed;							//< Initial number for random number generation. The same seed always produces the same randoms.
	uint	freezeRandomsAfterFrame;

	std::string particleShaderSource;			//< Particle system.
	uint	particleGridWidth;					//<	Number of cells per row in 2D grid array.
	uint	particleGridHeight;					//<	Number of cells per column in of 2D grid array.

	// tone dynamics
	float	gamma[101];							//< The 'gamma correction' curve of the display device, from 0 to one in 0.01 resolution
	int		gammaSamplesCount;					//< The number of meaningful elements is the gamma array. Cannot be higher than 101.

	bool	doesToneMappingInStimulusGenerator;
	bool	erfToneMapping;
	float	dynamicRangeMin;					//< The output stimulus value mapped to 0 on the display.
	float	dynamicRangeMax;					//< The output stimulus value mapped to 1 on the display.
	float	dynamicRangeMean;
	float	dynamicRangeVar;

	float measuredDynRangeMin;
	float measuredDynRangeMax;
	float measuredMean;
	float measuredVariance;

	void overrideTickSignals();
	void raiseSignalOnTick(uint iTick, std::string channel);
	void clearSignalOnTick(uint iTick, std::string channel);

	//! Constructor.
	Stimulus();
public:
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(Stimulus);

	//! Destructor. Releases dynamically allocated memory.
	~Stimulus();

	void setSequence(boost::shared_ptr<Sequence> sequence)
	{
		this->sequence = sequence;
	}

	unsigned int setStartingFrame(unsigned int offset) { this->startingFrame = offset; return duration; }

	void saveConfig(const std::string& expName);

	void setDuration(unsigned int duration) { this->duration = duration;}
	uint getDuration() const {return duration;} 

	std::string getRandomGeneratorShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		s += "uniform vec2 patternSizeOnRetina;\n";
		s += "uniform bool swizzleForFft;\n";
		s += "uniform int frame;\n";
		s += "uniform float time;\n";

		//for(auto& svar : shaderColors)
		//{
		//	s += "uniform vec3 ";
		//	s += svar.first;
		//	s += ";\n";
		//}
		//for(auto& svar : shaderVectors)
		//{
		//	s += "uniform vec2 ";
		//	s += svar.first;
		//	s += ";\n";
		//}
		//for(auto& svar : shaderVariables)
		//{
		//	s += "uniform float ";
		//	s += svar.first;
		//	s += ";\n";
		//}
		return s + randomGeneratorShaderSource;
	}

	std::string getParticleShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		s += "uniform vec2 patternSizeOnRetina;\n";
		s += "uniform int frame;\n";
		s += "uniform float time;\n";

		return s + particleShaderSource;
	}

	std::string getTemporalFilterShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		return s + 
					"	uniform sampler1D gamma;																	\n"
					+
			temporalFilterFuncSource + temporalFilterShaderSource;
	}

	std::string getTemporalFilterPlotVertexShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		return s +
					"	uniform sampler1D gamma;																	\n"
			+ temporalFilterFuncSource + temporalFilterPlotVertexShaderSource;
	}

	std::string getTemporalFilterPlotFragmentShaderSource() const
	{
		std::string s("#version 150 compatibility\n");
		return s + temporalFilterPlotFragmentShaderSource;
	}

	void setSpatialFilter(boost::shared_ptr<SpatialFilter> spatialFilter)
	{
		this->spatialFilter = spatialFilter;
		this->fullScreenTemporalFiltering = true;
		this->doesToneMappingInStimulusGenerator = false;
		if(this->temporalMemoryLength == 0 && this->temporalProcessingStateCount == 0)
		{
			this->temporalMemoryLength = 1;
			this->temporalWeights[63] = 1;
			for(int i=0; i<63; i++)
				temporalWeights[i] = 0;
			temporalWeightMin = 1;
			temporalWeightMax = 1;
		}
	}

	boost::shared_ptr<const SpatialFilter> getSpatialFilter() const
	{
		return spatialFilter;
	}

	float getSpatialPlotMin();
	float getSpatialPlotMax();
	float getSpatialPlotWidth ();
	float getSpatialPlotHeight ();

	uint getStartingFrame() const
	{
		return startingFrame;
	}

	//boost::python::object set(boost::python::object settings);

	boost::python::object setGamma(boost::python::object gammaList, bool invert=false);
	boost::python::object setTemporalWeights(boost::python::object twList, bool fullScreen);
	boost::python::object setTemporalWeightingFunction(std::string func, int memoryLength, bool fullscreen, float minPlot, float maxPlot);
	boost::python::object setLtiMatrix(boost::python::object mList);

	std::map<uint, std::vector<boost::python::object> > callbacks;

	void registerCallback(uint msg, boost::python::object callback);
	template<typename T>
	void executeCallbacks(typename T::P wevent) const
	{
		auto ic = callbacks.find(typename T::typeId);
		if(ic != callbacks.end())
			for(auto& cb : ic->second)
				cb(wevent);
	}

	//boost::python::object startCallback;
	//boost::python::object frameCallback;
	//boost::python::object finishCallback;
	//boost::python::object onStart(boost::python::object callback);
	//boost::python::object onFrame(boost::python::object callback);
	//boost::python::object onFinish(boost::python::object callback);

	bool hasSpatialFiltering() const
	{
		return spatialFilter != nullptr;
	}

	bool hasTemporalFiltering() const
	{
		return temporalMemoryLength > 1;
	}

	boost::shared_ptr<Sequence> getSequence() {return sequence;}

	float getDuration_s() const;

	boost::python::object pythonObject;
	boost::python::object setPythonObject(boost::python::object o);
	boost::python::object getPythonObject();

	bool usesChannel(std::string channel) { return stimulusChannels.count(channel) == 1;}
	uint getChannelCount() { return stimulusChannels.size();}


	void setMeasuredDynamics (
			float measuredDynRangeMin,
			float measuredDynRangeMax,
			float measuredMean,
			float measuredVariance
		)const;

	void setToneMappingLinear(float min, float max) const ;
	void setToneMappingErf	 (float mean, float var)const ;

	const SignalMap& getSignals() const{ return tickSignals;}

	void enableColorMode(){mono = false;}
	const std::vector< Pass::P >& getPasses() const {return passes;}
};
