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


class TextureManager
{
public:
	using TextureMap = std::map<std::string, Texture2D*>;

	Texture2D* loadTexture(std::string texturePath)
	{
		TextureMap::iterator i = textures.find(texturePath);
		if (i != textures.end())
			return i->second;
		Texture2D* newTexture = new Texture2D();
		newTexture->loadFromFile(texturePath.c_str());
		textures[texturePath] = newTexture;
		return newTexture;
	}

	GEARS_SHARED_CREATE(TextureManager);

private:
	TextureManager(){}
	TextureMap textures;
public:
	void clear()
	{
		for(auto i : textures)
			delete i.second;
		textures.clear();
	}
	Texture2D* get(std::string name)
	{
		TextureMap::iterator i = textures.find(name);
		if (i == textures.end())
			return nullptr;
		return i->second;
	}
	bool add(std::string name, Texture2D* tex)
	{
		TextureMap::iterator i = textures.find(name);
		if (i == textures.end())
		{
			textures[name] = tex;
			return true;
		}
		return false;
	}
};