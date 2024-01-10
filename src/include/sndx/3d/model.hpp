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

		[[nodiscard]]
		AABB getBounds() const {
			if (meshes.empty()) return AABB{ glm::vec3(0.0f), glm::vec3(0.0f) };

			AABB out = meshes.front().getBounds();

			for (const auto& mesh : meshes) {
				auto tmp = mesh.getBounds();
				if (tmp.surfaceArea() != 0.0) out = out.merge(tmp);
			}

			for (const auto& child : children) {
				auto tmp = child.getBounds();
				if (tmp.surfaceArea() != 0.0) out = out.merge(tmp);
			}

			return out;
		}

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

		[[nodiscard]]
		AABB getBounds() const {
			return root.getBounds();
		}

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
	inline std::vector<Texture> loadMaterialTextures(const aiScene* scene, aiMaterial* material, aiTextureType type, ModelT& target, const std::string path = "") {
		std::vector<Texture> textures{};
		auto texSize = material->GetTextureCount(type);
		textures.reserve(texSize);

		for (unsigned int i = 0; i < texSize; i++) {
			aiString str;
			if (material->GetTexture(type, i, &str) == AI_SUCCESS) {
				if (str.length > 0 && str.C_Str()[0] == '*') {
					// embedded textures
					// note: not put into loadedTextures map

					auto idx = std::atoi(str.C_Str() + 1);
					const auto& tex = scene->mTextures[idx];
					
					// raw data textures
					if (tex->mHeight > 0) {
						ImageData data;
						data.width = tex->mWidth;
						data.height = tex->mHeight;
						data.channels = 4;
						data.data.reserve(data.width * data.height * data.channels);

						for (int t = 0; t < data.width * data.height; ++t) {
							const auto& curTex = tex->pcData[t];

							// this does reorder the data from ARGB to RGBA
							data.data.emplace_back(curTex.r);
							data.data.emplace_back(curTex.g);
							data.data.emplace_back(curTex.b);
							data.data.emplace_back(curTex.a);
						}

						textures.emplace_back(textureFromImage(std::move(data), GL_RGBA));
					}
					else { 
						// file-like embedded textures are not currently supported :(
						throw std::runtime_error("Models with file-like embedded textures are not supported");
					}
				}
				else {
					auto tmp = path + str.C_Str();

					auto it = target.loadedTextures.find(tmp);
					if (it != target.loadedTextures.end()) {
						textures.emplace_back(target.textures[it->second]);
					}
					else {
						auto tex = textureFromFile(tmp.c_str(), 0);
						if (tex.has_value()) {
							target.textures.emplace_back(tex.value());
							textures.emplace_back(target.textures.back());
							target.loadedTextures.insert({ tmp, target.textures.size() - 1 });
						}
						else {
							// @TODO some failure indicator
							throw std::runtime_error("Couldn't load texture");
						}
					}
				}
			}
		}
		return textures;
	}

	// https://learnopengl.com/Model-Loading/Model
	[[nodiscard]]
	inline Mesh<Vertex, glm::vec3, glm::vec3, glm::vec2> processMesh(aiMesh* src, const aiScene* scene, ModelT& target, const std::string path = "") {
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

		if (src->HasBones()) {
			out.bones.reserve(src->mNumBones);
			for (unsigned int i = 0; i < src->mNumBones; ++i) {
				const auto& cur = src->mBones[i];

				Bone bone{};
				bone.reserve(cur->mNumWeights);

				for (unsigned int j = 0; j < cur->mNumWeights; ++j) {
					bone.emplace_back(cur->mWeights[j].mVertexId, cur->mWeights[j].mWeight);
				}

				out.bones.emplace(cur->mName.data, std::move(bone));
			}
		}

		if (src->HasFaces()) [[likely]] {
			out.indices.reserve(src->mNumFaces * 10);
			for (unsigned int i = 0; i < src->mNumFaces; ++i) {
				const auto& face = src->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; ++j) {
					out.indices.emplace_back(face.mIndices[j]);
				}
			}
		}
		
		if (src->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[src->mMaterialIndex];
			out.textures = loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, target, path);
			auto spec = loadMaterialTextures(scene, material, aiTextureType_SPECULAR, target, path);
			out.textures.insert(out.textures.end(), spec.begin(), spec.end());
		}

		return out;
	}

	inline void processNode(aiNode* cur, ModelNodeT& curNode, const aiScene* scene, ModelT& target, const std::string path = "") {
		
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
	inline std::optional<ModelT> loadModelFromFile(const std::filesystem::path& path) {
		Assimp::Importer importer;
		importer.SetPropertyBool("#AI_CONFIG_IMPORT_FBX_EMBEDDED_TEXTURES_LEGACY_NAMING", true); // for embedded textures
		const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

		if (scene == nullptr) return {};
		if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) return {};
		if (scene->mRootNode == nullptr) return {};

		ModelT out{};
		processNode(scene->mRootNode, out.root, scene, out, path.parent_path().string() + '/');
		return out;
	}
}