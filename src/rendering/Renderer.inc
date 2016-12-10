#include "XPLMGraphics.h"

// @author nstemmle
// This should be included in any file that needs to use opengl or XPlane drawing functions
// Handle cross platform differences
#if IBM
#include <gl\gl.h>
#include <gl\glu.h>
#elif LIN
#include <GL/gl.h>
#include <GL/glu.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl.h>
#include <glu.h>
#endif