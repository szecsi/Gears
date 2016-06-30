#version 150 compatibility

uniform sampler2D inputBuffer;
uniform float histogramLevels;
uniform float domainMin;
uniform float domainMax;

out vec4 color;

float I (vec2 coord){
	vec4 color = texture(inputBuffer, coord);
	return( color.r /*dot(color.rgb, vec3(0.2, 0.39, 0.4))*/ );
}

void main(void){
	vec2 resolution = textureSize(inputBuffer, 0);
	int px = gl_VertexID % int(resolution.x);
	int py = gl_VertexID / int(resolution.x);
	float luminance = I( vec2( px + (gl_VertexID*3631 & 0xf) * 0.0625, py + (gl_VertexID*3119 & 0xf) * 0.0625) / resolution );
	luminance = (luminance - domainMin) / (domainMax - domainMin);
	//gl_Position = vec4(2.0 * (luminance /*/ (1.0 + 1.0 / histogramLevels) + 0.5 / histogramLevels*/ - 0.5) , 0.5, 0.0, 1.0);
	gl_Position = vec4(2.0 * (luminance - 0.5) , 0.5, 0.0, 1.0);

	//gl_Position = vec4(luminance * 2.0 - 1.0, 0.5, 0.0, 1.0);
}

