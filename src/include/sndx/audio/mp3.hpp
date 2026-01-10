#pragma once

#include "./audiodata.hpp"

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
	class MP3decoder {
	protected:
		std::vector<mp3d_sample_t> m_buffer;
		mp3dec_ex_t m_dec;
		size_t m_pos = 0;
		bool m_dirty = false;

	public:
		explicit MP3decoder(std::istream& stream) {
		
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
		constexpr size_t getBitDepth() const noexcept {
			return sizeof(mp3d_sample_t) * 8;
		}

		[[nodiscard]]
		size_t getChannels() const noexcept {
			return getMeta().channels;
		}

		[[nodiscard]]
		size_t getSampleRate() const noexcept {
			return getMeta().hz;
		}

		size_t seek(size_t pos) noexcept {
			if (pos >= m_dec.samples * getChannels())
				pos = m_dec.samples * getChannels();
			
			m_dirty = true;

			return std::exchange(m_pos, pos);
		}

		[[nodiscard]]
		size_t tell() const noexcept {
			return m_pos;
		}

		[[nodiscard]]
		bool done() const {
			if (m_pos >= m_dec.samples * getChannels())
				return true;

			return false;
		}

		[[nodiscard]]
		AudioData<mp3d_sample_t> readSamples(size_t count) {
	
			auto realCount = std::min(count, size_t(m_dec.samples * getChannels() - m_pos));
			if (realCount == 0)
				return AudioData<mp3d_sample_t>{getChannels(), getSampleRate()};

			std::vector<mp3d_sample_t> out(realCount);

			if (m_dirty) {
				if (auto err = mp3dec_ex_seek(&m_dec, m_pos))
					throw deserialize_error("minimp3 returned seek error " + std::to_string(err));

				m_dirty = false;
			}

			size_t read = mp3dec_ex_read(&m_dec, out.data(), count / getChannels());
			if (read != count / getChannels() && m_dec.last_error)
				throw deserialize_error("minimp3 returned read error " + std::to_string(m_dec.last_error));

			out.resize(read * getChannels());
			m_pos += out.size();

			return AudioData<mp3d_sample_t>{getChannels(), getSampleRate(), std::move(out)};
		}
	};
}