#include "model.hpp"

// DEPRECATION WARNING:
// This class is deprecated and will be removed at a later date.
// It is around only for backwards compatibility with the cityscape assignment.

namespace Phi
{    
    Model::Model(const std::string& objPath)
    {
        // Create the importer and read the model file
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(objPath, aiProcess_Triangulate);

        // Ensure the scene was imported correctly
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            std::cout << "Error: Assimp: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Process all nodes, starting at the root node
        ProcessNode(scene->mRootNode, scene);
    }

    Model::~Model()
    {
    }

    void Model::Draw(const Shader& shader) const
    {
        for (const auto& mesh : meshes)
        {
            mesh.Draw(shader);
        }
    }

    // For Model::DrawInstances(...), check the header (templated code must be accessible)

    void Model::ProcessNode(aiNode* node, const aiScene* scene)
    {
        // Add all meshes from the node
        for(size_t i = 0; i < node->mNumMeshes; ++i)
        {
            AddMesh(scene->mMeshes[node->mMeshes[i]], scene);
        }

        // Process all sub-nodes
        for(size_t i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    void Model::AddMesh(aiMesh* mesh, const aiScene* scene)
    {
        // Initialize vertex data vectors
        std::vector<GLuint> indices;
        std::vector<Vertex> vertices;

        // Copy all vertices from the mesh
        for (size_t i = 0; i < mesh->mNumVertices; ++i)
        {
            Vertex v{0.0f};

            v.x = mesh->mVertices[i].x;
            v.y = mesh->mVertices[i].y;
            v.z = mesh->mVertices[i].z;

            // Add normals if present
            if (mesh->mNormals)
            {
                v.nx = mesh->mNormals[i].x;
                v.ny = mesh->mNormals[i].y;
                v.nz = mesh->mNormals[i].z;
            }

            // Add colors if present
            if (mesh->mColors[0])
            {
                v.r = mesh->mColors[0][i].r;
                v.g = mesh->mColors[0][i].g;
                v.b = mesh->mColors[0][i].b;
                v.a = mesh->mColors[0][i].a;
            }

            // Add UVs if present
            if (mesh->mTextureCoords[0])
            {
                v.u1 = mesh->mTextureCoords[0][i].x;
                v.v1 = mesh->mTextureCoords[0][i].y;
            }
            if (mesh->mTextureCoords[1])
            {
                v.u2 = mesh->mTextureCoords[1][i].x;
                v.v2 = mesh->mTextureCoords[1][i].y;
            }
            
            vertices.push_back(v);
        }

        // Copy indices
        for(size_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for(size_t j = 0; j < face.mNumIndices; ++j)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        // Construct mesh object in-place (no copy)
        meshes.emplace_back(&vertices, &indices);

        // Grab a reference to the one we just emplaced
        Mesh<Vertex>& meshObj = meshes[meshes.size() - 1];

        // Load material textures
        if (mesh->mMaterialIndex >= 0)
        {
            // Grab the material associated with the mesh
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString path;

            // TODO: Turn below into AddMaterialTextures(aiMaterial*, Mesh&, aiTextureType, TexUnitStart, TexUnitEnd)

            // Load albedo textures
            for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i)
            {
                // Stop importing textures if we have no more room
                if (i + (int)TexUnit::ALBEDO_1 > (int)TexUnit::ALBEDO_2) return;

                // Grab the path and load the texture into the mesh object
                material->GetTexture(aiTextureType_DIFFUSE, i, &path);
                meshObj.AddTexture((TexUnit)((int)TexUnit::ALBEDO_1 + i),
                                    path.C_Str(),
                                    GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER,
                                    GL_NEAREST, GL_NEAREST,
                                    true);
            }

            // Load specular maps
            for (int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); ++i)
            {
                // Stop importing textures if we have no more room
                if (i + (int)TexUnit::SPECULAR_1 > (int)TexUnit::SPECULAR_2) return;

                // Grab the path and load the texture into the mesh object
                material->GetTexture(aiTextureType_SPECULAR, i, &path);
                meshObj.AddTexture((TexUnit)((int)TexUnit::SPECULAR_1 + i),
                                    path.C_Str(),
                                    GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER,
                                    GL_NEAREST, GL_NEAREST,
                                    true);
            }

            // Load normal maps
            for (int i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); ++i)
            {
                // Stop importing textures if we have no more room
                if (i + (int)TexUnit::NORMAL_1 > (int)TexUnit::NORMAL_2) return;

                // Grab the path and load the texture into the mesh object
                material->GetTexture(aiTextureType_NORMALS, i, &path);
                meshObj.AddTexture((TexUnit)((int)TexUnit::NORMAL_1 + i),
                                    path.C_Str(),
                                    GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER,
                                    GL_NEAREST, GL_NEAREST,
                                    true);
            }
        }

        // Commit the mesh data
        meshObj.Commit();
    }
}