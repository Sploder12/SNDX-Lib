#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <optional>

#include "mesh.hpp"

namespace sndx {

	template <class VertexT, class... VertexLayout>
	struct Model {
		std::vector<Mesh<VertexT, VertexLayout...>> meshes;
	};
	
	[[nodiscard]]
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type) {
		std::vector<Texture> textures;
		for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
		{
			aiString str;
			material->GetTexture(type, i, &str);
			textures.emplace_back(textureFromFile(str.C_Str()));
		}
		return textures;
	}

	// https://learnopengl.com/Model-Loading/Model
	[[nodiscard]]
	Mesh<Vertex, glm::vec3, glm::vec3, glm::vec2> processMesh(aiMesh* src, const aiScene* scene) {
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
			out.textures = loadMaterialTextures(material, aiTextureType_DIFFUSE);
			auto spec = loadMaterialTextures(material, aiTextureType_DIFFUSE);
			out.textures.insert(out.textures.end(), spec.begin(), spec.end());
		}

		return out;
	}

	void processNode(aiNode* cur, const aiScene* scene, Model<Vertex, glm::vec3, glm::vec3, glm::vec2>& target) {
		
		for (unsigned int i = 0; i < cur->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[cur->mMeshes[i]];
			target.meshes.emplace_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < cur->mNumChildren; ++i) {
			processNode(cur->mChildren[i], scene, target);
		}
	}

	[[nodiscard]]
	std::optional<Model<Vertex, glm::vec3, glm::vec3, glm::vec2>> loadModelFromFile(const char* path) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (scene == nullptr) return {};
		if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) return {};
		if (scene->mRootNode == nullptr) return {};

		Model<Vertex, glm::vec3, glm::vec3, glm::vec2> out{};
		processNode(scene->mRootNode, scene, out);
		return out;
	}
}