#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <filesystem>

namespace sndx {
	enum class ShaderType : GLenum {
		Vertex = GL_VERTEX_SHADER,
		Geometry = GL_GEOMETRY_SHADER,
		Fragment = GL_FRAGMENT_SHADER,
	};

	[[nodiscard]]
	ShaderType determineShaderType(const std::filesystem::path& path) {
		auto ext = path.extension();
		if (ext == ".vs") {
			return ShaderType::Vertex;
		}
		else if (ext == ".gs") {
			return ShaderType::Geometry;
		}
		else if (ext == ".fs") {
			return ShaderType::Fragment;
		}
		else {
			throw std::runtime_error("Could not determine Shader Type for " + path.string());
		}
	}

	template <class T>
	void checkShaderErr(T&& obj) {
		GLint success;
		glGetShaderiv(obj.id, GL_COMPILE_STATUS, &success);

		if (!success) {
			int logLen;
			glGetShaderiv(obj.id, GL_INFO_LOG_LENGTH, &logLen);

			std::string errMsg;
			errMsg.resize(logLen + 1ll);
			glGetShaderInfoLog(obj.id, logLen, NULL, errMsg.data());
			obj.destroy();
			throw std::runtime_error(errMsg);
		}
	}

	struct Shader {
		GLuint id;

		Shader() :
			id(0) {}

		Shader(const char* code, ShaderType type) :
			id(glCreateShader((GLenum)type)) {

			glShaderSource(id, 1, &code, nullptr);
			glCompileShader(id);

			checkShaderErr(*this);
		}

		void destroy() {
			glDeleteShader(id);
			id = 0;
		}
	};

	struct ShaderProgram {
		GLuint id;

		std::unordered_map<std::string, GLint> uniformCache;

		explicit ShaderProgram() :
			id(0), uniformCache{} {}

		ShaderProgram(ShaderProgram&& other) noexcept :
			id(std::move(other.id)), uniformCache(std::move(other.uniformCache)) {

			other.id = 0;
			other.uniformCache = {};
		}

		template <class T>
		explicit ShaderProgram(T& shaders) :
			id(glCreateProgram()), uniformCache{} {

			for (auto shader : shaders) {
				glAttachShader(id, shader.id);
			}

			glLinkProgram(id);

			checkShaderErr(*this);

			for (auto shader : shaders) {
				glDetachShader(id, shader.id);
			}
		}

		void use() const {
			glUseProgram(id);
		}

		void destroy() {
			glDeleteProgram(id);
			id = 0;
		}

		[[nodiscard]]
		GLint getUniformLocation(const std::string& uid) {
			auto it = uniformCache.find(uid);
			if (it != uniformCache.end()) [[likely]] {
				return it->second;
			}

			auto ret = glGetUniformLocation(id, uid.c_str());
			uniformCache.emplace(uid, ret);
			return ret;
		}


		void uniform(const std::string& uid, int data) {
			glUniform1i(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, unsigned int data) {
			glUniform1ui(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, float data) {
			glUniform1f(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, glm::vec2 data) {
			glUniform2f(getUniformLocation(uid), data.x, data.y);
		}

		void uniform(const std::string& uid, glm::vec3 data) {
			glUniform3f(getUniformLocation(uid), data.x, data.y, data.z);
		}

		void uniform(const std::string& uid, glm::vec4 data) {
			glUniform4f(getUniformLocation(uid), data.x, data.y, data.z, data.w);
		}

		void uniform(const std::string& uid, glm::mat2 data) {
			glUniformMatrix2fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}

		void uniform(const std::string& uid, glm::mat3 data) {
			glUniformMatrix3fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}

		void uniform(const std::string& uid, glm::mat4 data) {
			glUniformMatrix4fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}
	};

	[[nodiscard]]
	static Shader shaderFromFile(const std::filesystem::path& path, ShaderType type) {
		std::ifstream file(path);

		if (file.is_open()) {
			std::stringstream strstr;
			strstr << file.rdbuf();

			return Shader(strstr.str().c_str(), type);
		}

		throw std::runtime_error("Failed to open file: " + path.string());
	}

	[[nodiscard]]
	static Shader shaderFromFile(const std::filesystem::path& path) {
		auto Ftype = determineShaderType(path);

		return shaderFromFile(path, Ftype);
	}

	template <class... Files> [[nodiscard]]
	static ShaderProgram programFromFiles(Files&&... files) {

		std::array<Shader, sizeof...(files)> shaders{};

		auto&& loci = { files... };
		size_t i = 0;
		for (auto&& file : loci) {
			shaders[i] = shaderFromFile(file);
			++i;
		}

		auto out = ShaderProgram(shaders);

		for (auto& shader : shaders) {
			shader.destroy();
		}

		return out;
	}
}