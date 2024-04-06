#pragma once

// DEPRECATION WARNING:
// This class is deprecated and will be removed at a later date.
// It is around only for backwards compatibility with the cityscape assignment.

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include <phi/graphics/mesh.hpp>

namespace Phi
{
    class Model
    {
        // Interface
        public:
            
            // Internal vertex format for models
            typedef VertexPosColorNormUv1Uv2 Vertex;

            Model(const std::string& objPath);
            ~Model();

            // Delete copy constructor/assignment
            Model(const Model&) = delete;
            Model& operator=(const Model&) = delete;

            // Delete move constructor/assignment
            Model(Model&& other) = delete;
            void operator=(Model&& other) = delete;

            // Immediately render to the current FBO
            void Draw(const Shader& shader) const;

            // Immediately render iData.size() instances of the model to the current FBO, uploads iData 
            // directly to the static instance buffer and binds it to SSBOBinding::InstanceBuffer
            template <typename InstanceData>
            void DrawInstances(const Shader& shader, const std::vector<InstanceData>& iData) const;

            // Const access to individual mesh resources by index
            const Mesh<Vertex>& GetMesh(int index) const { return meshes[index]; };

        // Data / implementation
        private:
            
            std::vector<Mesh<Vertex>> meshes;

            // Helper assimp loading / processing methods
            void ProcessNode(aiNode* node, const aiScene* scene);
            void AddMesh(aiMesh* mesh, const aiScene* scene);
    };

    // Templated implementations
    template <typename InstanceData>
    void Model::DrawInstances(const Shader& shader, const std::vector<InstanceData>& iData) const
    {
        // Upload instance data and bind the buffer
        MeshResources::instanceBuffer->Sync();
        MeshResources::instanceBuffer->Write(iData.data(), iData.size() * sizeof(InstanceData));
        MeshResources::instanceBuffer->BindRange(GL_SHADER_STORAGE_BUFFER,
                                                (int)SSBOBinding::InstanceBuffer,
                                                MeshResources::INSTANCE_BUFFER_SIZE * MeshResources::instanceBuffer->GetCurrentSection(),
                                                MeshResources::INSTANCE_BUFFER_SIZE);
        
        // Instance all meshes without reuploading data
        for (int i = 0; i < meshes.size(); ++i)
        {
            meshes[i].DrawInstances(shader, iData.size());
        }

        // Lock the buffer section and switch to the next one
        MeshResources::instanceBuffer->Lock();
        MeshResources::instanceBuffer->SwapSections();
    }
}