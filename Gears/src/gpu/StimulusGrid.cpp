
#include "StimulusGrid.hpp"

#include <string>
#include <iostream>
#include <cmath>

StimulusGrid::StimulusGrid(GLuint width, GLuint height, void* initData){
	this->width = width;
	this->height = height;

	glGenFramebuffers(1, &handle);
	glGenTextures(1, &colorBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, handle);

	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, initData);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	buffer = GL_COLOR_ATTACHMENT0;
	if(glGetError() != GL_NO_ERROR){
		std::cout << "StimulusGrid: Error creating color attachment" << std::endl;
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "StimulusGrid: Incomplete StimulusGrid (";
		switch(status){
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cout << "GL_FRAMEBUFFER_UNSUPPORTED";
			break;
		}
		std::cout << ")" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

StimulusGrid::~StimulusGrid(){
	glDeleteFramebuffers(1, &handle);
}

void StimulusGrid::setRenderTarget()
{
	glBindFramebuffer(GL_FRAMEBUFFER, handle);

	glViewport(0, 0, width, height);

	glDrawBuffers(1, &buffer);
}

void StimulusGrid::disableRenderTarget(){
	GLenum tmpBuff[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, tmpBuff);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

