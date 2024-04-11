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

    void File::Init()
    {
        // Grab the current path in the correct format
        std::string currentPath = std::filesystem::current_path().generic_string() + "/";

        // TODO: Generate folders in proper locations
        DATA_PATH = currentPath + "data/";
        USER_PATH = currentPath + "user/";
        PHI_PATH = currentPath + "phi/";
    }

    std::string File::GlobalizePath(const std::string& path)
    {
        // Path to return
        std::string globalPath = path;

        // Replace data folder token
        size_t pos = path.find("data://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, 7, DATA_PATH);

        // Replace user folder token
        pos = path.find("user://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, 7, USER_PATH);

        // Replace user folder token
        pos = path.find("phi://", 0);
        if (pos != std::string::npos) globalPath.replace(pos, 6, PHI_PATH);

        // Return the final value
        return std::move(globalPath);
    }

    std::string File::LocalizePath(const std::string& path)
    {
        // Path to return
        std::string localPath = path;

        // Replace data folder
        size_t pos = path.find(DATA_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, DATA_PATH.size(), "data://");

        // Replace user folder
        pos = path.find(USER_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, USER_PATH.size(), "user://");

        // Replace user folder
        pos = path.find(PHI_PATH, 0);
        if (pos != std::string::npos) localPath.replace(pos, PHI_PATH.size(), "phi://");

        // Return the final value
        return std::move(localPath);
    }
}