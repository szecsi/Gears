#include "stdafx.h"
#include "SequenceRenderer.h"
#pragma region OpenGL
#include <GL/glew.h>
#ifdef _WIN32
#include "wglext.h"
#elif __linux__
#include <GL/glx.h>
#endif
#pragma endregion includes for GLEW and WGL
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include "ShaderManager.h"

