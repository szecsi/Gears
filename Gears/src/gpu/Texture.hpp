#ifndef _TEXTURE_
#define _TEXTURE_

#include <string>

#include <GL/glew.h>

// DevIL
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

class Texture1D{
private:
  GLuint handle;
  GLuint width;

public:
  Texture1D();
  ~Texture1D();

  int getWidth(){ return width; }

  void initialize(GLuint width);
  void setData(float* data);

  GLuint getTextureHandle();
};

class Texture2D{
private:
  GLuint handle;
  GLuint width, height;
  GLuint bpp;

public:
  Texture2D();
  ~Texture2D();

  int getWidth(){ return width; }
  int getHeight(){ return height; }

  void initializeGrey(GLuint width, GLuint height, GLuint bpp);
  void setDataGrey(unsigned char* data);

  void initialize8bit(GLuint width, GLuint height, GLuint bpp);
  void setData8bit(unsigned char* data);


  void initialize(GLuint width, GLuint height, GLuint bpp);
  void setData(float* data);
  void setData(float* data, GLuint level);

  void initializeRG(GLuint width, GLuint height, GLuint bpp);
  void setDataRG(float* data);
  void loadFromFile(std::string fileName);
  GLuint getTextureHandle();
};

#endif
