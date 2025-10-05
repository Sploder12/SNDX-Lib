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
	};

	struct RIFFheader {
		static constexpr std::array<char, 4> ID = { 'R', 'I', 'F', 'F' };

		uint32_t size = 0;
		std::array<char, 4> type{};

		explicit RIFFheader() = default;
		explicit RIFFheader(std::array<char, 4> id) : type(id) {}

		[[nodiscard]]
		uint32_t getLength() const {
			return sizeof(type);
		}
	};

	struct Chunk {
		virtual ~Chunk() = default;

		// ex: `static std::array<char, 4> ID = ~;`

		virtual void deserialize(const std::vector<uint8_t>& data) = 0;
		virtual std::vector<uint8_t> serialize() const = 0;
		virtual uint32_t getLength() const = 0;

		using Factory = std::unique_ptr<Chunk> (*)(const std::vector<uint8_t>&, const ChunkHeader& header);

		[[nodiscard]]
		static std::unique_ptr<Chunk> create(const std::vector<uint8_t>& data, const ChunkHeader& header) {
			uint32_t rawID = idToRawID(header.id);

			const auto& map = getChunkMap();

			if (auto it = map.find(rawID); it != map.end()) {
				return it->second(data, header);
			}

			return nullptr;
		}

		template <std::derived_from<Chunk> T>
		static void registerChunkType() {
			auto& map = getChunkMap();

			map.emplace(idToRawID(T::ID), [](const std::vector<uint8_t>& data, const ChunkHeader& header) {
				std::unique_ptr<T> chunk = std::make_unique<T>(header);
				chunk->deserialize(data);
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
		RIFFheader m_header{};
		std::unordered_map<uint32_t, std::unique_ptr<Chunk>> m_chunks{};

	public:
		explicit File() = default;

		explicit File(std::array<char, 4> id) :
			m_header(id) {}

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
		
		template <class InputIt>
		void deserialize(InputIt& in, InputIt end) {
			deserializeFromAdjust(m_header, in, end);
			deserializeRest(in, end);
		}

		template <class InputIt>
		void deserialize(InputIt& in, InputIt end, std::array<char, 4> checkID) {
			deserializeFromAdjust(m_header, in, end);
			if (m_header.type != checkID)
				throw bad_field_error("RIFF description identifier mismatch");
			
			deserializeRest(in, end);
		}

		template <class SerializeIt>
		void serialize(SerializeIt& it) const {
			RIFFheader tmp = m_header;

			tmp.size = sizeof(tmp.type);
			for (const auto& [id, chunk] : m_chunks) {
				tmp.size += chunk->getLength();
			}

			serializeToAdjust(it, m_header);

			for (const auto& [id, chunk] : m_chunks) {
				const auto& data = chunk->serialize();
				for (auto b : data) {
					serializeToAdjust(it, b);
				}
			}
		}

	private:
		template <class InputIt>
		void deserializeRest(InputIt& in, InputIt end) {
			size_t read = 8;
			while (read < m_header.size) {
				ChunkHeader header;
				deserializeFromAdjust(header, in, end);

				std::vector<uint8_t> buffer{};
				buffer.reserve(header.size);

				for (size_t i = 0; i < header.size; ++i) {
					uint8_t b;
					deserializeFromAdjust(b, in, end);
					buffer.emplace_back(b);
				}

				auto ptr = Chunk::create(buffer, header);

				if (!ptr) {
					read += header.size + sizeof(header.id) + sizeof(header.size);
					continue;
				}

				read += ptr->getLength();

				m_chunks.emplace(idToRawID(header.id), std::move(ptr));
			}
		}
	};
}

namespace sndx {
	template<>
	struct Serializer<RIFF::ChunkHeader> {
		template <class SerializeIt>
		constexpr void serialize(const RIFF::ChunkHeader& value, SerializeIt& it) const {
			serializeToAdjust(it, value.id);
			serializeToAdjust(it, value.size);
		}
	};

	template<>
	struct Deserializer<RIFF::ChunkHeader> {
		template <class DeserializeIt>
		constexpr void deserialize(RIFF::ChunkHeader& to, DeserializeIt& in, DeserializeIt end) const {
			deserializeFromAdjust(to.id, in, end);
			deserializeFromAdjust(to.size, in, end);
		}
	};

	template<>
	struct Serializer<RIFF::RIFFheader> {
		template <class SerializeIt>
		constexpr void serialize(const RIFF::RIFFheader& value, SerializeIt& it) const {
			serializeToAdjust(it, RIFF::RIFFheader::ID);
			serializeToAdjust(it, value.size);
			serializeToAdjust(it, value.type);
		}
	};

	template<>
	struct Deserializer<RIFF::RIFFheader> {
		template <class DeserializeIt>
		constexpr void deserialize(RIFF::RIFFheader& to, DeserializeIt& in, DeserializeIt end) const {
			deserializeFromAdjust(to.type, in, end);
			if (to.type != RIFF::RIFFheader::ID)
				throw bad_field_error{ "RIFF not present in RIFF header" };

			deserializeFromAdjust(to.size, in, end);
			deserializeFromAdjust(to.type, in, end);
		}
	};

	template<>
	struct Serializer<RIFF::File> {
		template <class SerializeIt>
		constexpr void serialize(const RIFF::File& value, SerializeIt& it) const {
			value.serialize(it);
		}
	};

	template<>
	struct Deserializer<RIFF::File> {
		template <class DeserializeIt>
		constexpr void deserialize(RIFF::File& to, DeserializeIt& in, DeserializeIt end) const {
			to.deserialize(in, end);
		}
	};
}