#pragma once

#include "./audio_decoder.hpp"

#include "../utility/endian.hpp"
#include "../data/RIFF.hpp"

#include <array>
#include <concepts>
#include <ios>
#include <variant>

namespace sndx {
	namespace audio::FMTextension {
		struct ExtendedNone;
		struct Extended0;
		struct Extended;
	}

	template<class T>
		requires
			std::is_same_v<T, audio::FMTextension::ExtendedNone> ||
			std::is_same_v<T, audio::FMTextension::Extended0> ||
			std::is_same_v<T, audio::FMTextension::Extended>
	struct Serializer<T> {
		template <class SerializeIt>
		constexpr void serialize(const T& v, SerializeIt& it) const {
			v.serialize(it);
		}
	};

	template<class T>
		requires
			std::is_same_v<T, audio::FMTextension::ExtendedNone> ||
			std::is_same_v<T, audio::FMTextension::Extended0> ||
			std::is_same_v<T, audio::FMTextension::Extended>
	struct Deserializer<T> {
		template <class DeserializeIt>
		constexpr void deserialize(T& to, DeserializeIt& in, DeserializeIt end) const {
			to.deserialize(in, end);
		}
	};
}

namespace sndx::audio {

	static constexpr uint16_t WAVE_PCM_INT = 1;
	static constexpr uint16_t WAVE_IEE_FLOAT = 3;
	static constexpr uint16_t WAVE_A_LAW = 6;
	static constexpr uint16_t WAVE_MU_LAW = 7;
	static constexpr uint16_t WAVE_EXTENSIBLE = 0xFFFE;

	namespace FMTextension {
		struct ExtendedNone {
			template <class DeserializeIt>
			static constexpr void deserialize(DeserializeIt&, DeserializeIt) {};

			template <class SerializeIt>
			static constexpr void serialize(SerializeIt&) {};

			static constexpr uint32_t size() noexcept {
				return 0 + 16;
			}
		};

		struct Extended0 {
			template <class DeserializeIt>
			static constexpr void deserialize(DeserializeIt& in, DeserializeIt end) {
				uint16_t size;
				deserializeFromAdjust(size, in, end);

				if (size != 0)
					throw deserialize_error("Extended0 didn't have size 0");
			};

			template <class SerializeIt>
			static constexpr void serialize(SerializeIt& out) noexcept {
				serializeToAdjust(out, static_cast<uint16_t>(0));
			};

			static constexpr uint32_t size() noexcept {
				return 0 + sizeof(uint16_t) + 16;
			}
		};

		struct Extended {
			uint16_t validBitsPerSample = 0;
			uint32_t channelMask = 0;
			std::array<char, 16> guid{ 0 };

			static constexpr uint16_t dataSize = sizeof(validBitsPerSample) + sizeof(channelMask) + sizeof(guid);

			template <class DeserializeIt>
			constexpr void deserialize(DeserializeIt& in, DeserializeIt end) {
				uint16_t size;
				deserializeFromAdjust(size, in, end);

				if (size != dataSize)
					throw deserialize_error("Extended format didn't have size 22");

				deserializeFromAdjust(validBitsPerSample, in, end);
				deserializeFromAdjust(channelMask, in, end);
				deserializeFromAdjust(guid, in, end);
			};

			template <class SerializeIt>
			void serialize(SerializeIt& out) const {
				serializeToAdjust(out, dataSize);

				serializeToAdjust(out, validBitsPerSample);
				serializeToAdjust(out, channelMask);
				serializeToAdjust(out, guid);
			};

			static constexpr uint32_t size() noexcept {
				return dataSize + sizeof(uint16_t) + 16;
			}
		};
	}

	struct FMTchunk : public RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'f', 'm', 't', ' ' };

		using ExtendedNone = FMTextension::ExtendedNone;
		using Extended0 = FMTextension::Extended0;
		using Extended = FMTextension::Extended;

		uint16_t format = 0;
		uint16_t channels = 0;
		uint32_t sampleRate = 0;
		uint32_t byteRate = 0;
		uint16_t blockAlign = 0;
		uint16_t bitDepth = 0;

		std::variant<ExtendedNone, Extended0, Extended> ext = ExtendedNone();

		explicit constexpr FMTchunk() = default;

		explicit constexpr FMTchunk(const RIFF::ChunkHeader& header) {
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
				throw bad_field_error("Invalid fmt  size");
			}
		}
		
		[[nodiscard]]
		constexpr uint16_t getSampleSize() const {
			return blockAlign / channels;
		}

		void deserialize(const std::vector<uint8_t>& in) override {
			auto it = in.begin();
			deserializeFromAdjust(format, it, in.end());
			deserializeFromAdjust(channels, it, in.end());
			deserializeFromAdjust(sampleRate, it, in.end());
			deserializeFromAdjust(byteRate, it, in.end());
			deserializeFromAdjust(blockAlign, it, in.end());
			deserializeFromAdjust(bitDepth, it, in.end());

			std::visit([&it, end = in.end()]<typename T>(T&& arg) {
				deserializeFromAdjust(std::forward<T>(arg), it, end);
			}, ext);
		};

		std::vector<uint8_t> serialize() const override {
			std::vector<uint8_t> out{};
			auto it = std::back_inserter(out);

			serializeToAdjust(it, std::array{ 'f', 'm', 't', ' ' });
		
			std::visit([&it]<typename T>(const T&) {
				serializeToAdjust(it, static_cast<uint32_t>(T::size()));
			}, ext);
				
			serializeToAdjust(it, format);
			serializeToAdjust(it, channels);
			serializeToAdjust(it, sampleRate);
			serializeToAdjust(it, byteRate);
			serializeToAdjust(it, blockAlign);
			serializeToAdjust(it, bitDepth);

			std::visit([&it]<typename T>(const T& arg) {
				serializeToAdjust(it, arg);
			}, ext);

			return out;
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

	struct FACTchunk : public RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'f', 'a', 'c', 't' };

		uint32_t sampleLength = 0;

		explicit constexpr FACTchunk() = default;

		explicit constexpr FACTchunk(const RIFF::ChunkHeader&) noexcept {};

		void deserialize(const std::vector<uint8_t>& data) override {
			sndx::deserialize(sampleLength, data);
		}

		std::vector<uint8_t> serialize() const override {
			std::vector<uint8_t> out{};
			auto it = std::back_inserter(out);

			serializeToAdjust(it, std::array{ 'f', 'a', 'c', 't' });
			serializeToAdjust(it, static_cast<uint32_t>(sizeof(sampleLength)));
			serializeToAdjust(it, sampleLength);

			return out;
		}

		[[nodiscard]]
		constexpr uint32_t getLength() const noexcept override {
			return uint32_t(sizeof(ID) + sizeof(uint32_t) + sizeof(sampleLength));
		}
	};

	struct DATAchunk : public RIFF::Chunk {
		static constexpr std::array<char, 4> ID = { 'd', 'a', 't', 'a' };

		std::vector<uint8_t> data{};

		explicit DATAchunk() = default;

		explicit DATAchunk(const RIFF::ChunkHeader& header) {
			data.resize(header.size);
		}

		void deserialize(const std::vector<uint8_t>& data) override {
			auto it = data.begin();
			for (auto& b : this->data) {
				deserializeFromAdjust(b, it, data.end());
			}
		}

		std::vector<uint8_t> serialize() const override {
			std::vector<uint8_t> out{};
			auto it = std::back_inserter(out);

			serializeToAdjust(it, std::array{ 'd', 'a', 't', 'a' });
			serializeToAdjust(it, static_cast<uint32_t>(data.size()));

			for (const auto& b : data) {
				serializeToAdjust(it, b);
			}

			if (data.size() % 2 == 1) {
				serializeToAdjust(it, static_cast<uint8_t>(0));
			}

			return out;
		}

		[[nodiscard]]
		uint32_t getLength() const override {
			return uint32_t(4 + sizeof(uint32_t) + data.size() + (data.size() % 2 == 1));
		}
	};

	inline const bool _subchunkRegisterer = []() {
		RIFF::Chunk::registerChunkType<FMTchunk>();
		RIFF::Chunk::registerChunkType<DATAchunk>();
		RIFF::Chunk::registerChunkType<FACTchunk>();
		return true;
	}();

	class WAVfile {
	public:
		static constexpr std::array<char, 4> ID = {'W', 'A', 'V', 'E'};

	private:
		RIFF::File m_file{ ID };

		FMTchunk* m_fmt = nullptr;
		DATAchunk* m_data = nullptr;

	public:
		[[nodiscard]]
		RIFF::Chunk* getChunk(std::array<char, 4> id) {
			if (id == FMTchunk::ID)
				return m_fmt;

			if (id == DATAchunk::ID)
				return m_data;

			return m_file.getChunk(id);
		}

		template <std::derived_from<RIFF::Chunk> T> [[nodiscard]]
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

		template <std::derived_from<RIFF::Chunk> T>
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

		template <class InputIt>
		void deserialize(InputIt& in, InputIt end) {
			deserializeFromAdjust(m_file, in, end);

			if (m_file.getHeader().type != ID)
				throw bad_field_error("WAVE file missing WAVE");

			m_fmt = m_file.getChunk<FMTchunk>();
			m_data = m_file.getChunk<DATAchunk>();

			if (!m_fmt || !m_data)
				throw bad_field_error("WAVE file missing fmt  or data");
		}

		template <class SerializeIt>
		void serialize(SerializeIt& it) const {

			if (!m_fmt || !m_data)
				throw std::logic_error("WAVE file missing fmt  or data");

			auto tmp = m_file.getHeader();
			const auto& chunks = m_file.getChunks();
			
			tmp.size = sizeof(tmp.type);
			for (const auto& [id, chunk] : chunks) {
				tmp.size += chunk->getLength();
			}

			serializeToAdjust(it, tmp);

			auto buf = m_fmt->serialize();
			for (auto b : buf) {
				serializeToAdjust(it, b);
			}

			for (const auto& [id, chunk] : chunks) {
				if (id == RIFF::idToRawID(FMTchunk::ID) ||
					id == RIFF::idToRawID(DATAchunk::ID)) {
					
					continue;
				}

				buf = chunk->serialize();
				for (auto b : buf) {
					serializeToAdjust(it, b);
				}
			}

			buf = m_data->serialize();
			for (auto b : buf) {
				serializeToAdjust(it, b);
			}
		}
	};

	// a WAV file decoder.
	// seeking functionality requires the underlying istream to be seekable
	class WAVdecoder : public AudioDecoder {
		std::istream m_stream;

		FMTchunk m_meta{};
		size_t m_size = 0;
		size_t m_pos = 0;
		size_t m_offset = 0;
		bool m_dirty = false;

	public:
		explicit WAVdecoder(std::istream& stream) :
			m_stream(stream.rdbuf()) {
			m_stream >> std::noskipws;

			auto it = std::istream_iterator<uint8_t>(m_stream);
			auto end = std::istream_iterator<uint8_t>();

			RIFF::RIFFheader head;
			deserializeFromAdjust(head, it, end);

			if (head.type != std::array<char, 4>{'W', 'A', 'V', 'E'})
				throw bad_field_error("RIFF file is not WAVE");

			RIFF::ChunkHeader header;
			deserializeFromAdjust(header, it, end);

			
			if (header.id != std::array<char, 4>{'f', 'm', 't', ' '})
				throw bad_field_error("fmt  not first subchunk in RIFF");

			m_meta = FMTchunk(header);
			std::vector<uint8_t> buf{};
			buf.resize(m_meta.getLength() - 4 - sizeof(uint32_t));
			for (auto& b : buf) {
				deserializeFromAdjust(b, it, end);
			}
			m_meta.deserialize(buf);

			do {
				deserializeFromAdjust(header, it, end);

				m_size = header.size;

				if (header.id == std::array<char, 4>{'d', 'a', 't', 'a'}) {
					m_offset = size_t(m_stream.tellg()) - 1;
					m_dirty = true;
					return;
				}
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
						auto val = math::remap((long double)d, 0.0l, oldMax, 0.0l, 255.0l);
						out.emplace_back(static_cast<std::byte>(val));
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
