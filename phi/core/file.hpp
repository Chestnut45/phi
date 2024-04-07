#pragma once

#include <fstream>
#include <filesystem>
#include <string>

namespace Phi
{
    // Wrapper class to access files in the filesystem
    // Uses UNIX-style file separators ('/')
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
            File(const std::string& filepath, Mode mode);
            ~File();

            // Delete copy constructor/assignment
            File(const File&) = delete;
            File& operator=(const File&) = delete;

            // Delete move constructor/assignment
            File(File&& other) = delete;
            void operator=(File&& other) = delete;

            // TODO: Initialization and engine / project paths setup

            // Global / Local conversions

            // Cursor / seeking

        // Data / implementation
        private:

            // Path to the file
            std::string path;

            // Stream object for the file's data
            std::fstream file;

            // Special file paths (set during app initialization)
            static std::string USER_PATH;
            static std::string DATA_PATH;
    };
}