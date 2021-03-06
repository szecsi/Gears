#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "Pointgrid.hpp"
#include "Quad.hpp"

#include <string>
#include <map>


class ShaderManager
{
public:
	using ShaderMap = std::map<std::string, Shader*>;

	Shader* loadShader(std::string fragProgramSource)
	{
		ShaderMap::iterator i = shaders.find(fragProgramSource);
		if (i != shaders.end())
			return i->second;
		Shader* newShader = new Shader(std::string("./Project/Shaders/quad.vert"), fragProgramSource);
		shaders[fragProgramSource] = newShader;
		return newShader;
	}

	Shader* loadShaderFromFile(std::string vertProgramPath, std::string fragProgramPath)
	{
		std::string pathkey = vertProgramPath + " ### " + fragProgramPath;
		ShaderMap::iterator i = shaders.find(pathkey);
		if (i != shaders.end())
			return i->second;
		Shader* newShader = new Shader(vertProgramPath.c_str(), fragProgramPath.c_str());
		shaders[pathkey] = newShader;
		return newShader;
	}

	Shader* loadShaderFromFile(std::string vertProgramPath, std::string geomProgramPath, std::string fragProgramPath)
	{
		std::string pathkey = vertProgramPath + " ### " + geomProgramPath + " ### " + fragProgramPath;
		ShaderMap::iterator i = shaders.find(pathkey);
		if (i != shaders.end())
			return i->second;
		Shader* newShader = new Shader(vertProgramPath.c_str(), fragProgramPath.c_str(), geomProgramPath.c_str());
		shaders[pathkey] = newShader;
		return newShader;
	}

	Shader* loadShader(std::string vertProgramSource, std::string fragProgramSource)
	{
		ShaderMap::iterator i = shaders.find(vertProgramSource + fragProgramSource);
		if (i != shaders.end())
			return i->second;
		Shader* newShader = new Shader(vertProgramSource, fragProgramSource, true);
		shaders[vertProgramSource + fragProgramSource] = newShader;
		return newShader;
	}

	Shader* loadShader(std::string vertProgramSource, std::string geomProgramSource, std::string fragProgramSource)
	{
		ShaderMap::iterator i = shaders.find(vertProgramSource + geomProgramSource + fragProgramSource);
		if (i != shaders.end())
			return i->second;
		Shader* newShader = new Shader(vertProgramSource, geomProgramSource, fragProgramSource);
		shaders[vertProgramSource + geomProgramSource + fragProgramSource] = newShader;
		return newShader;
	}

	GEARS_SHARED_CREATE(ShaderManager);
private:
	ShaderManager(){}

	ShaderMap shaders;
public:
	void clear()
	{
		for(auto i : shaders)
			delete i.second;
		shaders.clear();
	}
};