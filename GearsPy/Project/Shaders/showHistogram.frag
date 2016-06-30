#version 150 compatibility

uniform sampler2D histogramBuffer;
uniform float maxValue;
uniform float domainMin;
uniform float domainMax;

in vec2  fTexCoord;

out vec4 outColor;


void main() {
//	float data = texture(histogramBuffer, fTexCoord).x;
// red black
//	if(data / maxValue > fTexCoord.y){
//		if(fTexCoord.x + 0.001 >= (0-domainMin) / (domainMax - domainMin) && fTexCoord.x  - 0.001 <= (1-domainMin) / (domainMax - domainMin))
//			outColor = vec4(0.5, 0.5, 0.0, 1.0);
//		else
//			outColor = vec4(1.0, 0.0, 0.0, 1.0);
//	} else {
//		if(fTexCoord.x + 0.001 > (0-domainMin) / (domainMax - domainMin) && fTexCoord.x - 0.001 < (1-domainMin) / (domainMax - domainMin))
//			outColor = vec4(0.0, 0.3, 0.0, 1.0);
//		else
//			outColor = vec4(0.0, 0.0, 0.0, 1.0);
//	}

//print mode
	float fy = fTexCoord.y * 1.2 - 0.2;
	float fx = fTexCoord.x * 1.1 - 0.05;
	float data = texture(histogramBuffer, vec2(fx, 0.5)).x;
	if(fx < 0 || fx > 1)
		outColor = vec4(0.5, 0.5, 0.5, 1.0);
	else if(fy < 0)
		outColor = vec4(1.0, 1.0, 1.0, 1.0);
	else if(data / maxValue > fy * 1.2){
		if(fx + 0.001 >= (0-domainMin) / (domainMax - domainMin) && fx  - 0.001 <= (1-domainMin) / (domainMax - domainMin))
			outColor = vec4(0.0, 0.1, 0.0, 1.0);
		else
			outColor = vec4(1.0, 0.0, 0.0, 1.0);
	} else {
		if(fx + 0.001 > (0-domainMin) / (domainMax - domainMin) && fx - 0.001 < (1-domainMin) / (domainMax - domainMin))
			outColor = vec4(0.9, 0.9, 0.9, 1.0);
		else
			outColor = vec4(1.0, 1.0, 1.0, 1.0);
	}
}

