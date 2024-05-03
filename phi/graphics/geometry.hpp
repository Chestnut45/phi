#include <phi/core/math/constants.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/vertex.hpp>

// Geometry data used in various places
// All specific shapes are wrapped in a namespace so globals can be constrained to their scope
namespace Phi
{
    namespace Cube
    {
        // Half length of unit cube
        static const float r = 0.5f;

        static const VertexPos UNIT_CUBE_VERTICES[] =
        {
            // Z+
            {-r, r, r},
            {r, r, r},
            {-r, -r, r},
            {r, -r, r},

            // Z-
            {r, r, -r},
            {-r, r, -r},
            {r, -r, -r},
            {-r, -r, -r},

            // Y+
            {-r, r, -r},
            {r, r, -r},
            {-r, r, r},
            {r, r, r},

            // Y-
            {-r, -r, -r},
            {-r, -r, r},
            {r, -r, -r},
            {r, -r, r},

            // X+
            {r, r, r},
            {r, r, -r},
            {r, -r, r},
            {r, -r, -r},

            // X-
            {-r, r, -r},
            {-r, r, r},
            {-r, -r, -r},
            {-r, -r, r}
        };

        static const GLuint UNIT_CUBE_INDICES[] =
        {
            0, 2, 1,
            1, 2, 3,

            4, 6, 5,
            5, 6, 7,

            8, 10, 9,
            9, 10, 11,

            12, 14, 13,
            13, 14, 15,

            16, 18, 17,
            17, 18, 19,

            20, 22, 21,
            21, 22, 23
        };
    }

    namespace Icosphere
    {
        // Icosphere data
        static const float X = 1.0f;
        static const float Z = 1.0f / PHI;

        static const VertexPos UNIT_ICOSPHERE_VERTICES[] =
        {
            {0.0f, Z, -X},
            {Z, X, 0.0f},
            {-Z, X, 0.0f},
            {0.0f, Z, X},
            {0.0f, -Z, X},
            {-X, 0.0f, Z},
            {0.0f, -Z, -X},
            {X, 0.0f, -Z},
            {X, 0.0f, Z},
            {-X, 0.0f, -Z},
            {Z, -X, 0.0f},
            {-Z, -X, 0.0f}
        };

        static const GLuint UNIT_ICOSPHERE_INDICES[] =
        {
            2, 1, 0,
            1, 2, 3,
            5, 4, 3,
            4, 8, 3,
            7, 6, 0,
            6, 9, 0,
            11, 10, 4,
            10, 11, 6,
            9, 5, 2,
            5, 9, 11,
            8, 7, 1,
            7, 8, 10,
            2, 5, 3,
            8, 1, 3,
            9, 2, 0,
            1, 7, 0,
            11, 9, 6,
            7, 10, 6,
            5, 11, 4,
            10, 8, 4
        };
    }
}