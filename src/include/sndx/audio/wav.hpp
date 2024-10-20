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
			static void deserialize(const sndx::serialize::Deserializer&) {};
			static void serialize(const sndx::serialize::Serializer&) {};

			static constexpr uint32_t size() noexcept {
				return 0 + 16;
			}
		};

		struct Extended0 {
			static void deserialize(sndx::serialize::Deserializer& deserializer) {
				uint16_t size;
				deserializer.deserialize<std::endian::little>(size);

				if (size != 0)
					throw deserialize_error("Extended0 didn't have size 0");
			};

			static void serialize(sndx::serialize::Serializer& serializer) noexcept {
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

			std::visit([&deserializer]<typename T>(T&& arg) {
				deserializer.deserialize(std::forward<T>(arg));
			}, ext);
		};

		void serialize(sndx::serialize::Serializer& serializer) const override {
			serializer.serialize("fmt ", 4);
		
			std::visit([&serializer]<typename T>(const T&) {
				serializer.serialize<std::endian::little>(static_cast<uint32_t>(T::size()));
			}, ext);
				
			serializer.serialize<std::endian::little>(format);
			serializer.serialize<std::endian::little>(channels);
			serializer.serialize<std::endian::little>(sampleRate);
			serializer.serialize<std::endian::little>(byteRate);
			serializer.serialize<std::endian::little>(blockAlign);
			serializer.serialize<std::endian::little>(bitDepth);

			std::visit([&serializer]<typename T>(T&& arg) {
				serializer.serialize(std::forward<T>(arg));
			}, ext);
		};

		[[nodiscard]]
		constexpr uint32_t getLength() const override {
			uint32_t size = 4 + sizeof(uint32_t);

			std::visit([&size]<typename T>(const T&) {
				size += static_cast<uint32_t>(T::size());
			}, ext);

			return size;
		}

		[[nodiscard]]
		constexpr DataFormat getDataFormat() const noexcept {
			switch (format) {
			case WAVE_PCM_INT:
				return DataFormat::pcm_int;
			case WAVE_IEE_FLOAT:
				return DataFormat::iee_float;
			case WAVE_A_LAW:
				return DataFormat::a_law;
			case WAVE_MU_LAW:
				return DataFormat::mu_law;
			default:
				return DataFormat::error;
			}
		}
	};

	struct FACTchunk : public sndx::RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'f', 'a', 'c', 't' };

		uint32_t sampleLength = 0;

		explicit constexpr FACTchunk() = default;

		explicit constexpr FACTchunk(const sndx::RIFF::ChunkHeader&) noexcept {};

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

		explicit DATAchunk() = default;

		explicit DATAchunk(const sndx::RIFF::ChunkHeader& header) {
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
		uint32_t getLength() const override {
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
		const DATAchunk& getData() const {
			if (!m_data) [[unlikely]]
				throw std::runtime_error("WAV file data not defined");

			return *m_data;
		}

		[[nodiscard]]
		const FMTchunk& getFormat() const {
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

	// a WAV file decoder.
	// seeking functionality requires the underlying istream to be seekable
	class WAVdecoder : public AudioDecoder {
	protected:
		std::istream m_stream;

		FMTchunk m_meta{};
		size_t m_size = 0;
		size_t m_pos = 0;
		size_t m_offset = 0;
		bool m_dirty = false;

	public:
		explicit WAVdecoder(std::istream& stream) :
			m_stream(stream.rdbuf()) {

			sndx::serialize::Deserializer deserializer{ stream };
			bool seekable = true;

			sndx::RIFF::RIFFheader head;
			deserializer.deserialize(head);

			if (head.type != std::array<char, 4>{'W', 'A', 'V', 'E'})
				throw identifier_error("RIFF file is not WAVE");

			sndx::RIFF::ChunkHeader header;
			deserializer.deserialize(header);

			
			if (header.id != std::array<char, 4>{'f', 'm', 't', ' '})
				throw identifier_error("fmt  not first subchunk in RIFF");

			m_meta = FMTchunk(header);
			deserializer.deserialize(m_meta);

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
		explicit WAVdecoder(const std::istream& stream, const FMTchunk& meta, size_t size) :
			m_stream(stream.rdbuf()), m_meta(meta),
			m_size(uint32_t(size)) {

			m_offset = uint32_t(m_stream.tellg());

			if (m_offset == -1) m_offset = 0;
		}

		[[nodiscard]]
		const FMTchunk& getMeta() const noexcept {
			return m_meta;
		}

		[[nodiscard]]
		size_t getBitDepth() const noexcept override {
			return getMeta().bitDepth;
		}

		[[nodiscard]]
		size_t getSampleAlignment() const noexcept override {
			return getMeta().blockAlign;
		}

		[[nodiscard]]
		size_t getChannels() const noexcept override {
			return getMeta().channels;
		}

		[[nodiscard]]
		size_t getSampleRate() const noexcept override {
			return getMeta().sampleRate;
		}

		[[nodiscard]]
		DataFormat getFormat() const noexcept override {
			return getMeta().getDataFormat();
		}

		// returns previous position
		size_t seek(size_t pos) noexcept override {
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
		std::vector<std::byte> readRawBytes(size_t count) override {
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
		ALaudioData readSamples(size_t count) override {
			switch (m_meta.format) {
			case WAVE_PCM_INT:
				return readPCMintSamples(count);
			default:
				throw std::runtime_error("Unimplemented WAVE format");
			}
		}

	private:
		[[nodiscard]]
		ALaudioData readPCMintSamples(size_t count) {
			
			auto channels = (short)(getChannels());
			if (channels > 2 || channels == 0)
				throw std::runtime_error("Unsupported WAVE PCM channels format");

			auto bits = (short)(getBitDepth());
			if (bits > 64 || bits == 0)
				throw std::runtime_error("Unsupported WAVE PCM bit depth format");

			ALaudioMeta meta{};
			meta.m_frequency = getSampleRate();

			std::vector<std::byte> out{};

			if (bits <= 8) {
				meta.m_format = determineALformat(8, channels);

				auto data = readRawSamples(count);

				if (bits == 8) {
					out = std::move(data);
				}
				else {
					out.reserve(data.size());

					long double oldMax = std::exp2(bits) - 1;

					for (const auto& d : data) {
						auto val = sndx::math::remap((long double)d, 0.0l, oldMax, 0.0l, 255.0l);
						out.emplace_back((std::byte)(val));
					}
				}
			}
			else {
				meta.m_format = determineALformat(16, channels);

				auto data = readRawSamples(count);

				if (bits == 16) {
					out = std::move(data);
				}
				else {
					//out.reserve(count * channels * 2);
					// @TODO
					throw std::runtime_error("Strange PCM int formats not yet implemented");
				}
			}
			
			return ALaudioData{meta, std::move(out)};
		}
	};

	inline const bool _wavDecoderRegisterer = []() {
		return 
			registerDecoder<WAVdecoder>(".wav") &&
			registerDecoder<WAVdecoder>(".wave");
	}();
}