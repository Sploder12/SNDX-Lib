#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <functional>

#include "mesh.hpp"

namespace sndx {

	template <class VertexT, class... VertexLayout>
	struct ModelNode {
		using MeshT = Mesh<VertexT, VertexLayout...>;
		std::vector<MeshT> meshes{};
		std::vector<ModelNode<VertexT, VertexLayout...>> children{};

		void destroy() {
			for (auto& mesh : meshes) {
				mesh.destroy();
			}

			for (auto& child : children) {
				child.destroy();
			}

			meshes.clear();
			children.clear();
		}

		void onEachMesh(std::function<void(MeshT&)> func) {
			for (auto& mesh : meshes) {
				func(mesh);
			}

			for (auto& child : children) {
				child.onEachMesh(func);
			}
		}
	};

	template <class VertexT, class... VertexLayout>
	struct Model {
		ModelNode<VertexT, VertexLayout...> root;

		std::vector<Texture> textures{};
		std::unordered_map<std::string, size_t> loadedTextures{};

		void destroy() {
			loadedTextures.clear();
			
			root.destroy();

			for (auto& texture : textures) {
				texture.destroy();
			}

			textures.clear();
		}

		void onEachMesh(std::function<void(Mesh<VertexT, VertexLayout...>&)> func) {
			root.onEachMesh(func);
		}
	};

	using ModelT = Model<Vertex, glm::vec3, glm::vec3, glm::vec2>;
	using ModelNodeT = ModelNode<Vertex, glm::vec3, glm::vec3, glm::vec2>;
	
	[[nodiscard]]
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, Model<Vertex, glm::vec3, glm::vec3, glm::vec2>& target, const std::string path = "") {
		std::vector<Texture> textures;
		for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
		{
			aiString str;
			material->GetTexture(type, i, &str);
			auto tmp = path + str.C_Str();

			auto it = target.loadedTextures.find(tmp);
			if (it != target.loadedTextures.end()) {
				textures.emplace_back(target.textures[it->second]);
			}
			else {
				target.textures.emplace_back(textureFromFile(tmp.c_str()));
				textures.emplace_back(target.textures.back());
				target.loadedTextures.insert({tmp, target.textures.size() - 1 });
			}
		}
		return textures;
	}

	// https://learnopengl.com/Model-Loading/Model
	[[nodiscard]]
	Mesh<Vertex, glm::vec3, glm::vec3, glm::vec2> processMesh(aiMesh* src, const aiScene* scene, Model<Vertex, glm::vec3, glm::vec3, glm::vec2>& target, const std::string path = "") {
		Mesh<Vertex, glm::vec3, glm::vec3, glm::vec2> out{};

		out.vertices.reserve(src->mNumVertices);
		for (unsigned int i = 0; i < src->mNumVertices; ++i) {
			Vertex v{};

			const auto& curPos = src->mVertices[i];
			const auto& curNorm = src->mNormals[i];

			v.pos = { curPos.x, curPos.y, curPos.z };
			v.normal = { curNorm.x, curNorm.y, curNorm.z };

			if (src->mTextureCoords[0] != nullptr) {
				const auto& curTC = src->mTextureCoords[0][i];
				v.texCoords = { curTC.x, curTC.y };
			}

			out.vertices.emplace_back(std::move(v));
		}

		out.indices.reserve(src->mNumFaces * 10);
		for (unsigned int i = 0; i < src->mNumFaces; ++i) {
			const auto& face = src->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; ++j) {
				out.indices.emplace_back(face.mIndices[j]);
			}
		}

		if (src->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[src->mMaterialIndex];
			out.textures = loadMaterialTextures(material, aiTextureType_DIFFUSE, target, path);
			auto spec = loadMaterialTextures(material, aiTextureType_DIFFUSE, target, path);
			out.textures.insert(out.textures.end(), spec.begin(), spec.end());
		}

		return out;
	}

	void processNode(aiNode* cur, ModelNodeT& curNode, const aiScene* scene, ModelT& target, const std::string path = "") {
		
		for (unsigned int i = 0; i < cur->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[cur->mMeshes[i]];
			curNode.meshes.emplace_back(processMesh(mesh, scene, target, path));

			curNode.meshes.back().gen();
		}

		for (unsigned int i = 0; i < cur->mNumChildren; ++i) {
			curNode.children.emplace_back();
			processNode(cur->mChildren[i], curNode.children.back(), scene, target, path);
		}
	}

	[[nodiscard]]
	std::optional<ModelT> loadModelFromFile(std::filesystem::path path) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

		if (scene == nullptr) return {};
		if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) return {};
		if (scene->mRootNode == nullptr) return {};

		ModelT out{};
		processNode(scene->mRootNode, out.root, scene, out, path.parent_path().string() + '/');
		return out;
	}
}