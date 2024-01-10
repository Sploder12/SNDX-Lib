#pragma once

#include "al.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4267) 
#pragma warning(disable: 4244)
#endif

#ifndef WIN32_LEAN_AND_MEAN

#define WIN32_LEAN_AND_MEAN
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>
#include <minimp3/minimp3_ex.h>
#undef WIN32_LEAN_AND_MEAN

#else
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>
#include <minimp3/minimp3_ex.h>
#endif


#ifdef _MSC_VER
#pragma warning(pop)
#endif


#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <bit>

#include <algorithm>
#include <execution>

namespace sndx {

	enum class ChannelFormat : ALenum {
		mono8 = AL_FORMAT_MONO8,
		mono16 = AL_FORMAT_MONO16,
		stereo8 = AL_FORMAT_STEREO8,
		stereo16 = AL_FORMAT_STEREO16,
	};

	[[nodiscard]]
	constexpr ChannelFormat determineFormat(short bitsPerSample, short channels) {
		using enum ChannelFormat;
		if (channels == 1) {
			if (bitsPerSample == 8) {
				return mono8;
			}
			
			return mono16;
		}

		if (bitsPerSample == 8) {
			return stereo8;
		}

		return stereo16;
	}

	template <typename T>
	struct AudioData {
		static_assert(std::is_integral_v<T>);

		ChannelFormat format{};
		ALsizei freq{};
		std::vector<T> buffer{};

		bool isMono() const {
			return format == ChannelFormat::mono8 || format == ChannelFormat::mono16;
		}

		void destroy() {
			buffer.clear();
		}

		void makeMono() {
			using enum ChannelFormat;
			if (format == mono8 || format == mono16) return;

			std::vector<long long> tmp{};
			tmp.resize(buffer.size() / 2);

			for (int i = 0; i < buffer.size() / 2; i += 2) {
				// average the stereo
				tmp[i] = (buffer[i * 2] + buffer[i * 2 + 1]) / 2;
			}
			
			buffer.clear();
			for (auto data : tmp) {
				buffer.emplace_back(T(data));
			}

			if (format == stereo8) format = mono8;
			else if (format == stereo16) format = mono16;
		}

		void makeStereo() {
			using enum ChannelFormat;
			if (format == stereo8 || format == stereo16) return;

			std::vector<T> tmp{};
			tmp.resize(buffer.size() * 2);

			for (int i = 0; i < buffer.size(); ++i) {
				tmp[i * 2] = buffer[i];
				tmp[i * 2 + 1] = buffer[i];
			}

			buffer = std::move(tmp);

			if (format == mono8) format = stereo8;
			else if (format == mono16) format = stereo16;
		}

		void normalize() {
			static constexpr size_t tmax = (1 << (sizeof(T) * 8 - std::is_signed_v<T>)) - 1;

			T maxVal = *std::max_element(buffer.begin(), buffer.end());

			if constexpr (std::is_signed_v<T>) {
				T minVal = *std::min_element(buffer.begin(), buffer.end());

				T best = std::max(abs(minVal), abs(maxVal));

				long double scalar = (long double)(tmax) / (long double)(best);
				for (auto& val : buffer) {
					val = T((long double)(val) * scalar);
				}
			}
			else {

				long double scalar = (long double)(tmax) / (long double)(maxVal);

				for (auto& val : buffer) {
					val = T((long double)(val)*scalar);
				}
			}
		}

		template <typename F = unsigned char> [[nodiscard]]
		AudioData<F> asType() {
			static_assert(std::is_integral_v<F>);
			static_assert(!std::is_same_v<F, char> || !std::is_signed_v<F>, "I know what you're thinking, but you don't want signed char.");
			static_assert(sizeof(T) != sizeof(F) || std::is_signed_v<T> != std::is_signed_v<F>);

			static constexpr size_t oldMax = (1 << (sizeof(T) * 8 - std::is_signed_v<T>)) - 1;
			static constexpr size_t newMax = (1 << (sizeof(F) * 8 - std::is_signed_v<F>)) - 1;
			
			static constexpr long double conversion = (long double)(newMax) / (long double)(oldMax);

			AudioData<F> out;

			out.freq = freq;
			out.buffer.resize(buffer.size());

			std::transform(buffer.begin(), buffer.end(), out.buffer.begin(), [](long long val) {
				if constexpr (std::is_unsigned_v<F> && !std::is_unsigned_v<T>) {
					long double tmp = (long double)(val + oldMax) * conversion / 2.0;

					return F(tmp);
				}
				else if constexpr (std::is_unsigned_v<T> && !std::is_unsigned_v<F>) {
					val -= oldMax / 2;
					return F((long double)(val) * conversion * 1.9);
				}
				else {
					return F((long double)(val) * conversion);
				}
			});

			if constexpr (sizeof(F) == 1) {
				out.format = (isMono()) ? ChannelFormat::mono8 : ChannelFormat::stereo8;
			}
			else if constexpr (sizeof(F) == 2) {
				out.format = (isMono()) ? ChannelFormat::mono16 : ChannelFormat::stereo16;
			}
			else {
				out.format = format;
			}
			
			return out;
		}
	};

	[[nodiscard]]
	inline AudioData<short> loadMP3(std::istream& in) {
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		in.seekg(0);

		std::vector<char> data;
		data.resize(size);
		in.read(data.data(), size);

		mp3dec_t mp3d;
		mp3dec_file_info_t info;

		mp3dec_load_buf(&mp3d, (const uint8_t*)data.data(), size, &info, 0, 0);

		AudioData<mp3d_sample_t> out{};
		out.format = determineFormat(16, info.channels);
		out.freq = info.hz;

		out.buffer.resize(info.samples);
		for (int i = 0; i < info.samples; ++i) {
			out.buffer[i] = info.buffer[i];
		}

		delete[] info.buffer;

		return out;
	}

	[[nodiscard]]
	inline std::optional<AudioData<short>> loadMP3(const std::filesystem::path& path) {
		std::ifstream file(path, std::ifstream::in | std::ifstream::binary);

		if (file.is_open()) {
			return loadMP3(file);
		}

		return {};
	}

	[[nodiscard]]
	inline AudioData<short> loadWAV(std::istream& in) {
	
		char type[4];
		unsigned long size, chunkSize;
		short formatType, channels;
		unsigned long sampleRate, avgBytesPerSec;
		short bytesPerSample, bitsPerSample;
		unsigned long dataSize;
		//check for metadata
		in.read(type, sizeof(type));
		if (type[0] != 'R' || type[1] != 'I' || type[2] != 'F' || type[3] != 'F') {
			throw std::runtime_error("WAV file missing RIFF");
		}

		in.read((char*)&size, sizeof(size));
		//check for metadata pt2
		in.read(type, sizeof(type));
		if (type[0] != 'W' || type[1] != 'A' || type[2] != 'V' || type[3] != 'E') {
			throw std::runtime_error("WAV file missing WAVE");
		}
		//check for metadata pt3
		in.read(type, sizeof(type));
		if (type[0] != 'f' || type[1] != 'm' || type[2] != 't' || type[3] != ' ') {
			throw std::runtime_error("WAV file missing fmt ");
		}

		//get metadata
		in.read((char*)&chunkSize, sizeof(chunkSize));
		in.read((char*)&formatType, sizeof(formatType));
		in.read((char*)&channels, sizeof(channels));
		in.read((char*)&sampleRate, sizeof(sampleRate));
		in.read((char*)&avgBytesPerSec, sizeof(avgBytesPerSec));
		in.read((char*)&bytesPerSample, sizeof(bytesPerSample));
		in.read((char*)&bitsPerSample, sizeof(bitsPerSample));
		//check for metadata pt4
		in.read(type, sizeof(type));
		if (type[0] != 'd' || type[1] != 'a' || type[2] != 't' || type[3] != 'a') {
			throw std::runtime_error("WAV file missing data");
		}
		//get that data
		in.read((char*)&dataSize, sizeof(dataSize));

		AudioData<char> tmp{};
		tmp.buffer.resize(dataSize);
		tmp.format = determineFormat(bitsPerSample, channels);
		tmp.freq = sampleRate;

		in.read(tmp.buffer.data(), dataSize);

		if (tmp.format == ChannelFormat::mono8 || tmp.format == ChannelFormat::stereo8) {
			return tmp.asType<short>();
		}
		else {
			static constexpr auto endianess = int(std::endian::native);

			AudioData<short> out{};
			out.freq = tmp.freq;
			out.format = tmp.format;

			out.buffer.resize(tmp.buffer.size() / 2);

			for (int i = 0; i < tmp.buffer.size() / 2; ++i) {
				char* cur = (char*)(&out.buffer[i]);
				*(cur + endianess) = tmp.buffer[i * 2];
				*(cur + 1 - endianess) = tmp.buffer[i * 2 + 1];
			}
			return out;
		}
	}

	[[nodiscard]]
	inline std::optional<AudioData<short>> loadWAV(const std::filesystem::path& path) {
		std::ifstream file(path, std::ifstream::in | std::ifstream::binary);

		if (file.is_open()) {
			return loadWAV(file);
		}

		return {};
	}

	[[nodiscard]]
	inline std::optional<AudioData<short>> loadAudioFile(const std::filesystem::path& path) {
		auto&& extension = path.extension().string();

		if (extension == ".wav") {
			return loadWAV(path);
		}
		if (extension == ".mp3") {
			return loadMP3(path);
		}
		
		return {};
	}
}