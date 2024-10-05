#pragma once

#include <array>
#include <unordered_map>
#include <concepts>
#include <cstdint>
#include <cstring>

#include "../utility/endian.hpp"

#include "./serialize.hpp"

namespace sndx::RIFF {

	[[nodiscard]]
	inline uint32_t idToRawID(std::array<char, 4> id) {
		uint32_t out;
		std::memcpy(&out, id.data(), sizeof(out));

		return out;
		static_assert(sizeof(out) == id.size());
	}

	struct ChunkHeader {
		std::array<char, 4> id = { 0 };
		uint32_t size = 0;

		void deserialize(sndx::serialize::Deserializer& deserializer) {
			deserializer.deserialize(id.data(), sizeof(id));
			deserializer.deserialize<std::endian::little>(size);
		}

		void serialize(sndx::serialize::Serializer& serializer) const {
			serializer.serialize(id.data(), sizeof(id));
			serializer.serialize<std::endian::little>(size);
		}
	};

	struct RIFFheader {
		static constexpr std::array<char, 4> ID = { 'R', 'I', 'F', 'F' };

		uint32_t size = 0;
		std::array<char, 4> type{};

		explicit RIFFheader() = default;
		explicit RIFFheader(std::array<char, 4> id) :
			size(0), type(id) {}

		void deserialize(sndx::serialize::Deserializer& deserializer) {
			deserializer.deserialize(type.data(), sizeof(type));
			if (type != ID)
				throw identifier_error("RIFF not present in RIFF header");

			deserializer.deserialize<std::endian::little>(size);
			deserializer.deserialize(type.data(), sizeof(type));
		}

		void serialize(sndx::serialize::Serializer& serializer) const {
			serializer.serialize("RIFF", 4);
			serializer.serialize<std::endian::little>(size);
			serializer.serialize(type.data(), sizeof(type));
		}

		[[nodiscard]]
		uint32_t getLength() const {
			return sizeof(type);
		}
	};

	struct Chunk {
		virtual ~Chunk() = default;

		// static std::array<char, 4> ID = ~;

		virtual void deserialize(sndx::serialize::Deserializer&) = 0;
		virtual void serialize(sndx::serialize::Serializer& serializer) const = 0;
		virtual uint32_t getLength() const = 0;

		typedef std::unique_ptr<Chunk>(*Factory)(sndx::serialize::Deserializer&, const ChunkHeader&);

		[[nodiscard]]
		static std::unique_ptr<Chunk> create(sndx::serialize::Deserializer& deserializer, const ChunkHeader& header) {
			uint32_t rawID = idToRawID(header.id);

			const auto& map = getChunkMap();

			if (auto it = map.find(rawID); it != map.end()) {
				return it->second(deserializer, header);
			}

			return nullptr;
		}

		template <std::derived_from<Chunk> T>
		static void registerChunkType() {
			auto& map = getChunkMap();

			map.emplace(idToRawID(T::ID), [](sndx::serialize::Deserializer& deserializer, const ChunkHeader& header) {
				std::unique_ptr<T> chunk = std::make_unique<T>(header);
				deserializer.deserialize(*chunk);
				return std::unique_ptr<Chunk>(std::move(chunk));
			});
		}

	private:
		static std::unordered_map<uint32_t, Factory>& getChunkMap() {
			static std::unordered_map<uint32_t, Factory> map{};
			return map;
		}
	};

	class File {
	private:
		RIFFheader m_header{};

		std::unordered_map<uint32_t, std::unique_ptr<Chunk>> m_chunks{};

	public:
		explicit File() = default;

		explicit File(std::array<char, 4> id) :
			m_header(id), m_chunks{} {}

		[[nodiscard]]
		const RIFFheader& getHeader() const noexcept {
			return m_header;
		}

		[[nodiscard]]
		Chunk* getChunk(std::array<char, 4> id) {
			uint32_t rawID = idToRawID(id);

			if (auto it = m_chunks.find(rawID); it != m_chunks.end()) {
				return it->second.get();
			}

			return nullptr;
		}

		template <std::derived_from<Chunk> T> [[nodiscard]]
		T* getChunk() {
			return static_cast<T*>(getChunk(T::ID));
		}

		template <std::derived_from<Chunk> T>
		decltype(auto) emplaceChunk(const T& chunk) {
			return m_chunks.emplace(idToRawID(T::ID), static_cast<Chunk>(std::make_unique<T>(chunk)));
		}

		[[nodiscard]]
		const auto& getChunks() const noexcept {
			return m_chunks;
		}

		void deserialize(sndx::serialize::Deserializer& deserializer) {
			deserializer.deserialize(m_header);
			deserializeRest(deserializer);
		}

		void deserialize(sndx::serialize::Deserializer& deserializer, std::array<char, 4> checkID) {
			deserializer.deserialize(m_header);

			if (m_header.type != checkID)
				throw identifier_error("RIFF description identifier mismatch");
			
			deserializeRest(deserializer);
		}

		void serialize(sndx::serialize::Serializer& serializer) const {
			RIFFheader tmp = m_header;

			tmp.size = sizeof(tmp.type);
			for (const auto& [id, chunk] : m_chunks) {
				tmp.size += chunk->getLength();
			}

			serializer.serialize(tmp);

			for (const auto& [id, chunk] : m_chunks) {
				serializer.serialize(*chunk);
			}
		}

	private:
		void deserializeRest(sndx::serialize::Deserializer& deserializer) {
			size_t read = 8;
			bool seekable = true;

			while (read < m_header.size && deserializer.m_source.good()) {
				ChunkHeader header;
				deserializer.deserialize(header);

				auto ptr = Chunk::create(deserializer, header);
				if (!ptr) {
					seekable = deserializer.discard(header.size, seekable);
					read += header.size + sizeof(header.id) + sizeof(header.size);
					continue;
				}

				read += ptr->getLength();

				m_chunks.emplace(idToRawID(header.id), std::move(ptr));
			}
		}
	};

	// requires that the decoder is seekable!
	class LazyFile {
	public:
		struct LazyChunk {
			ChunkHeader header{};
			std::ios::pos_type streamPos = -1;
			std::unique_ptr<Chunk> data = nullptr;

			[[nodiscard]]
			constexpr uint32_t getLength() const {
				return sizeof(header.id) + sizeof(header.size) + header.size;
			}
		};

	private:
		RIFFheader m_header{};

		std::unordered_map<uint32_t, LazyChunk> m_chunks{};

	public:
		void deserialize(sndx::serialize::Deserializer& deserializer) {
			deserializer.deserialize(m_header);
			deserializeRest(deserializer);
		}

		void deserialize(sndx::serialize::Deserializer& deserializer, std::array<char, 4> checkID) {
			deserializer.deserialize(m_header);

			if (m_header.type != checkID)
				throw identifier_error("RIFF description identifier mismatch");

			deserializeRest(deserializer);
		}

		[[nodiscard]]
		Chunk* getChunk(std::array<char, 4> id, sndx::serialize::Deserializer& deserializer) {
			uint32_t rawID = idToRawID(id);

			if (auto it = m_chunks.find(rawID); it != m_chunks.end()) {
				auto& chunk = it->second;

				if (chunk.data)
					return chunk.data.get();

				deserializer.m_source.seekg(chunk.streamPos);
				
				chunk.data = Chunk::create(deserializer, chunk.header);
				return chunk.data.get();
			}

			return nullptr;
		}

		template <std::derived_from<Chunk> T> [[nodiscard]]
		T* getChunk(sndx::serialize::Deserializer& deserializer) {
			return static_cast<T*>(getChunk(T::ID, deserializer));
		}

	private:
		void deserializeRest(sndx::serialize::Deserializer& deserializer) {
			size_t read = 8;
			bool seekable = true;

			while (read < m_header.size && deserializer.m_source.good()) {
				LazyChunk chunk;
				deserializer.deserialize(chunk.header);

				chunk.streamPos = deserializer.m_source.tellg();

				seekable = deserializer.discard(chunk.header.size, seekable);

				read += chunk.getLength();

				m_chunks.emplace(idToRawID(chunk.header.id), std::move(chunk));
			}
		}
	};
}