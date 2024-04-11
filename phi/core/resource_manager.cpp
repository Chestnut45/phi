#include "resource_manager.hpp"

#include <phi/core/file.hpp>
#include <phi/core/logging.hpp>

namespace Phi
{
    ResourceManager::ResourceManager()
    {
    }

    ResourceManager::~ResourceManager()
    {
        // Cleanup all textures
        for (auto& entry : loadedTextures)
        {
            // Delete the Texture2D resource
            delete entry.second.texture;
        }
        loadedTextures.clear();
    }

    Texture2D* ResourceManager::LoadTexture2D(const std::string& path, Texture2D::FilterMode filterMode)
    {
        // Globalize the path first
        std::string globalPath = File::GlobalizePath(path);

        // Return cached texture if already loaded
        if (loadedTextures.count(globalPath) > 0)
        {
            auto& texData = loadedTextures[globalPath];
            texData.refCount++;
            return texData.texture;
        }

        // Choose the correct OpenGL filter enum
        // TODO: Eventually this should be part of another simpler Texture2D constructor
        GLenum filterEnum = (filterMode == Texture2D::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR);

        // Attempt to load the texture from file
        Texture2D* texture = new Texture2D(globalPath, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, filterEnum, filterEnum);

        // Ensure texture loaded properly
        if (texture->GetWidth() < 1 || texture->GetHeight() < 1)
        {
            // Make sure we don't leak VRAM
            delete texture;

            // Log an error message and return
            Error("Resource Manager texture failed to load: ", globalPath);
            return nullptr;
        }

        // Update cache and return non-owning pointer to texture
        auto& texData = loadedTextures[globalPath];
        texData.texture = texture;
        texData.refCount++;

        return texture;
    }

    void ResourceManager::UnloadTexture2D(const std::string& path, bool force)
    {
        // Globalize the path first
        std::string globalPath = File::GlobalizePath(path);

        // Only bother if texture is actually loaded
        if (loadedTextures.count(globalPath) > 0)
        {
            // Decrease reference counter
            auto& texData = loadedTextures[globalPath];
            texData.refCount--;

            if (texData.refCount == 0 || force)
            {
                // Free texture resource and remove from cache
                delete texData.texture;
                loadedTextures.erase(globalPath);
            }
        }
    }
}