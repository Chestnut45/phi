#pragma once

#include <fstream>
#include <filesystem>
#include <string>

namespace Phi
{
    // Wrapper class to access files in the filesystem
    // Uses _ONLY_ UNIX-style file separators ('/')
    // There are special tokens that refer to implicit paths:
    // 1. "data://" - The project's data folder (e.g. resources, should be copied to program install location)
    // 2. "user://" - The user's persistent folder for the project (e.g. save file location)
    // 3. "phi://" - Internal engine data (e.g. built-in resources, should be read-only)
    class File
    {
        // Interface
        public:

            // Valid file modes
            enum class Mode
            {
                Read,
                Write,
                ReadWrite
            };

            // Opens the file at the given path in the given mode
            File(const std::string& path, Mode mode);
            ~File();

            // Delete copy constructor/assignment
            File(const File&) = delete;
            File& operator=(const File&) = delete;

            // Delete move constructor/assignment
            File(File&& other) = delete;
            void operator=(File&& other) = delete;

            // Initialization
            // Called by Phi::App automatically on construction
            // Engine users should not have to call this unless changing the default user:// or data:// paths
            static void Init();

            // Path globalization / localization

            // Converts a local path using any special tokens to a fully qualified global path
            static std::string GlobalizePath(const std::string& path);

            // Converts a fully qualified global path to a local path using special tokens
            static std::string LocalizePath(const std::string& path);

            // Special path access

            // Gets the global path to the special data folder
            static std::string GetDataPath() { return DATA_PATH; }

            // Gets the global path to the special user folder
            static std::string GetUserPath() { return USER_PATH; }

            // Gets the global path to the special engine folder
            static std::string GetPhiPath() { return PHI_PATH; }

            // TODO:

            // Cursor / seeking

            // Stream input / output

        // Data / implementation
        private:

            // Path to the file
            std::string pathToFile;

            // Stream object for the file's data
            std::fstream fileStream;

            // Special file paths (set during app initialization)
            static inline std::string DATA_PATH;
            static inline std::string USER_PATH;
            static inline std::string PHI_PATH;
    };
}