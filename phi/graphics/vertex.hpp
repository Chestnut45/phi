#pragma once

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions

namespace Phi
{
    // List of all internal formats, used to construct VertexAttributes objects automagically when possible
    // POS      = 3 GLfloats x, y, z
    // COLOR    = 4 GLfloats r, g, b, a
    // NORM     = 3 GLfloats nx, ny, nz
    // UV       = 2 GLfloats u, v
    enum class VertexFormat
    {
        POS,
        POS_COLOR,
        POS_COLOR_NORM,
        POS_COLOR_NORM_UV,
        POS_COLOR_NORM_UV1_UV2,
        POS_COLOR_UV,
        POS_NORM,
        POS_NORM_UV,
        POS_UV,
        VOXEL,
        VOXEL_HALF_PRECISION
    };

    // Common internal vertex formats that can be used with GPUBuffers, VAOs, and Meshes

    struct VertexPos
    {
        GLfloat x, y, z;
    };

    struct VertexPosColor
    {
        GLfloat x, y, z;
        GLfloat r, g, b, a;
    };

    struct VertexPosColorNorm
    {
        GLfloat x, y, z;
        GLfloat r, g, b, a;
        GLfloat nx, ny, nz;
    };

    struct VertexPosColorNormUv
    {
        GLfloat x, y, z;
        GLfloat r, g, b, a;
        GLfloat nx, ny, nz;
        GLfloat u, v;
    };

    struct VertexPosColorNormUv1Uv2
    {
        GLfloat x, y, z;
        GLfloat r, g, b, a;
        GLfloat nx, ny, nz;
        GLfloat u1, v1;
        GLfloat u2, v2;
    };

    struct VertexPosColorUv
    {
        GLfloat x, y, z;
        GLfloat r, g, b, a;
        GLfloat u, v;
    };

    struct VertexPosNorm
    {
        GLfloat x, y, z;
        GLfloat nx, ny, nz;
    };

    struct VertexPosNormUv
    {
        GLfloat x, y, z;
        GLfloat nx, ny, nz;
        GLfloat u, v;
    };

    struct VertexPosUv
    {
        GLfloat x, y, z;
        GLfloat u, v;
    };
}