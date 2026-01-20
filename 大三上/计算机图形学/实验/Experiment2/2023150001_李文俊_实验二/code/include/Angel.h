//////////////////////////////////////////////////////////////////////////////
//
//  --- Angel.h ---
//
//   The main header file for examples, adapted for Experiment2
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __ANGEL_H__
#define __ANGEL_H__

#include <cmath>
#include <iostream>

#ifndef M_PI
#  define M_PI  3.14159265358979323846
#endif

#define GL_SILENCE_DEPRECATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

namespace Angel {

// Helper function to load vertex and fragment shader files
GLuint InitShader( const char* vertexShaderFile,
                   const char* fragmentShaderFile );

const GLfloat  DivideByZeroTolerance = GLfloat(1.0e-07);
const GLfloat  DegreesToRadians = M_PI / 180.0;

}  // namespace Angel

#include "CheckError.h"

#define Print(x)  do { std::cerr << #x " = " << (x) << std::endl; } while(0)

using namespace Angel;

#endif // __ANGEL_H__