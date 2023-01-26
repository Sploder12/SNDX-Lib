#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"
#include <minimp3/minimp3_ex.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace sndx {

	[[nodiscard]]
	constexpr ALenum determineFormat(short bitsPerSample, short channels) {
		if (channels == 1) {
			if (bitsPerSample == 8) {
				return AL_FORMAT_MONO8;
			}
			
			return AL_FORMAT_MONO16;
		}

		if (bitsPerSample == 8) {
			return AL_FORMAT_STEREO8;
		}

		return AL_FORMAT_STEREO16;
	}

	template <typename T>
	struct AudioData {
		ALenum format;
		ALsizei freq;
		std::vector<T> buffer;

		void destroy() {
			buffer.clear();
		}
	};

	[[nodiscard]]
	static AudioData<mp3d_sample_t> loadMP3(std::istream& in) {
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
	static AudioData<mp3d_sample_t> loadMP3(const std::filesystem::path& path) {
		std::ifstream file(path, std::ifstream::in | std::ifstream::binary);

		if (file.is_open()) {
			return loadMP3(file);
		}

		throw std::runtime_error("Could not open file " + path.filename().string());
	}

	[[nodiscard]]
	static AudioData<char> loadWAV(std::istream& in) {
	
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

		AudioData<char> out{};
		out.format = determineFormat(bitsPerSample, channels);
		out.freq = sampleRate;
		out.buffer.resize(dataSize);

		in.read(out.buffer.data(), dataSize);

		return out;
	}

	[[nodiscard]]
	static AudioData<char> loadWAV(const std::filesystem::path& path) {
		std::ifstream file(path, std::ifstream::in | std::ifstream::binary);

		if (file.is_open()) {
			return loadWAV(file);
		}

		throw std::runtime_error("Could not open file " + path.filename().string());
	}
}