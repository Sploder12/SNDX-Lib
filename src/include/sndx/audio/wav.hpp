#pragma once

#include "./audio_decoder.hpp"

#include "../utility/endian.hpp"
#include "../data/RIFF.hpp"

#include <iostream>
#include <variant>
#include <array>
#include <concepts>

namespace sndx::audio {

	static constexpr uint16_t WAVE_PCM_INT = 1;
	static constexpr uint16_t WAVE_IEE_FLOAT = 3;
	static constexpr uint16_t WAVE_A_LAW = 6;
	static constexpr uint16_t WAVE_MU_LAW = 7;
	static constexpr uint16_t WAVE_EXTENSIBLE = 0xFFFE;

	struct FMTchunk : public sndx::RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'f', 'm', 't', ' ' };

		struct ExtendedNone {
			void deserialize(sndx::serialize::Deserializer&) {};
			void serialize(sndx::serialize::Serializer&) const {};

			static constexpr uint32_t size() noexcept {
				return 0 + 16;
			}
		};

		struct Extended0 {
			void deserialize(sndx::serialize::Deserializer& deserializer) {
				uint16_t size;
				deserializer.deserialize<std::endian::little>(size);

				if (size != 0)
					throw deserialize_error("Extended0 didn't have size 0");
			};

			void serialize(sndx::serialize::Serializer& serializer) const noexcept {
				serializer.serialize<std::endian::little>(static_cast<uint16_t>(0));
			};

			static constexpr uint32_t size() noexcept {
				return 0 + sizeof(uint16_t) + 16;
			}
		};

		struct Extended {
			uint16_t validBitsPerSample = 0;
			uint32_t channelMask = 0;
			char guid[16] = { 0 };

			static constexpr uint16_t dataSize = sizeof(validBitsPerSample) + sizeof(channelMask) + sizeof(guid);

			void deserialize(sndx::serialize::Deserializer& deserializer) {
				uint16_t size;
				deserializer.deserialize<std::endian::little>(size);

				if (size != dataSize)
					throw deserialize_error("Extended format didn't have size 22");

				deserializer.deserialize<std::endian::little>(validBitsPerSample);
				deserializer.deserialize<std::endian::little>(channelMask);
				deserializer.deserialize(guid, sizeof(guid));
			};

			void serialize(sndx::serialize::Serializer& serializer) const {
				serializer.serialize<std::endian::little>(dataSize);

				serializer.serialize<std::endian::little>(validBitsPerSample);
				serializer.serialize<std::endian::little>(channelMask);
				serializer.serialize(guid, sizeof(guid));
			};

			static constexpr uint32_t size() noexcept {
				return dataSize + sizeof(uint16_t) + 16;
			}
		};

		uint16_t format = 0;
		uint16_t channels = 0;
		uint32_t sampleRate = 0;
		uint32_t byteRate = 0;
		uint16_t blockAlign = 0;
		uint16_t bitDepth = 0;

		std::variant<ExtendedNone, Extended0, Extended> ext = ExtendedNone();

		explicit constexpr FMTchunk() = default;

		explicit constexpr FMTchunk(const sndx::RIFF::ChunkHeader& header) {
			switch (header.size) {
			case ExtendedNone::size():
				ext = ExtendedNone();
				break;
			case Extended0::size():
				ext = Extended0();
				break;
			case Extended::size():
				ext = Extended();
				break;
			default:
				throw deserialize_error("Invalid fmt  size");
			}
		}
		
		[[nodiscard]]
		constexpr uint16_t getSampleSize() const {
			return blockAlign / channels;
		}

		void deserialize(sndx::serialize::Deserializer& deserializer) override {
			deserializer.deserialize<std::endian::little>(format);
			deserializer.deserialize<std::endian::little>(channels);
			deserializer.deserialize<std::endian::little>(sampleRate);
			deserializer.deserialize<std::endian::little>(byteRate);
			deserializer.deserialize<std::endian::little>(blockAlign);
			deserializer.deserialize<std::endian::little>(bitDepth);

			std::visit([&deserializer](auto&& arg) {
				deserializer.deserialize(arg);
			}, ext);
		};

		void serialize(sndx::serialize::Serializer& serializer) const override {
			serializer.serialize("fmt ", 4);
		
			std::visit([&serializer](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;

				serializer.serialize<std::endian::little>(static_cast<uint32_t>(T::size()));
			}, ext);
				
			serializer.serialize<std::endian::little>(format);
			serializer.serialize<std::endian::little>(channels);
			serializer.serialize<std::endian::little>(sampleRate);
			serializer.serialize<std::endian::little>(byteRate);
			serializer.serialize<std::endian::little>(blockAlign);
			serializer.serialize<std::endian::little>(bitDepth);

			std::visit([&serializer](auto&& arg) {
				serializer.serialize(arg);
			}, ext);
		};

		[[nodiscard]]
		constexpr uint32_t getLength() const override {
			uint32_t size = 4 + sizeof(uint32_t);

			std::visit([&size](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;

				size += static_cast<uint32_t>(T::size());
			}, ext);

			return size;
		}
	};

	struct FACTchunk : public sndx::RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'f', 'a', 'c', 't' };

		uint32_t sampleLength = 0;

		explicit constexpr FACTchunk() = default;

		explicit constexpr FACTchunk(const sndx::RIFF::ChunkHeader&) noexcept:
			sampleLength(0) {};

		void deserialize(sndx::serialize::Deserializer& deserializer) override {
			deserializer.deserialize<std::endian::little>(sampleLength);
		}

		void serialize(sndx::serialize::Serializer& serializer) const override {
			serializer.serialize("fact", 4);
			serializer.serialize<std::endian::little>(static_cast<uint32_t>(sizeof(sampleLength)));

			serializer.serialize<std::endian::little>(sampleLength);
		}

		[[nodiscard]]
		constexpr uint32_t getLength() const noexcept override {
			return uint32_t(sizeof(ID) + sizeof(uint32_t) + sizeof(sampleLength));
		}
	};

	struct DATAchunk : public sndx::RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'd', 'a', 't', 'a' };

		std::vector<uint8_t> data{};

		explicit constexpr DATAchunk() = default;

		explicit constexpr DATAchunk(const sndx::RIFF::ChunkHeader& header) {
			data.resize(header.size);
		}

		void deserialize(sndx::serialize::Deserializer& deserializer) override {
			deserializer.deserialize(data.data(), data.size());

			uint8_t padding = 0;
			if (data.size() % 2 == 1)
				deserializer.deserialize(padding);
		}

		void serialize(sndx::serialize::Serializer& serializer) const override {
			serializer.serialize("data", 4);
			serializer.serialize<std::endian::little>(static_cast<uint32_t>(data.size()));

			for (const auto& b : data) {
				serializer.serialize(b);
			}

			if (data.size() % 2 == 1) {
				serializer.serialize(static_cast<uint8_t>(0));
			}
		}

		[[nodiscard]]
		constexpr uint32_t getLength() const override {
			return uint32_t(4 + sizeof(uint32_t) + data.size() + (data.size() % 2 == 1));
		}
	};

	inline const bool _subchunkRegisterer = []() {
		sndx::RIFF::Chunk::registerChunkType<FMTchunk>();
		sndx::RIFF::Chunk::registerChunkType<DATAchunk>();
		sndx::RIFF::Chunk::registerChunkType<FACTchunk>();
		return true;
	}();

	class WAVfile {
	public:
		static constexpr std::array<char, 4> ID = {'W', 'A', 'V', 'E'};

	private:
		sndx::RIFF::File m_file{ ID };

		FMTchunk* m_fmt = nullptr;
		DATAchunk* m_data = nullptr;

	public:
		[[nodiscard]]
		sndx::RIFF::Chunk* getChunk(std::array<char, 4> id) {
			if (id == FMTchunk::ID) {
				return m_fmt;
			}
			else if (id == DATAchunk::ID) {
				return m_data;
			}
			else {
				return m_file.getChunk(id);
			}
		}

		template <std::derived_from<sndx::RIFF::Chunk> T> [[nodiscard]]
		T* getChunk() {
			if constexpr (std::is_same_v<T, FMTchunk>) {
				return m_fmt;
			}
			else if constexpr (std::is_same_v<T, DATAchunk>) {
				return m_data;
			}
			else {
				return static_cast<T*>(getChunk(T::ID));
			}
		}

		template <std::derived_from<sndx::RIFF::Chunk> T>
		bool emplaceChunk(const T& chunk) {
			auto& [it, success] = m_file.emplaceChunk(chunk);

			if constexpr (std::is_same_v<T, FMTchunk>) {
				m_fmt = it->second.get();
			}
			else if constexpr (std::is_same_v<T, DATAchunk>) {
				m_data = it->second.get();
			}

			return success;
		}

		[[nodiscard]]
		const DATAchunk& getData() {
			if (!m_data) [[unlikely]]
				throw std::runtime_error("WAV file data not defined");

			return *m_data;
		}

		[[nodiscard]]
		const FMTchunk& getFormat() {
			if (!m_fmt) [[unlikely]]
				throw std::runtime_error("WAV file format not defined");

			return *m_fmt;
		}

		void deserialize(sndx::serialize::Deserializer& deserializer) {
			m_file.deserialize(deserializer, ID);

			m_fmt = m_file.getChunk<FMTchunk>();
			m_data = m_file.getChunk<DATAchunk>();

			if (!m_fmt || !m_data)
				throw deserialize_error("WAVE file missing fmt  or data");
		}

		void serialize(sndx::serialize::Serializer& serializer) const {

			if (!m_fmt || !m_data)
				throw serialize_error("WAVE file missing fmt  or data");

			auto tmp = m_file.getHeader();
			const auto& chunks = m_file.getChunks();
			
			tmp.size = sizeof(tmp.type);
			for (const auto& [id, chunk] : chunks) {
				tmp.size += chunk->getLength();
			}

			serializer.serialize(tmp);
			serializer.serialize(*m_fmt);

			for (const auto& [id, chunk] : chunks) {
				if (id == sndx::RIFF::idToRawID(FMTchunk::ID) ||
					id == sndx::RIFF::idToRawID(DATAchunk::ID)) {
					
					continue;
				}

				serializer.serialize(*chunk);
			}

			serializer.serialize(*m_data);
		}
	};

	struct WAVmeta {
		uint32_t m_sampleRate = 0;
		uint32_t m_dataSize = 0;

		uint16_t m_bitDepth = 0;
		DataFormat m_format = DataFormat::error;
		uint16_t m_channels = 0;

		constexpr WAVmeta() noexcept = default;

		constexpr WAVmeta(const FMTchunk& fmt) noexcept:
			m_sampleRate(fmt.sampleRate),
			m_dataSize(0),
			m_bitDepth(fmt.bitDepth),
			m_format(DataFormat::error),
			m_channels(fmt.channels) {

			switch (fmt.format) {
			case WAVE_PCM_INT:
				m_format = DataFormat::pcm_int;
				break;
			case WAVE_IEE_FLOAT:
				m_format = DataFormat::iee_float;
				break;
			case WAVE_A_LAW:
				m_format = DataFormat::a_law;
				break;
			case WAVE_MU_LAW:
				m_format = DataFormat::mu_law;
				break;
			default:
				m_format = DataFormat::error;
			}
		}
	};

	// a WAV file decoder.
	// seeking functionality requires the underlying istream to be seekable
	class WAVdecoder : public AudioDecoder {
	protected:
		std::istream m_stream;

		WAVmeta m_meta{};
		size_t m_size = 0;
		size_t m_pos = 0;
		size_t m_offset = 0;
		bool m_dirty = false;

	public:
		explicit WAVdecoder(std::istream& stream) :
			m_stream(stream.rdbuf()), m_meta{}, 
			m_size(0), m_pos(0), m_offset(0), m_dirty(false) {

			sndx::serialize::Deserializer deserializer{ stream };
			bool seekable = true;

			sndx::RIFF::RIFFheader head;
			deserializer.deserialize(head);

			if (head.type != std::array<char, 4>{'W', 'A', 'V', 'E'})
				throw deserialize_error("RIFF file is not WAVE");

			sndx::RIFF::ChunkHeader header;
			deserializer.deserialize(header);

			
			if (header.id != std::array<char, 4>{'f', 'm', 't', ' '})
				throw deserialize_error("fmt  not first subchunk in RIFF");

			FMTchunk fmt(header);
			deserializer.deserialize(fmt);

			m_meta = WAVmeta(fmt);

			do {
				deserializer.deserialize(header);

				m_size = header.size;

				if (header.id == std::array<char, 4>{'d', 'a', 't', 'a'}) {
					m_offset = size_t(m_stream.tellg());
					return;
				}

				seekable = deserializer.discard(header.size, seekable);
			} while (m_stream.good());

			throw deserialize_error("Invalid .wav file");
		}

		// initialize the decoder with stream at the data, known meta and size
		explicit WAVdecoder(const std::istream& stream, const WAVmeta& meta, size_t size) :
			m_stream(stream.rdbuf()), m_meta(meta),
			m_size(uint32_t(size)), m_pos(0), m_offset(0), m_dirty(false) {

			m_offset = uint32_t(m_stream.tellg());

			if (m_offset == -1) m_offset = 0;
		}

		[[nodiscard]]
		const WAVmeta& getMeta() const noexcept {
			return m_meta;
		}

		[[nodiscard]]
		size_t getBitDepth() const noexcept override {
			return getMeta().m_bitDepth;
		}

		[[nodiscard]]
		size_t getChannels() const noexcept override {
			return getMeta().m_channels;
		}

		[[nodiscard]]
		size_t getSampleRate() const noexcept override {
			return getMeta().m_sampleRate;
		}

		[[nodiscard]]
		DataFormat getFormat() const noexcept override {
			return getMeta().m_format;
		}

		// returns previous position
		size_t seek(size_t pos) noexcept {
			if (pos >= m_size)
				pos = m_size;

			m_dirty = true;

			return std::exchange(m_pos, uint32_t(pos));
		}

		[[nodiscard]]
		size_t tell() const noexcept override {
			return m_pos;
		}

		[[nodiscard]]
		bool done() const override {
			if (m_pos >= m_size)
				return true;

			return !m_stream.good();
		}

		[[nodiscard]]
		std::vector<std::byte> readBytes(size_t count) {
			auto realCount = std::min(count, size_t(m_size - m_pos));
			
			std::vector<std::byte> out{};

			if (realCount == 0)
				return out;

			out.resize(realCount);

			if (m_dirty) {
				m_stream.seekg(m_offset + m_pos, std::ios::beg);
				m_dirty = false;
			}

			m_stream.read((char*)out.data(), realCount);
			out.resize(m_stream.gcount());
			m_pos += uint32_t(out.size());
			
			return out;
		}

		[[nodiscard]]
		std::vector<std::byte> readAll() {
			return readBytes(m_size);
		}
	};
}