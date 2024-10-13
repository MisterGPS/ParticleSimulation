#include "../Public/Shader.h"
#include <glad.h>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <string>

Shader::Shader()
{
    SubShaderIDs = *new std::vector<unsigned int*>();
    SubShaderIDs.push_back(new unsigned int());
    SubShaderIDs.push_back(new unsigned int());
    ShaderID = new unsigned int();
}

bool Shader::LoadShader(const char* ShaderSource, ShaderType shader)
{
	int type = shader == ShaderType::SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

	*SubShaderIDs[shader] = glCreateShader(type);
	glShaderSource(*SubShaderIDs[shader], 1, &ShaderSource, nullptr);
	glCompileShader(*SubShaderIDs[shader]);

	int success;
	glGetShaderiv(*SubShaderIDs[shader], GL_COMPILE_STATUS, &success);

#if DEBUG
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
#endif // DEBUG

	return success;
}

bool Shader::LoadShaderFromFile(const char* ShaderPath, ShaderType shader)
{
	std::string source;
	std::string line;
	std::ifstream FileStream(ShaderPath);

    if (!FileStream.is_open())
    {
        std::cout << "Shader file path could not be found" << std::endl;
        return false;
    }

	while (std::getline(FileStream, line))
	{
		source.append(line);
		source.append("\n");
	}
	FileStream.close();
	source[source.size() - 1] = *"\0";

	bool bSuccess = LoadShader(source.c_str(), shader);

#if DEBUG
	if (!bSuccess)
	{
		std::cout << "Shader: " << ShaderPath << " failed to load" << std::endl;
	}
#endif // DEBUG

	return bSuccess;
}

void Shader::BuildShaderProgram()
{
	*ShaderID = glCreateProgram();
	if (SubShaderIDs[0] != nullptr)
	{
        glAttachShader(*ShaderID, *SubShaderIDs[0]);
	}
	if (SubShaderIDs[1] != nullptr)
	{
		glAttachShader(*ShaderID, *SubShaderIDs[1]);
	}
	glLinkProgram(*ShaderID);

#if DEBUG
	int success;
	glGetProgramiv(*ShaderID, GL_LINK_STATUS, &success);

	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(*ShaderID, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER_PROGRAM::LINK_FAILED\n" << infoLog << std::endl;
	}
#endif // DEBUG
}

void Shader::Use() const
{
	glUseProgram(*ShaderID);
}

void Shader::SetBool(const std::string& name, const bool &value) const
{
    glUniform1i(glGetUniformLocation(*ShaderID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, const int &value) const
{
    glUniform1i(glGetUniformLocation(*ShaderID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, const float &value) const
{
    glUniform1f(glGetUniformLocation(*ShaderID, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2 &value) const
{
    glUniform2fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec2(const std::string& name, const float x, const float y) const
{
    glUniform2f(glGetUniformLocation(*ShaderID, name.c_str()), x, y);
}

void Shader::SetVec3(const std::string& name, const glm::vec3 &value) const
{
    glUniform3fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string &name, const float &x, const float &y, const float &z) const
{
    glUniform3f(glGetUniformLocation(*ShaderID, name.c_str()), x, y, z);
}

void Shader::SetVec4(const std::string& name, const glm::vec4 &value) const
{
    glUniform4fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetMat2(const std::string& name, const glm::mat2 &value) const
{
    glUniformMatrix2fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat3(const std::string& name, const glm::mat3 &value) const
{
    glUniformMatrix4fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const glm::mat4 &value) const
{
    glUniformMatrix4fv(glGetUniformLocation(*ShaderID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
