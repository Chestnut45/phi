#pragma once

#define GLEW_NO_GLU
#include <GL/glew.h>

namespace Phi
{
    struct DrawArraysCommand
    {
        GLuint count; // The number of consumed vertices
        GLuint instanceCount; // The number of instances to be rendered
        GLuint first; // Starting index in the enabled arrays
        GLuint baseInstance; // Base instance for instanced vertex attributes
    };

    struct DrawElementsCommand
    {
        GLuint count; // The number of consumed vertices (3 tris = 9 count / number of indices)
        GLuint instanceCount; // The number of instances of the particular mesh to draw
        GLuint firstIndex; // The index of the first index relative to the start of the buffer
        GLuint baseVertex; // The index of the first vertex relative to the start of the buffer
        GLuint baseInstance; // Base instance for instanced vertex attributes
    };
}