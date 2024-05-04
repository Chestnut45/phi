#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions

namespace Phi
{
    // Shader management class
    class Shader
    {
        public:

            Shader();
            ~Shader();

            // Loading / compiling

            // Loads shader source code from a file
            // Accepts local paths like data:// and user://
            bool LoadSource(GLenum stage, const std::string& path);
            bool Link() const;

            // Delete copy constructor/assignment
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;

            // Delete move constructor/assignment
            Shader(Shader&& other) = delete;
            void operator=(Shader&& other) = delete;

            // Set as the active program
            void Use() const;

            // Uniform / binding manipulation
            // NOTE: All calls to SetUniform() are only valid following a call to Use()!
            void BindUniformBlock(const std::string& blockName, GLuint bindingPoint);
            void SetUniform(const std::string& name, unsigned int value);
            void SetUniform(const std::string& name, int value);
            void SetUniform(const std::string& name, float value);
            void SetUniform(const std::string& name, const glm::vec2& value);
            void SetUniform(const std::string& name, const glm::vec3& value);
            void SetUniform(const std::string& name, const glm::vec4& value);
            void SetUniform(const std::string& name, const glm::mat4& value);

            // Accessors
            inline GLuint GetProgramID() const { return programID; };
        
        // Implementation
        private:
            
            // Identifiers
            GLuint programID;

            // IDs of individual shaders
            std::vector<GLuint> shaders;

            // Map of uniform locations by name
            std::unordered_map<std::string, GLint> uniformLocations;

            // Helper function to get the location of a uniform
            // Retrieves from internal cache if accessed before
            // TODO: Benchmark against just wrapping glGetUniformLocation()
            inline GLint GetUniformLocation(const std::string& name)
            {
                const auto& it = uniformLocations.find(name);
                if (it != uniformLocations.end())
                {
                    // Uniform has been accessed before, use cached value
                    return it->second;
                }

                // Uniform has not been cached, cache the location and return it
                GLint loc = glGetUniformLocation(programID, name.c_str());
                uniformLocations[name] = loc;
                return loc;
            }
    };
}