#pragma once

#define NOMINMAX
#include <gl/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <array>
#include <vector>

namespace sndx {
	enum class ShaderType : GLenum {
		Vertex = GL_VERTEX_SHADER,
		Geometry = GL_GEOMETRY_SHADER,
		Fragment = GL_FRAGMENT_SHADER,
	};

	[[nodiscard]]
	inline ShaderType determineShaderType(const std::filesystem::path& path) {
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

	struct Shader {
		GLuint id;

		constexpr Shader() noexcept:
			id(0) {}

		Shader(const char* code, ShaderType type) :
			id(glCreateShader((GLenum)type)) {

			glShaderSource(id, 1, &code, nullptr);
			glCompileShader(id);

			auto err = checkErr();
			if (err.has_value()) [[unlikely]] {
				throw std::runtime_error(err.value());
			}
		}

		void destroy() {
			glDeleteShader(id);
			id = 0;
		}
		
	protected:
		// returns a error message on error. Empty optional otherwise.
		[[nodiscard]]
		std::optional<std::string> checkErr() {
			GLint success = 0;
			glGetShaderiv(id, GL_COMPILE_STATUS, &success);

			if (!success) {
				int logLen = 0;
				glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);

				std::string errMsg{};
				errMsg.resize(logLen);
				glGetShaderInfoLog(id, logLen, NULL, errMsg.data());
				destroy();
				return errMsg;
			}

			return std::nullopt;
		}
	};

	struct ShaderProgram {
		GLuint id;

		mutable std::unordered_map<std::string, GLint> uniformCache;

		explicit ShaderProgram() :
			id(0), uniformCache{} {}

		ShaderProgram(ShaderProgram&& other) noexcept :
			id(std::exchange(other.id, 0)), uniformCache(std::exchange(other.uniformCache, {})) {}

		ShaderProgram& operator=(ShaderProgram&& other) noexcept {
			std::swap(id, other.id);
			std::swap(uniformCache, other.uniformCache);
			return *this;
		}

		template <class T>
		explicit ShaderProgram(T& shaders) :
			id(glCreateProgram()), uniformCache{} {

			for (auto shader : shaders) {
				glAttachShader(id, shader.id);
			}

			glLinkProgram(id);

			auto err = checkErr();

			for (auto shader : shaders) {
				glDetachShader(id, shader.id);
			}

			if (err.has_value()) [[unlikely]] {
				throw std::runtime_error(err.value());
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
		GLint getUniformLocation(const std::string& uid) const {
			auto it = uniformCache.find(uid);
			if (it != uniformCache.end()) [[likely]] {
				return it->second;
			}

			auto ret = glGetUniformLocation(id, uid.c_str());
			uniformCache.emplace(uid, ret);
			return ret;
		}


		void uniform(const std::string& uid, int data) const {
			glUniform1i(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, unsigned int data) const {
			glUniform1ui(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, float data) const {
			glUniform1f(getUniformLocation(uid), data);
		}

		void uniform(const std::string& uid, glm::vec2 data) const {
			glUniform2f(getUniformLocation(uid), data.x, data.y);
		}

		void uniform(const std::string& uid, glm::vec3 data) const {
			glUniform3f(getUniformLocation(uid), data.x, data.y, data.z);
		}

		void uniform(const std::string& uid, glm::vec4 data) const {
			glUniform4f(getUniformLocation(uid), data.x, data.y, data.z, data.w);
		}

		void uniform(const std::string& uid, glm::mat2 data) const {
			glUniformMatrix2fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}

		void uniform(const std::string& uid, glm::mat3 data) const {
			glUniformMatrix3fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}

		void uniform(const std::string& uid, glm::mat4 data) const {
			glUniformMatrix4fv(getUniformLocation(uid), 1, false, glm::value_ptr(data));
		}

	protected:
		// returns a error message on error. Empty optional otherwise.
		[[nodiscard]]
		std::optional<std::string> checkErr() {
			GLint success = 0;
			glGetProgramiv(id, GL_LINK_STATUS, &success);

			if (!success) {
				int logLen = 0;
				glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLen);

				std::string errMsg{};
				errMsg.resize(logLen);
				glGetProgramInfoLog(id, logLen, NULL, errMsg.data());
				destroy();
				return errMsg;
			}

			return std::nullopt;
		}
	};

	[[nodiscard]]
	inline std::optional<Shader> shaderFromFile(const std::filesystem::path& path, ShaderType type) {
		std::ifstream file(path);

		if (file.is_open()) {
			std::stringstream strstr;
			strstr << file.rdbuf();

			return Shader(strstr.str().c_str(), type);
		}

		return {};
	}

	[[nodiscard]]
	inline std::optional<Shader> shaderFromFile(const std::filesystem::path& path) {
		auto Ftype = determineShaderType(path);

		return shaderFromFile(path, Ftype);
	}

	template <class Iterable> [[nodiscard]]
	inline std::optional<ShaderProgram> programFromFiles(const Iterable& files) {
		std::vector<Shader> shaders{};
		shaders.resize(files.size());

		size_t i = 0;
		for (auto&& file : files) {
			auto shdr = shaderFromFile(file);
			if (shdr) [[likely]] {
				shaders[i] = std::move(shdr.value());
			}
			else {
				for (; i > 0; --i) {
					shaders[i - 1].destroy();
				}
				return {};
			}

			++i;
		}

		auto out = ShaderProgram(shaders);

		for (auto& shader : shaders) {
			shader.destroy();
		}

		return out;
	}

	template <class... Files> [[nodiscard]]
	inline std::optional<ShaderProgram> programFromFiles(Files&&... files) {

		std::array<Shader, sizeof...(files)> shaders{};

		size_t i = 0;
		for (auto&& file : { files... }) {
			auto shdr = shaderFromFile(file);
			if (shdr) [[likely]] {
				shaders[i] = std::move(shdr.value());
			}
			else {
				for (; i > 0; --i) {
					shaders[i - 1].destroy();
				}
				return {};
			}

			++i;
		}

		auto out = ShaderProgram(shaders);

		for (auto& shader : shaders) {
			shader.destroy();
		}

		return out;
	}
}