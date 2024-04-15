#include "shader.hpp"

#include <phi/core/file.hpp>

namespace Phi
{
    // Initialize the shader program
    Shader::Shader()
    {
        programID = glCreateProgram();
    }

    // Destructor
    Shader::~Shader()
    {
        glDeleteProgram(programID);
    }

    // Methods
    // Sets this shader as the currently active program
    void Shader::Use() const
    {
        glUseProgram(programID);
    }
    
    // TODO: Use Phi::File for reading the file!
    bool Shader::LoadSource(GLenum stage, const std::string& path)
    {
        // Verification vars
        GLint success;
        GLchar infoLog[512];
        
        // Read the file stream
        std::ifstream ifs(File::GlobalizePath(path));
        std::string shaderSourceString((std::istreambuf_iterator<char>(ifs)),
                                        (std::istreambuf_iterator<char>()));
        
        // Format source and create the shader object
        const GLchar* shaderSource = shaderSourceString.c_str();
        GLuint shader = glCreateShader(stage);

        // Compile shader
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);

        // Verify
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            // Compilation failed, report error and return
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR: Shader compilation failed." << infoLog;
            return false;
        }

        // Attach shader to the program and keep track
        glAttachShader(programID, shader);
        shaders.push_back(shader);

        // Successful return if no errors
        return true;
    }

    // Link the shader program
    // This method detaches and deletes all shaders from the program if successful
    bool Shader::Link() const
    {
        // Verification vars
        GLint success;
        GLchar infoLog[512];
        
        // Link the program
        glLinkProgram(programID);

        // Verify link stage
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cout << "ERROR: Shader program link failed." << infoLog;
            return false;
        }

        // Detach and delete all shader objects if linking is successful
        for (GLuint shader : shaders)
        {
            glDetachShader(programID, shader);
            glDeleteShader(shader);
        }

        return true;
    }

    // Binds a uniform block in the shader to a specific binding point
    void Shader::BindUniformBlock(const std::string& blockName, GLuint bindingPoint)
    {
        GLuint index = glGetUniformBlockIndex(programID, blockName.c_str());
        glUniformBlockBinding(programID, index, bindingPoint);
    }

    // Sets a uniform int value in the shader
    void Shader::SetUniform(const std::string& name, unsigned int value)
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    // Sets a uniform int value in the shader
    void Shader::SetUniform(const std::string& name, int value)
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    // Sets a uniform float value in the shader
    void Shader::SetUniform(const std::string& name, float value)
    {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetUniform(const std::string& name, const glm::vec3& value)
    {
        glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::SetUniform(const std::string& name, const glm::vec4& value)
    {
        glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::SetUniform(const std::string& name, const glm::mat4& value)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, false, glm::value_ptr(value));
    }
}