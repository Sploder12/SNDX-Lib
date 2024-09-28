#pragma once

#include "./audio_decoder.hpp"
#include "../data/serialize.hpp"

#include <iostream>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4267) 
#pragma warning(disable: 4244)
#endif

#ifndef NOMINMAX
#define NOMINMAX
#define UNDEF_NO_MIN_MAX
#endif

#ifndef WIN32_LEAN_AND_MEAN

#define WIN32_LEAN_AND_MEAN

#include <minimp3/minimp3.h>
#include <minimp3/minimp3_ex.h>
#undef WIN32_LEAN_AND_MEAN

#else

#include <minimp3/minimp3.h>
#include <minimp3/minimp3_ex.h>
#endif

#ifdef UNDEF_NO_MIN_MAX
#undef NOMINMAX
#undef UNDEF_NO_MIN_MAX
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace sndx::audio {
	class MP3decoder : public AudioDecoder {
	protected:
		std::istream m_stream;

		std::vector<mp3d_sample_t> m_buffer;
		mp3dec_ex_t m_dec;
		size_t m_pos = 0;
		bool m_dirty = false;

	public:
		explicit MP3decoder(std::istream& stream) :
			m_stream(stream.rdbuf()), m_buffer{},
			m_dec{}, m_pos(0), m_dirty(false) {
		
			stream.seekg(0, std::ios::end);
			size_t size = stream.tellg();
			stream.seekg(0);

			m_buffer.resize((size + sizeof(mp3d_sample_t) - 1) / sizeof(mp3d_sample_t));
			stream.read((char*)(m_buffer.data()), size);

			if (auto err = mp3dec_ex_open_buf(&m_dec, (const uint8_t*)(m_buffer.data()), size, MP3D_SEEK_TO_BYTE)) {
				throw deserialize_error("minimp3 returned open error " + std::to_string(err));
			}
		}

		~MP3decoder() {
			mp3dec_ex_close(&m_dec);
		}

	public:

		[[nodiscard]]
		const auto& getMeta() const noexcept {
			return m_dec.info;
		}

		[[nodiscard]]
		constexpr size_t getBitDepth() const noexcept override {
			return sizeof(mp3d_sample_t) * 8;
		}

		[[nodiscard]]
		constexpr size_t getSampleAlignment() const noexcept override {
			return sizeof(mp3d_sample_t);
		}

		[[nodiscard]]
		size_t getChannels() const noexcept override {
			return getMeta().channels;
		}

		[[nodiscard]]
		size_t getSampleRate() const noexcept override {
			return getMeta().hz;
		}

		[[nodiscard]]
		DataFormat getFormat() const noexcept override {
			return DataFormat::pcm_int;
		}

		size_t seek(size_t pos) noexcept override {
			if (pos >= m_dec.samples * getSampleAlignment() * getChannels())
				pos = m_dec.samples * getSampleAlignment() * getChannels();
			
			m_dirty = true;

			return std::exchange(m_pos, pos);
		}

		[[nodiscard]]
		size_t tell() const noexcept override {
			return m_pos;
		}

		[[nodiscard]]
		bool done() const override {
			if (m_pos >= m_dec.samples * getSampleAlignment() * getChannels())
				return true;

			return false;
		}

		[[nodiscard]]
		std::vector<std::byte> readRawBytes(size_t count) override {
			auto realCount = std::min(count, size_t(m_dec.samples * getSampleAlignment() * getChannels() - m_pos));

			std::vector<std::byte> out{};

			if (realCount == 0)
				return out;

			out.resize(realCount);

			if (m_dirty) {
				if (auto err = mp3dec_ex_seek(&m_dec, m_pos))
					throw deserialize_error("minimp3 returned seek error " + std::to_string(err));

				m_dirty = false;
			}

			size_t read = mp3dec_ex_read(&m_dec, (mp3d_sample_t*)(out.data()), count / getSampleAlignment() / getChannels());
			if (read != count / getSampleAlignment() / getChannels() && m_dec.last_error)
				throw deserialize_error("minimp3 returned read error " + std::to_string(m_dec.last_error));
				
			out.resize(read * getSampleAlignment() * getChannels());
			m_pos += out.size();

			return out;
		}

		[[nodiscard]]
		ALaudioData readSamples(size_t count) override {
			ALaudioMeta meta{ getSampleRate(), determineALformat(16, short(getChannels())) };
			
			return ALaudioData{std::move(meta), readRawBytes(count * getSampleAlignment() * getChannels())};
		}
	};
}