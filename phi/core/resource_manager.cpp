#include "resource_manager.hpp"

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

    Texture2D* ResourceManager::LoadTexture2D(const std::string& filepath, Texture2D::FilterMode filterMode)
    {
        // Return cached texture if already loaded
        if (loadedTextures.count(filepath) > 0)
        {
            auto& texData = loadedTextures[filepath];
            texData.refCount++;
            return texData.texture;
        }

        // Grab the filter enum
        GLenum filterEnum = (filterMode == Texture2D::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR);

        // Attempt to load the texture from file
        Texture2D* texture = new Texture2D(filepath, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, filterEnum, filterEnum);

        // Ensure texture loaded properly
        if (texture->GetWidth() < 1 || texture->GetHeight() < 1)
        {
            // Make sure we don't leak VRAM
            delete texture;

            // Log an error message and return
            Error("Resource Manager texture failed to load: ", filepath);
            return nullptr;
        }

        // Update cache and return non-owning pointer to texture
        auto& texData = loadedTextures[filepath];
        texData.texture = texture;
        texData.refCount++;

        return texture;
    }

    void ResourceManager::UnloadTexture2D(const std::string& filepath, bool force)
    {
        // Only bother if texture is actually loaded
        if (loadedTextures.count(filepath) > 0)
        {
            // Decrease reference counter
            auto& texData = loadedTextures[filepath];
            texData.refCount--;

            if (texData.refCount == 0 || force)
            {
                // Free texture resource and remove from cache
                delete texData.texture;
                loadedTextures.erase(filepath);
            }
        }
    }
}