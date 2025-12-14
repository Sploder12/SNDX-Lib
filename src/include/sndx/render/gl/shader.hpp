#pragma once

#define NOMINMAX
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <array>
#include <vector>
#include <sstream>

namespace sndx::render {
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

	class Shader {
	private:
		GLuint m_id{0};

		friend class ShaderProgram;

	public:
		constexpr Shader() noexcept = default;
		
		constexpr Shader(Shader&& other) noexcept:
			m_id(std::exchange(other.m_id, 0)) {}

		Shader(const Shader&) = delete;

		Shader& operator=(Shader&& other) noexcept {
			std::swap(m_id, other.m_id);
			return *this;
		}

		Shader& operator=(const Shader&) = delete;

		~Shader() noexcept {
			destroy();
		}

		Shader(const char* code, ShaderType type) :
			m_id(glCreateShader((GLenum)type)) {

			glShaderSource(m_id, 1, &code, nullptr);
			glCompileShader(m_id);

			auto err = checkErr();
			if (err.has_value()) [[unlikely]] {
				throw std::runtime_error(err.value());
			}
		}

		void destroy() {
			if (m_id != 0) {
				glDeleteShader(std::exchange(m_id, 0));
			}
		}

	protected:
		// returns a error message on error. Empty optional otherwise.
		[[nodiscard]]
		std::optional<std::string> checkErr() {
			GLint success = 0;
			glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);

			if (!success) {
				int logLen = 0;
				glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &logLen);

				std::string errMsg{};
				errMsg.resize(logLen);
				glGetShaderInfoLog(m_id, logLen, NULL, errMsg.data());
				destroy();
				return errMsg;
			}

			return std::nullopt;
		}
	};

	class ShaderProgram {
	private:
		mutable std::unordered_map<std::string, GLint> uniformCache{};

		GLuint m_id{0};

		void destroy() {
			glDeleteProgram(std::exchange(m_id, 0));
		}

	public:

		explicit ShaderProgram() = default;

		~ShaderProgram() noexcept {
			destroy();
		}

		ShaderProgram(ShaderProgram&& other) noexcept :
			m_id(std::exchange(other.m_id, 0)), uniformCache(std::exchange(other.uniformCache, {})) {
		}

		ShaderProgram& operator=(ShaderProgram&& other) noexcept {
			std::swap(m_id, other.m_id);
			std::swap(uniformCache, other.uniformCache);
			return *this;
		}

		template <class T>
		explicit ShaderProgram(T& shaders) :
			m_id(glCreateProgram()), uniformCache{} {

			for (const auto& shader : shaders) {
				glAttachShader(m_id, shader.m_id);
			}

			glLinkProgram(m_id);

			auto err = checkErr();

			for (const auto& shader : shaders) {
				glDetachShader(m_id, shader.m_id);
			}

			if (err.has_value()) [[unlikely]] {
				throw std::runtime_error(err.value());
			}
		}

		void use() const {
			glUseProgram(m_id);
		}

		[[nodiscard]]
		GLint getUniformLocation(const std::string& uid) const {
			auto it = uniformCache.find(uid);
			if (it != uniformCache.end()) [[likely]] {
				return it->second;
			}

			auto ret = glGetUniformLocation(m_id, uid.c_str());
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
			glGetProgramiv(m_id, GL_LINK_STATUS, &success);

			if (!success) {
				int logLen = 0;
				glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLen);

				std::string errMsg{};
				errMsg.resize(logLen);
				glGetProgramInfoLog(m_id, logLen, NULL, errMsg.data());
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
				return {};
			}

			++i;
		}

		auto out = ShaderProgram(shaders);
		return out;
	}
}