#pragma once

namespace Phi
{
    // Represents a color, internally a floating point representation
    // Supports transparency
    struct Color
    {
        // Default color (opaque black)
        Color();

        // Color from floating point RGBA
        Color(float r, float g, float b, float a = 1.0f);

        // TODO: More constructors (Hex codes, HTML, HSV)

        // Dtor
        ~Color();

        // Default copy constructor/assignment
        Color(const Color &) = default;
        Color &operator=(const Color &) = default;

        // Default move constructor/assignment
        Color(Color &&other) = default;
        Color &operator=(Color &&other) = default;

        // Color data
        float r;
        float g;
        float b;
        float a;
    };
}