// Gears.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include "PythonDict.h"

#include <string>

#include <GL/glew.h>
#	ifdef _WIN32
#include "wglext.h"
#elif __linux__
#	include <GL/glx.h>
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>

#include "Sequence.h"
#include "SequenceRenderer.h"
#include "Ticker.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "KernelManager.h"
#include "StimulusWindow.h"
#include "event/events.h"
#include "Response.h"

Sequence::P				sequence = nullptr;
SequenceRenderer::P		sequenceRenderer = nullptr;
ShaderManager::P		shaderManager = nullptr;
TextureManager::P		textureManager = nullptr;
KernelManager::P		kernelManager = nullptr;
StimulusWindow::P		stimulusWindow = nullptr;

std::string createStimulusWindow()
{
#ifdef _WIN32
	StimulusWindow::registerClass();
#endif
	stimulusWindow = StimulusWindow::create();
	stimulusWindow->createWindow(false, 256, 256);

	sequenceRenderer = SequenceRenderer::create();
	stimulusWindow->setSequenceRenderer (sequenceRenderer);

	shaderManager  = ShaderManager::create();
	textureManager = TextureManager::create();
	kernelManager = KernelManager::create(sequenceRenderer, shaderManager);
	return stimulusWindow->getSpecs();
}

void onHideStimulusWindow(boost::python::object onHideCallback)
{
	if(stimulusWindow)
	{
		stimulusWindow->onHide(onHideCallback);
	}
}

void showText(){
	if(sequence == nullptr)
		return;
	sequenceRenderer->showText();
}

void setText(std::string tag, std::string text){
	if(sequence == nullptr)
		return;
	sequenceRenderer->setText(tag, text);
}

void drawSequenceTimeline(int x, int y, int w, int h){
	if(sequence == nullptr)
		return;
	glViewport(x, y, w, h);
	sequenceRenderer->renderTimeline();
}

void drawStimulusTimeline(int x, int y, int w, int h){
	if(sequence == nullptr)
		return;
	glViewport(x, y, w, h);
	sequenceRenderer->renderSelectedStimulusTimeline();
}

void drawSpatialKernel(float min, float max, float width, float height){
	if(sequence == nullptr)
		return;
	sequenceRenderer->renderSelectedStimulusSpatialKernel(min, max, width, height);
}

void drawTemporalKernel(){
	if(sequence == nullptr)
		return;
	sequenceRenderer->renderSelectedStimulusTemporalKernel();
}

void toggleChannelsOrPreview(){
	if(sequence == nullptr)
		return;
	sequenceRenderer->toggleChannelsOrPreview();
}


void drawSpatialProfile(float min, float max, float width, float height){
	if(sequence == nullptr)
		return;
	sequenceRenderer->renderSelectedStimulusSpatialProfile(min, max, width, height);
}

void updateSpatialKernel(){
	if(sequence == nullptr)
		return;
	sequenceRenderer->updateSpatialKernel(kernelManager);
}

double getTime(){
	if(sequence == nullptr)
		return 0.0;
	return sequenceRenderer->getTime();
}

Sequence::P createSequence(std::string name)
{
	using namespace boost::python;
	Sequence::P sequence = Sequence::create(name);
	::sequence = sequence;
	return sequence;
}

Sequence::P setSequence(Sequence::P sequence)
{
	using namespace boost::python;
	::sequence = sequence;
	textureManager->clear();
	shaderManager->clear();
	kernelManager->clear();
	sequenceRenderer->apply(sequence, shaderManager, textureManager, kernelManager);
	return sequence;
}

Sequence::P getSequence()
{
	using namespace boost::python;
	return sequence;
}

void pickStimulus(double x, double y)
{
	if(sequenceRenderer == nullptr)
		return;
	sequenceRenderer->pickStimulus(x, y);
}

Stimulus::CP getSelectedStimulus()
{
	return sequenceRenderer->getSelectedStimulus();
}

void reset()
{
	sequenceRenderer->reset();
}

void skip(int skipCount)
{
	sequenceRenderer->skip(skipCount);
}

Stimulus::CP getCurrentStimulus()
{
	return sequenceRenderer->getCurrentStimulus();
}

void cleanup()
{
	sequenceRenderer->cleanup();
//	textureManager->clear();
//	shaderManager->clear();
//	kernelManager->clear();
}

void setMousePointerLocation(float x, float y)
{
	#ifdef _WIN32
		uint screenw = GetSystemMetrics(SM_CXSCREEN);
		uint screenh = GetSystemMetrics(SM_CYSCREEN);
		SetCursorPos((int)(screenw * x), (int)(screenh * y));
	#elif __linux__
		if(stimulusWindow)
			stimulusWindow->setCursorPos();
	#endif
}

void instantlyRaiseSignal(std::string c)
{
	sequenceRenderer->raiseSignal(c);
}

void instantlyClearSignal(std::string c)
{
	sequenceRenderer->clearSignal(c);
}

int getSkippedFrameCount()
{
	return sequenceRenderer->getSkippedFrameCount();
}

std::string getSequenceTimingReport()
{
	return sequenceRenderer->getSequenceTimingReport();
}

Ticker::P startTicker()
{
	return sequenceRenderer->startTicker();
}

void enableExport(std::string path)
{
	sequenceRenderer->enableExport(path);
}
void enableVideoExport(const char * path, int fr, int w, int h)
{
	sequenceRenderer->enableVideoExport(path, fr, w, h);
}

void enableCalibration(uint startingFrame, uint duration, float histogramMin, float histogramMax)
{
	sequenceRenderer->enableCalibration(startingFrame, duration, histogramMin, histogramMax);
}

void setSequenceTimelineZoom(uint nFrames)
{
	sequenceRenderer->setSequenceTimelineZoom(nFrames);
}

void setSequenceTimelineStart(uint iStartFrame)
{
	sequenceRenderer->setSequenceTimelineStart(iStartFrame);
}

void setStimulusTimelineZoom(uint nFrames)
{
	sequenceRenderer->setStimulusTimelineZoom(nFrames);
}

void setStimulusTimelineStart(uint iStartFrame)
{
	sequenceRenderer->setStimulusTimelineStart(iStartFrame);
}

void pauseRender()
{
	sequenceRenderer->pause();
}

void setResponded()
{
	sequenceRenderer->setResponded();
}


boost::python::object renderSample(uint iFrame, uint x, uint y, uint w, uint h)
{
	return sequenceRenderer->renderSample(iFrame, x, y, w, h);
}

void makePath(std::string path)
{
	boost::filesystem::path bpath(path);
	if( !boost::filesystem::exists( bpath.parent_path() ))
		boost::filesystem::create_directories( bpath.parent_path() );
}

// A couple of simple C++ functions that we want to expose to Python.
std::string greet() { return "hello, world"; }
int square(int number) { return number * number; }


std::string getSpecs()
{
	if(!stimulusWindow)
		return "N/A";
	return stimulusWindow->getSpecs();
}

void makeCurrent()
{
	if(!stimulusWindow)
		return;
	stimulusWindow->makeCurrent();
}

#ifdef _WIN32
void shareCurrent()
{
	if(!stimulusWindow)
		return;
	stimulusWindow->shareCurrent();
}
#elif __linux__
void shareCurrent(unsigned int winId)
{
	if (!stimulusWindow)
		return;
	stimulusWindow->shareCurrent(winId);
}
#endif

void run()
{
	if(!stimulusWindow)
		return;
	stimulusWindow->run();
}

void loadTexture(std::string filename)
{
	if(textureManager)
		textureManager->loadTexture(filename);
}

void bindTexture(std::string filename)
{
	if(textureManager)
	{
		Texture2D* tex = textureManager->loadTexture(filename);
		glBindTexture(GL_TEXTURE_2D, tex->getTextureHandle());
	}
}

namespace python = boost::python;

// Python requires an exported function called init<module-name> in every
// extension module. This is where we build the module contents.

using namespace boost::python;


BOOST_PYTHON_MODULE(Gears)
{

    // Add regular functions to the module.
    def("greet", greet);
    def("square", square);
	//class_<Gears::Event::Base>("BaseEvent", no_init)
	//	.def_readonly(	"message"	, &Gears::Event::Base::message	, "Windows message.")
	//	.def_readonly(	"wParam"	, &Gears::Event::Base::wParam	, "Windows message wParam.")
	//	.def_readonly(	"lParam"	, &Gears::Event::Base::lParam	, "Windows message lParam.")
	//	;
	//register_ptr_to_python<Gears::Event::Base::P>();

	class_<Gears::Event::MouseMove>("MouseMoveEvent", no_init)
		.def( "globalX", &Gears::Event::MouseMove::globalX)
		.def( "globalY", &Gears::Event::MouseMove::globalY)
		.def( "globalPercentX", &Gears::Event::MouseMove::globalPercentX)
		.def( "globalPercentY", &Gears::Event::MouseMove::globalPercentY)
		.def_readonly(	"typeId"	, &Gears::Event::MouseMove::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MouseMove::P>();

	class_<Gears::Event::MousePressedLeft>("MousePressedLeftEvent", no_init)
		.def( "globalX", &Gears::Event::MousePressedLeft::globalX)
		.def( "globalY", &Gears::Event::MousePressedLeft::globalY)
		.def_readonly(	"typeId"	, &Gears::Event::MousePressedLeft::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MousePressedLeft::P>();

	class_<Gears::Event::MouseReleasedLeft>("MouseReleasedLeftEvent", no_init)
		.def( "globalX", &Gears::Event::MouseReleasedLeft::globalX)
		.def( "globalY", &Gears::Event::MouseReleasedLeft::globalY)
		.def("globalPercentX", &Gears::Event::MouseReleasedLeft::globalPercentX)
		.def("globalPercentY", &Gears::Event::MouseReleasedLeft::globalPercentY)
		.def_readonly(	"typeId"	, &Gears::Event::MouseReleasedLeft::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MouseReleasedLeft::P>();

	class_<Gears::Event::MousePressedMiddle>("MousePressedMiddleEvent", no_init)
		.def( "globalX", &Gears::Event::MousePressedMiddle::globalX)
		.def( "globalY", &Gears::Event::MousePressedMiddle::globalY)
		.def_readonly(	"typeId"	, &Gears::Event::MousePressedMiddle::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MousePressedMiddle::P>();

	class_<Gears::Event::MouseReleasedMiddle>("MousePressedMiddleEvent", no_init)
		.def( "globalX", &Gears::Event::MouseReleasedMiddle::globalX)
		.def( "globalY", &Gears::Event::MouseReleasedMiddle::globalY)
		.def_readonly(	"typeId"	, &Gears::Event::MouseReleasedMiddle::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MouseReleasedMiddle::P>();

	class_<Gears::Event::MousePressedRight>("MousePressedRightEvent", no_init)
		.def( "globalX", &Gears::Event::MousePressedRight::globalX)
		.def( "globalY", &Gears::Event::MousePressedRight::globalY)
		.def_readonly(	"typeId"	, &Gears::Event::MousePressedRight::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MousePressedRight::P>();

	class_<Gears::Event::MouseReleasedRight>("MousePressedRightEvent", no_init)
		.def( "globalX", &Gears::Event::MouseReleasedRight::globalX)
		.def( "globalY", &Gears::Event::MouseReleasedRight::globalY)
		.def_readonly(	"typeId"	, &Gears::Event::MouseReleasedRight::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::MouseReleasedRight::P>();

	class_<Gears::Event::Wheel>("WheelEvent", no_init)
		.def( "deltaX", &Gears::Event::Wheel::deltaX)
		.def( "deltaY", &Gears::Event::Wheel::deltaY)
		.def_readonly(	"typeId"	, &Gears::Event::Wheel::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::Wheel::P>();

	class_<Gears::Event::KeyPressed>("KeyPressedEvent", no_init)
		.def( "text", &Gears::Event::KeyPressed::text)
		.def( "key", &Gears::Event::KeyPressed::key)
		.def_readonly(	"typeId"	, &Gears::Event::KeyPressed::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::KeyPressed::P>();

	class_<Gears::Event::KeyReleased>("KeyReleasedEvent", no_init)
		.def( "text", &Gears::Event::KeyReleased::text)
		.def( "key", &Gears::Event::KeyReleased::key)
		.def_readonly(	"typeId"	, &Gears::Event::KeyReleased::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::KeyReleased::P>();

	class_<Gears::Event::Frame>("FrameEvent", no_init)
		.def_readonly(	"index"	, &Gears::Event::Frame::iFrame, "Frame index.")
		.def_readonly(	"time"	, &Gears::Event::Frame::time, "Frame time.")
		.def_readonly(	"typeId"	, &Gears::Event::Frame::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::Frame::P>();

	class_<Gears::Event::StimulusStart>("StimulusStartEvent", no_init)
		.def_readonly(	"typeId"	, &Gears::Event::StimulusStart::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::StimulusStart::P>();

	class_<Gears::Event::StimulusEnd>("StimulusEndEvent", no_init)
		.def_readonly(	"typeId"	, &Gears::Event::StimulusEnd::typeId, "Message ID.")
		;
	register_ptr_to_python<Gears::Event::StimulusEnd::P>();

//    def("onDisplay", onDisplay);
//    def("onDisplayHidden", onDisplayHidden);
	def("drawSequenceTimeline", drawSequenceTimeline);
	def("drawStimulusTimeline", drawStimulusTimeline);
	def("drawTemporalKernel", drawTemporalKernel);
	def("drawSpatialKernel", drawSpatialKernel);
	def("drawSpatialProfile", drawSpatialProfile);
	class_<SpatialFilter>("SpatialFilter", no_init)
		.def( "__init__", make_constructor( &SpatialFilter::create<>) )
		.def( "setShaderFunction"	, &SpatialFilter::setShaderFunction, ( arg("name"), arg("src")) )
		.def( "setShaderColor"		, &SpatialFilter::setShaderColor, ( arg("name"), arg("all")=-2, arg("red")=0, arg("green")=0, arg("blue")=0) )
		.def( "setShaderVector"		, &SpatialFilter::setShaderVector, ( arg("name"), arg("x")=0, arg("y")=0 ) )
		.def( "setShaderVariable"	, &SpatialFilter::setShaderVariable, ( arg("name"), arg("value")) )
		.def("setSpatialDomainShader", &SpatialFilter::setSpatialDomainShader, "Set spatial domain filtering shader. Default is convolution. ")
		.def( "makeUnique"			, &SpatialFilter::makeUnique, "Given a unique ID to the filter. The filter will not be shared with other stimuli using the same filter with the same settings. This is useful for interactive filters.")
		.def_readwrite(	"minimum"	, &SpatialFilter::minimum	, "The minimum plotted value of the kernel function.")
		.def_readwrite(	"maximum"	, &SpatialFilter::maximum	, "The minimum plotted value of the kernel function.")
		.def_readwrite(	"width_um"	, &SpatialFilter::width_um	, "The horizontal extent of the spatial filter kernel [um].")
		.def_readwrite(	"height_um"	, &SpatialFilter::height_um	, "The vertical extent of the spatial filter kernel [um].")
		.def_readwrite(	"horizontalSampleCount"	, &SpatialFilter::horizontalSampleCount, "The number of samples used in spatial domain convolution.")
		.def_readwrite(	"verticalSampleCount"	, &SpatialFilter::verticalSampleCount, "The number of samples used in spatial domain convolution.")
		.def_readwrite(	"useFft"	, &SpatialFilter::useFft, "If True, convolution in computed in the frequency domain, if False, in the spatial domain. Use FFT for large kernels on powerful computers.")
		.def_readwrite(	"kernelGivenInFrequencyDomain"	, &SpatialFilter::kernelGivenInFrequencyDomain, "If True, the kernel function gives the kernel directly in the frquency domain.")
		.def_readwrite(	"showFft"						, &SpatialFilter::showFft						, "If true, the result of frequency domain spatial processing is not trnasformed back to the spatial domain.")
		.def_readwrite(	"stimulusGivenInFrequencyDomain", &SpatialFilter::stimulusGivenInFrequencyDomain, "If true, the stimulus is assumed to be already in frequency domain and not transformed before frequency domain processing.")
		;

//	class_<StimulusWindow>("StimulusWindow", no_init)
//		.def( "__init__", make_constructor( &Stimulus::create<>) )
//		.def( "makeCurrent", &StimulusWindow::makeCurrent, "Set the OpenGL context of the stimulus window as the current OpenGL context. " )
//		;

    class_< Sequence::StimulusMap >("StimulusMap")
        .def(map_indexing_suite< Sequence::StimulusMap, true >() );
    class_< Sequence::ChannelMap >("ChannelMap")
        .def(map_indexing_suite< Sequence::ChannelMap, true >() );

	class_<Sequence::Channel>("Channel")
		.def_readonly(	"portName"	, &Sequence::Channel::portName, "Name of the port the channel is associated to.")
		.def_readonly(	"raiseFunc"	, &Sequence::Channel::raiseFunc, "ID of the signal the channel is associated to.")
		;

	class_<Pass>("Pass", no_init)
		.def( "__init__", make_constructor( &Pass::create<>) )
		//.def( "set", &Stimulus::set)
		.def( "getDuration", &Pass::getDuration)
		.def( "getDuration_s", &Pass::getDuration_s)
		.def( "getStartingFrame", &Pass::getStartingFrame)
		.def( "getStimulus", &Pass::getStimulus)
		.def( "getSequence", &Pass::getSequence)
		.def_readwrite(	"name"							, &Pass::name							, "Stimulus name.")
		.def_readwrite(	"stimulusGeneratorShaderSource"	, &Pass::stimulusGeneratorShaderSource	, "Sets GLSL source for stimulus generator shader.")
		.def_readwrite(	"timelineVertexShaderSource"	, &Pass::timelineVertexShaderSource		, "Sets GLSL source for timeline shader.")
		.def_readwrite(	"timelineFragmentShaderSource"	, &Pass::timelineFragmentShaderSource	, "Sets GLSL source for timeline shader.")
		.def_readwrite(	"transparent"					, &Pass::transparent					, "If True, the alpha mask is used for blending. Otherwise intensities are just added.")
		.def( "setShaderImage"							, &Pass::setShaderImage, ( arg("name"), arg("file")) )
		.def( "setShaderFunction"						, &Pass::setShaderFunction, ( arg("name"), arg("src")) )
		.def( "setGeomShaderFunction"					, &Pass::setGeomShaderFunction, ( arg("name"), arg("src")) )
		.def( "setShaderVector"							, &Pass::setShaderVector, ( arg("name"), arg("x")=0, arg("y")=0 ) )
		.def( "setShaderColor"							, &Pass::setShaderColor, ( arg("name"), arg("all")=-2, arg("red")=0, arg("green")=0, arg("blue")=0) )
		.def( "setShaderVariable"						, &Pass::setShaderVariable, ( arg("name"), arg("value")) )
		.def("setJoiner"								, &Pass::setJoiner )
		.def("setVideo"									, &Pass::setVideo )
		.def("setPythonObject"							, &Pass::setPythonObject )
		.def("getPythonObject"							, &Pass::getPythonObject )
		.def("registerTemporalFunction"					, &Pass::registerTemporalFunction )
		.def("setPolygonMask"							, &Pass::setPolygonMask )
		.def("setMotionTransformFunction"				, &Pass::setMotionTransformFunction )
		.def("enableColorMode"							, &Pass::enableColorMode )
		.add_property(	"duration"						, &Pass::getDuration,		&Pass::setDuration 		, "Pass duration in frames.")
		;
	register_ptr_to_python<Pass::P>();

	class_<Response>("Response", no_init)
		.def("__init__", make_constructor(&Response::create<>))
		.def_readwrite("question", &Response::question)
		.def_readwrite("loop", &Response::loop)
		.def("setPythonObject", &Response::setPythonObject)
		.def("setJoiner", &Response::setJoiner)
		.def("registerCallback", &Response::registerCallback)
		.def("addButton", &Response::addButton)
		.def("getSequence", &Response::getSequence);
	register_ptr_to_python<Response::P>();

	class_<Stimulus>("Stimulus", no_init)
		.def( "__init__", make_constructor( &Stimulus::create<>) )
		//.def( "set", &Stimulus::set)
		.def("addPass", &Stimulus::addPass )
		.def( "getDuration", &Stimulus::getDuration)
		.def( "getDuration_s", &Stimulus::getDuration_s)
		.def( "getStartingFrame", &Stimulus::getStartingFrame)
		.def( "getSequence", &Stimulus::getSequence)
		.def_readwrite(	"name"							, &Stimulus::name							, "Stimulus name.")
		//.def_readwrite(	"stimulusGeneratorShaderSource"	, &Stimulus::stimulusGeneratorShaderSource	, "Sets GLSL source for stimulus generator shader.")
		//.def_readwrite(	"timelineVertexShaderSource"	, &Stimulus::timelineVertexShaderSource		, "Sets GLSL source for timeline shader.")
		//.def_readwrite(	"timelineFragmentShaderSource"	, &Stimulus::timelineFragmentShaderSource	, "Sets GLSL source for timeline shader.")
		.def_readwrite(	"randomGeneratorShaderSource"	, &Stimulus::randomGeneratorShaderSource	, "Sets GLSL source for random generator shader.")
		.def_readwrite(	"randomGridHeight"				, &Stimulus::randomGridHeight				, "Chessboard height [fields].")
		.def_readwrite(	"randomGridWidth"				, &Stimulus::randomGridWidth				, "Chessboard width [fields].")
		.def_readwrite(	"randomSeed"					, &Stimulus::randomSeed						, "Random seed.")
		.def_readwrite(	"freezeRandomsAfterFrame"		, &Stimulus::randomSeed						, "Stop generating new randoms after this many frames of the stimulus.")
		.def_readwrite(	"particleShaderSource"			, &Stimulus::particleShaderSource			, "Sets GLSL source for particle system shader.")
		.def_readwrite(	"particleGridHeight"			, &Stimulus::particleGridHeight				, "Particle grid height.")
		.def_readwrite(	"particleGridWidth"				, &Stimulus::particleGridWidth				, "Particle grid  width.")
		.def_readwrite(	"requiresClearing"				, &Stimulus::requiresClearing				, "If True, the screen is cleared before rendering the stimulus. Useful for multi-pass or instanced rendering. The default is False.")
		.def_readonly (	"measuredToneRangeVar"		, &Stimulus::measuredVariance				, "Measured variance.")
		.def_readonly (	"measuredToneRangeMean"		, &Stimulus::measuredMean					, "Measured mean.")
		.def_readonly (	"measuredToneRangeMin"		, &Stimulus::measuredToneRangeMin			, "Measured min.")
		.def_readonly (	"measuredToneRangeMax"		, &Stimulus::measuredToneRangeMax			, "Measured max.")
		.def_readwrite (	"toneRangeVar"		, &Stimulus::toneRangeVar				, "Scale applied in sigmoidal tone mapping.")
		.def_readwrite (	"toneRangeMean"		, &Stimulus::toneRangeMean				, "Offset applied in sigmoidal tone mapping.")
		.def_readwrite (	"toneRangeMin"		, &Stimulus::toneRangeMin			, "Linear tone mapping tone mapped to zero.")
		.def_readwrite (	"toneRangeMax"		, &Stimulus::toneRangeMax			, "Linear tone mapping tone mapped to one.")
		.def( "setMeasuredDynamics", &Stimulus::setMeasuredDynamicsFromPython )
		.def( "getMeasuredHistogramAsPythonList", &Stimulus::getMeasuredHistogramAsPythonList )
		.def( "setToneMappingErf", &Stimulus::setToneMappingErf )
		.def( "setToneMappingLinear", &Stimulus::setToneMappingLinear )
		.def( "setToneMappingEqualized", &Stimulus::setToneMappingEqualized )
		//.def( "setShaderImage", &Stimulus::setShaderImage, ( arg("name"), arg("file")) )
		//.def( "setShaderFunction", &Stimulus::setShaderFunction, ( arg("name"), arg("src")) )
		//.def( "setShaderVector"		, &Stimulus::setShaderVector, ( arg("name"), arg("x")=0, arg("y")=0 ) )
		//.def( "setShaderColor", &Stimulus::setShaderColor, ( arg("name"), arg("all")=-2, arg("red")=0, arg("green")=0, arg("blue")=0) )
		//.def( "setShaderVariable", &Stimulus::setShaderVariable, ( arg("name"), arg("value")) )
		.def( "setClearColor", &Stimulus::setClearColor, ( arg("all")=-2, arg("red")=0, arg("green")=0, arg("blue")=0) )
		.def( "setSpatialFilter", &Stimulus::setSpatialFilter )
		.def( "getSpatialFilter", &Stimulus::getSpatialFilter )
		.def("getSpatialPlotMin"		,	&Stimulus::getSpatialPlotMin ) 
		.def("getSpatialPlotMax"		,	&Stimulus::getSpatialPlotMax )
		.def("getSpatialPlotWidth"		,	&Stimulus::getSpatialPlotWidth )
		.def("getSpatialPlotHeight"		,	&Stimulus::getSpatialPlotHeight )
		.def("raiseSignalOnTick", &Stimulus::raiseSignalOnTick )
		.def("clearSignalOnTick", &Stimulus::clearSignalOnTick )
		.def("overrideTickSignals", &Stimulus::overrideTickSignals )
		.def("setTemporalWeights", &Stimulus::setTemporalWeights )
		.def("setTemporalWeightingFunction", &Stimulus::setTemporalWeightingFunction )
		.def("setLtiMatrix",	&Stimulus::setLtiMatrix )
		.def("setLtiImpulseResponse",	&Stimulus::setLtiImpulseResponse )
		.def("hasSpatialFiltering", &Stimulus::hasSpatialFiltering )
		.def("hasTemporalFiltering", &Stimulus::hasTemporalFiltering )
		.def("doesErfToneMapping", &Stimulus::doesErfToneMapping )
		//.def("onStart", &Stimulus::onStart )
		//.def("onFrame", &Stimulus::onFrame )
		//.def("onFinish", &Stimulus::onFinish )
		.def("registerCallback", &Stimulus::registerCallback )
		//.def("executeCallbacks", &Stimulus::executeCallbacks )
		.def("setJoiner", &Stimulus::setJoiner )
		.def("setForwardRenderingCallback", &Stimulus::setForwardRenderingCallback )
		.def("setGamma", &Stimulus::setGamma, ( arg("gammaList"), arg("invert")=false ) )
		.def("setPythonObject",	&Stimulus::setPythonObject )
		.def("getPythonObject",	&Stimulus::getPythonObject )
		.def("usesChannel",	&Stimulus::usesChannel )
		.def("getChannelCount",	&Stimulus::getChannelCount )
		.def("enableColorMode",	&Stimulus::enableColorMode )
		.add_property(	"duration"					, &Stimulus::getDuration,		&Stimulus::setDuration 		, "Stimulus duration in frames.")
		.def("addTag",&Stimulus::addTag )
		;
	register_ptr_to_python<Stimulus::P>();

	class_<Sequence::RaiseSignal>("RaiseSignal", no_init /*, init<std::string>()*/)
		.def( "__init__", make_constructor( &Sequence::RaiseSignal::create<std::string>) );
	register_ptr_to_python<Sequence::RaiseSignal::P>();
	class_<Sequence::ClearSignal>("ClearSignal", no_init /*, init<std::string>()*/)
		.def( "__init__", make_constructor( &Sequence::ClearSignal::create<std::string>) );
	register_ptr_to_python<Sequence::ClearSignal::P>();
	class_<Sequence::RaiseAndClearSignal>("RaiseAndClearSignal", no_init /*, init<std::string>()*/)
		.def( "__init__", make_constructor( &Sequence::RaiseAndClearSignal::create<std::string, uint>) );
	register_ptr_to_python<Sequence::RaiseAndClearSignal::P>();
	class_<Sequence::StartMeasurement>("StartMeasurement", no_init )
		.def( "__init__", make_constructor( &Sequence::StartMeasurement::create<std::string>, default_call_policies(), 
				(arg("sequenceSyncSignal")="Exp sync" )  ) );
	register_ptr_to_python<Sequence::StartMeasurement::P>();
	class_<Sequence::EndMeasurement>("EndMeasurement", no_init )
		.def( "__init__", make_constructor( &Sequence::EndMeasurement::create<std::string, std::string, uint>, default_call_policies(), 
				(arg("sequenceSyncSignal")="Exp sync", arg("sequenceStopSignal")="Msr stop", arg("holdFrameCount")=1 )  ) );
	register_ptr_to_python<Sequence::EndMeasurement::P>();

	class_<Sequence>("Sequence", no_init /*, init<std::string>()*/)
		.def( "__init__", make_constructor( &Sequence::create<std::string>) )
		.def("set", &Sequence::set)
		.def( "getDuration", &Sequence::getDuration)
		.def("addStimulus", &Sequence::addStimulus )
		.def("getShortestStimulusDuration", &Sequence::getShortestStimulusDuration )
		.def("setAgenda", &Sequence::setAgenda )
		.def("addChannel", &Sequence::addChannel )
		.def("getChannels", &Sequence::getChannels, return_value_policy<reference_existing_object>() )
		.def("getChannelCount", &Sequence::getChannelCount )
		.def("raiseSignal", &Sequence::raiseSignal )
		.def("clearSignal", &Sequence::clearSignal )
		.def("raiseAndClearSignal", &Sequence::raiseAndClearSignal )
		.def("getFrameInterval_s",	&Sequence::getFrameInterval_s )
		.def("usesRandoms",	&Sequence::usesRandoms )
		.def("onReset",	&Sequence::onReset )
		.def("setPythonObject",	&Sequence::setPythonObject )
		.def("getPythonObject",	&Sequence::getPythonObject )
		.def("getStimulusAtFrame", &Sequence::getStimulusAtFrame )
		.def("getStimuli", &Sequence::getStimuli, return_value_policy<reference_existing_object>() )
		.def("setBusyWaitingTickInterval", &Sequence::setBusyWaitingTickInterval )
		.def("getUsesBusyWaitingThreadForSingals", &Sequence::getUsesBusyWaitingThreadForSingals )
		.def("getSpatialFilteredFieldWidth_um"		,	&Sequence::getSpatialFilteredFieldWidth_um )
		.def("getSpatialFilteredFieldHeight_um"		,	&Sequence::getSpatialFilteredFieldHeight_um )
		.def_readwrite(	"name"								, &Sequence::name							, "Sequence name.")
		.def_readwrite(	"field_width_um"					, &Sequence::fieldWidth_um				, "The horizontal extent of the light pattern appearing on the retina [um].")
		.def_readwrite(	"field_height_um"					, &Sequence::fieldHeight_um				, "The vertical extent of the light pattern appearing on the retina [um].")
		.def_readwrite(	"field_width_px"					, &Sequence::fieldWidth_px				, "The size of the light pattern in display device pixels.")
		.def_readwrite(	"field_height_px"					, &Sequence::fieldHeight_px				, "The size of the light pattern in display device pixels.")
		.def_readwrite(	"field_left_px"						, &Sequence::fieldLeft_px					, "The horizontal position of the light pattern in pixels.")
		.def_readwrite(	"field_bottom_px"					, &Sequence::fieldBottom_px				, "The vertical position of the light pattern in pixels.")
		.def_readwrite(	"fft_width_px"						, &Sequence::fftWidth_px					, "The horizontal resolution used for frequency domain filtering [pixels].")
		.def_readwrite(	"fft_height_px"						, &Sequence::fftHeight_px					, "The vertical resolution used for frequency domain filtering [pixels].")
		.def_readwrite(	"monitorIndex"						, &Sequence::monitorIndex				, "Index of the display device to be used for stimulus projection.")
		.def_readwrite(	"deviceFrameRate"					, &Sequence::deviceFrameRate				, "VSYNC frequency of projector device. [1/s].")
		.def_readwrite(	"frameRateDivisor"					, &Sequence::frameRateDivisor				, "VSYNC cycles per frame rendered. [1].")
		.def_readwrite(	"exportRandomsWithHashmarkComments"	, &Sequence::exportRandomsWithHashmark	, "True if comments (denoted with #) should be included in the random numbers file.")
		.def_readwrite(	"exportRandomsChannelCount"			, &Sequence::exportRandomsChannelCount	, "Number of randoms to export for a grid cell [1-4].")
		.def_readwrite(	"exportRandomsAsReal"				, &Sequence::exportRandomsAsReal			, "True if randoms should be exported as floating point numbers.")
		.def_readwrite(	"exportRandomsAsBinary"				, &Sequence::exportRandomsAsBinary		, "True if randoms should be exported as fair 0/1 values.")
		.def_readwrite(	"greyscale"							, &Sequence::greyscale						, "Setting this to true allows faster filtering, but no colors.")
		;

	register_ptr_to_python<Ticker::P>();
	class_<Ticker>("Ticker", no_init /*, init<std::string>()*/)
		.def( "__init__", make_constructor( &Ticker::create<SequenceRenderer::P>) )
		.def("stop", &Ticker::stop)
		.def("onBufferSwap", &Ticker::onBufferSwap)
		;

	register_ptr_to_python<Sequence::P>();
	def("setSequence", setSequence /*, return_value_policy<reference_existing_object>()*/ );
	def("getSequence", getSequence /*, return_value_policy<reference_existing_object>()*/ );
	def("pickStimulus", pickStimulus );
	def("getSelectedStimulus", getSelectedStimulus );
	def("reset", reset );
	def("cleanup", cleanup );
	def("enableExport", enableExport );
	def("enableVideoExport", enableVideoExport);
	def("enableCalibration", enableCalibration );
	def("startTicker", startTicker );
	def("skip", skip );
	def("getCurrentStimulus", getCurrentStimulus );
//	def("setSwapInterval", setSwapInterval );
	def("makePath", makePath );
	def("instantlyRaiseSignal", instantlyRaiseSignal );
	def("instantlyClearSignal", instantlyClearSignal );
	def("setSequenceTimelineZoom", setSequenceTimelineZoom );
	def("setSequenceTimelineStart", setSequenceTimelineStart );
	def("setStimulusTimelineZoom",  setStimulusTimelineZoom );
	def("setStimulusTimelineStart", setStimulusTimelineStart );
	def("pause", pauseRender );
	def("getSkippedFrameCount", getSkippedFrameCount );
	def("getSequenceTimingReport", getSequenceTimingReport );
	def("setText", setText );
	def("showText", showText );
	def("createStimulusWindow", createStimulusWindow );
	def("getSpecs", getSpecs );
	def("makeCurrent", makeCurrent );
	def("shareCurrent", shareCurrent );
	def("run", run );
	def("onHideStimulusWindow", onHideStimulusWindow );
	def("loadTexture", loadTexture );
	def("bindTexture", bindTexture );
	def("setMousePointerLocation", setMousePointerLocation );
	def("updateSpatialKernel", updateSpatialKernel );
	def("getTime", getTime );
	def("renderSample", renderSample );
	def("setResponded", setResponded);
	def("toggleChannelsOrPreview", toggleChannelsOrPreview);
}