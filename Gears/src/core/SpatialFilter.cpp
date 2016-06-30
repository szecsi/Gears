#include <stdafx.h>
#include "SequenceRenderer.h"
#pragma region OpenGL
#include <GL/glew.h>
#include "wglext.h"
#pragma endregion includes for GLEW, and WGL
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include "SpatialFilter.h"

SpatialFilter::SpatialFilter(std::string name) :
	name(name),
	width_um(500),
	height_um(500),
	useFft(true),
	separable(false),
	horizontalSampleCount(16),
	verticalSampleCount(16),
	kernelGivenInFrequencyDomain(false)
{
	kernelFuncSource = 
		"	in vec2 pos;										\n"
		"	out vec4 outcolor;									\n"
		"	uniform vec2 texelSize_um;									\n"
		"	uniform vec2 patternSizeOnRetina;									\n"
		"	uniform bool kernelGivenInFrequencyDomain;									\n"
//		"	void main() { outcolor = mix(vec4(plotDarkColor.rgb, 0), vec4(plotBrightColor, 1), (kernel(pos) - vec4(plotMin))/vec4(plotMax-plotMin)); }	\n"
		"	void main() { 	\n"
		"		vec4 kernelInTex = vec4(0, 0, 0, 0);										\n"
		"		for(int u=-4; u<=4; u++)									\n"
		"			for(int v=-4; v<=4; v++)	{								\n"
		"				vec2 px = pos + vec2(u, v) / 8.5 * texelSize_um;							\n"
		"				if(kernelGivenInFrequencyDomain) px = mod(px + patternSizeOnRetina, patternSizeOnRetina) - patternSizeOnRetina * 0.5;							\n"
//		"				if(kernelGivenInFrequencyDomain) px /= dot(px, px);							\n"
		"				kernelInTex += 	kernel(px);									\n"
		"				}									\n"
		"		kernelInTex /= 81.0;				\n"
		"		outcolor = mix(vec4(plotDarkColor.rgb, 0), vec4(plotBrightColor, 1), (kernelInTex - vec4(plotMin))/vec4(plotMax-plotMin));									\n"
		"		int cq = (int(gl_FragCoord.x) % 2) ^ (int(gl_FragCoord.y) % 2);		\n"
		"		if(kernelGivenInFrequencyDomain && (  cq == 0) )			\n"
		"			outcolor = -outcolor;									\n"
		"					}						\n"
		;
	setShaderVariable("plotMin", 0);
	setShaderVariable("plotMax", 1);
	setShaderColor("plotDarkColor", -2, 0, 0, 0);
	setShaderColor("plotBrightColor", -2, 1, 1, 1);
	kernelProfileVertexSource = 
		"	uniform vec2 patternSizeOnRetina;		 \n"
		"	void main(void) {		 \n"
		"		float x = float(gl_VertexID)/256.0 - 1.0;			\n"
		"		gl_Position	= vec4(x, (kernel(vec2(x*patternSizeOnRetina.x*0.5, 0)).r - plotMin)/(plotMax-plotMin)*2.0-1, 0.5, 1.0);		\n"
		"	}																										\n"
		;
	kernelProfileFragmentSource = 
		"	void main() {																							\n"
		"		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);															\n"
		"	}																										\n"
		;
}

