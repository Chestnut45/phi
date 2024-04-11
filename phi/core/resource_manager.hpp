#pragma once

#include <string>
#include <unordered_map>

#include <phi/graphics/texture_2d.hpp>

namespace Phi
{
    // A singleton used for loading / accessing resources available to apps
    class ResourceManager
    {
        // Interface
        public:

            // Access to singleton instance
            static ResourceManager& Instance()
            {
                // Constructed on first use
                // Guaranteed to be destroyed
                static ResourceManager instance;
                return instance;
            }
            
            // Delete copy constructor/assignment
            ResourceManager(const ResourceManager&) = delete;
            ResourceManager& operator=(const ResourceManager&) = delete;

            // Delete move constructor/assignment
            ResourceManager(ResourceManager&& other) = delete;
            ResourceManager& operator=(ResourceManager&& other) = delete;

            // Texture management

            // Loads a 2D texture from disk, increasing an internal reference counter
            // Does not create copies if the same file is loaded multiple times
            // Accepts local paths like data:// and user://
            Texture2D* LoadTexture2D(const std::string& path, Texture2D::FilterMode filterMode = Texture2D::FilterMode::Nearest);

            // Decreases the reference counter for the given texture filepath
            // Fully unloads the resource if the reference counter reaches 0 or force is given the value of true
            // Accepts local paths like data:// and user://
            void UnloadTexture2D(const std::string& path, bool force = false);

        // Data / implementation
        private:

            // Private ctor / dtor
            ResourceManager();
            ~ResourceManager();

            // Texture resources

            // Struct for refcounting each texture
            struct TexData
            {
                Texture2D* texture = nullptr;
                size_t refCount = 0;
            };

            // Mapping of filepaths to loaded textures
            std::unordered_map<std::string, TexData> loadedTextures;
    };
}