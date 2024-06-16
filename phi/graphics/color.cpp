#include "color.hpp"

namespace Phi
{
    Color::Color()
        : r(0), g(0), b(0), a(1)
    {
    }

    Color::Color(float r, float g, float b, float a)
        : r(r), g(g), b(b), a(a)
    {
    }

    Color::~Color()
    {
    }

    
}