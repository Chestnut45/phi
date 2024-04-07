#include "file.hpp"

namespace Phi
{
    File::File(const std::string& filepath, Mode mode)
    {
        // Grab path
        path = filepath;
        std::string realPath = path;

        // TODO: Replace engine special folder tokens with OS path

        size_t pos = std::string::npos;
        if ((pos = realPath.find("user://", 0)) != std::string::npos)
        {
            realPath.replace(pos, 7, "");
        }

        pos = std::string::npos;
        if ((pos = realPath.find("data://", 0)) != std::string::npos)
        {
            realPath.replace(pos, 7, "");
        }

        // Open the file
        file.open(path, std::ios::binary);
    }

    File::~File()
    {
        file.close();
    }


}