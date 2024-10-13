#pragma once

#include <glm.hpp>
#include <vector>
#include <tuple>
#include <string>

enum  ShaderType : unsigned short
{
	SHADER_VERTEX = 0,
	SHADER_FRAGMENT = 1
};

class Shader
{
public:
	Shader();

	/** Loads the shader source and saves it as the specified shader for later use
	* Overwrites any shader currently bound to the specified type */
	bool LoadShader(const char* ShaderSource, ShaderType shader);

	// Loads a shader from the specified path
	bool LoadShaderFromFile(const char* ShaderPath, ShaderType shader);

	// Compiles the provided shaders to use them in rendering
	void BuildShaderProgram();

	// Makes OpenGL use the shader
	void Use() const;

    void SetBool(const std::string& name, const bool& value) const;
    void SetInt(const std::string& name, const int& value) const;
    void SetFloat(const std::string& name, const float& value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec2(const std::string& name, const float x, const float y) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec3(const std::string& name, const float& x, const float& y, const float& z) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat2(const std::string& name, const glm::mat2& value) const;
    void SetMat3(const std::string& name, const glm::mat3& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;

    unsigned int GetShaderID() { return *ShaderID; }

private:
	unsigned int* ShaderID;
    std::vector<unsigned int*> SubShaderIDs;
};
