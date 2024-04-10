#include "file.hpp"

namespace Phi
{
    File::File(const std::string& path, Mode mode)
    {
        // Grab path
        pathToFile = path;

        // Convert to global (remove tokens)
        std::string globalPath = GlobalizePath(pathToFile);

        // Open the file
        fileStream.open(globalPath, std::ios::binary);
    }

    File::~File()
    {
        fileStream.close();
    }

    std::string File::GlobalizePath(const std::string& path)
    {
        // Path to return
        std::string globalPath = path;

        // Replace data folder token
        size_t pos = path.find("data://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, DATA_PATH.size(), DATA_PATH);

        // Replace user folder token
        pos = path.find("user://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, USER_PATH.size(), USER_PATH);

        // Replace user folder token
        pos = path.find("phi://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, PHI_PATH.size(), PHI_PATH);

        // Return the final value
        return std::move(globalPath);
    }

    std::string File::LocalizePath(const std::string& path)
    {
        // Path to return
        std::string localPath = path;

        // Replace data folder
        size_t pos = path.find(DATA_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, 7, "data://");

        // Replace user folder
        pos = path.find(USER_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, 7, "user://");

        // Replace user folder
        pos = path.find(PHI_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, 6, "phi://");

        // Return the final value
        return std::move(localPath);
    }
}