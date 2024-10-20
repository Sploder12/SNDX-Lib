#pragma once

#include <cstddef>
#include <chrono>
#include <vector>
#include <istream>
#include <fstream>
#include <filesystem>
#include <optional>

#include "./al/audio_data.hpp"

#include "../utility/registry.hpp"

namespace sndx::audio {

	enum class DataFormat : uint8_t {
		error = 0,
		pcm_int,
		iee_float,

		a_law,
		mu_law
	};

	class AudioDecoder {
	public:
		using Factory = std::unique_ptr<AudioDecoder> (*)(std::istream&);

		virtual ~AudioDecoder() = default;

		[[nodiscard]]
		virtual size_t getBitDepth() const noexcept = 0;

		[[nodiscard]]
		virtual size_t getSampleAlignment() const noexcept = 0;

		[[nodiscard]]
		virtual size_t getChannels() const noexcept = 0;

		[[nodiscard]]
		virtual size_t getSampleRate() const noexcept = 0;

		[[nodiscard]]
		virtual DataFormat getFormat() const noexcept = 0;

		[[nodiscard]]
		virtual bool done() const = 0;

		[[nodiscard]]
		virtual size_t tell() const = 0;

		virtual size_t seek(size_t pos) = 0;

		// returns previous position
		size_t seekSample(size_t sample) {
			auto bytePos = (getBitDepth() * getChannels() * sample) / 8;

			return seek(bytePos);
		}

		// returns previous position
		size_t seekSecond(std::chrono::duration<float> second) {
			return seekSample(size_t(second.count() * getSampleRate()));
		}

		[[nodiscard]]
		virtual std::vector<std::byte> readRawBytes(size_t count) = 0;

		[[nodiscard]]
		std::vector<std::byte> readRawSamples(size_t count) {
			auto bytes = (getBitDepth() * getChannels() * count) / 8;
			return readRawBytes(bytes);
		}

		[[nodiscard]]
		std::vector<std::byte> readRawSeconds(std::chrono::duration<float> seconds) {
			return readRawSamples(size_t(seconds.count() * getSampleRate()));
		}

		[[nodiscard]]
		std::vector<std::byte> readAllRaw() {
			return readRawBytes(std::numeric_limits<size_t>::max());
		}

		[[nodiscard]]
		virtual ALaudioData readSamples(size_t count) = 0;

		[[nodiscard]]
		ALaudioData readSeconds(std::chrono::duration<float> seconds) {
			return readSamples(size_t(seconds.count() * getSampleRate()));
		}

		[[nodiscard]]
		ALaudioData readAll() {
			return readSamples(std::numeric_limits<size_t>::max());
		}
	};

	using DecoderRegistry = utility::FactoryRegistry<std::string, AudioDecoder::Factory>;

	[[nodiscard]]
	inline DecoderRegistry& getDecoderRegistry() noexcept {
		static DecoderRegistry defaultRegistry{};
		return defaultRegistry;
	}

	template <std::derived_from<AudioDecoder> T>
	bool registerDecoder(const std::string& extension) {
		return getDecoderRegistry().add(extension, [](std::istream& in) {
			return std::unique_ptr<AudioDecoder>{std::make_unique<T>(in)};
		});
	}

	inline bool removeDecoder(const std::string& extension) {
		return getDecoderRegistry().remove(extension);
	}

	[[nodiscard]] // may throw DecoderRegistry::no_factory_error
	inline decltype(auto) createDecoder(const std::string& extension, std::istream& stream) {
		return getDecoderRegistry().apply(extension, stream);
	}

	[[nodiscard]]
	inline std::unique_ptr<AudioDecoder> tryCreateDecoder(const std::string& extension, std::istream& stream) {
		try {
			return createDecoder(extension, stream);
		}
		catch (...) {
			return nullptr;
		}
	}

	[[nodiscard]]
	inline std::optional<ALaudioData> readFile(const std::filesystem::path& filePath) {
		std::ifstream file{ filePath, std::ios::binary };

		if (!file.is_open())
			return std::nullopt;

		auto dec = tryCreateDecoder(filePath.extension().string(), file);
		if (!dec)
			return std::nullopt;

		try {
			return dec->readAll();
		}
		catch (...) {
			return std::nullopt;
		}
	}
}