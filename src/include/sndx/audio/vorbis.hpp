#pragma once

#include "./audio_decoder.hpp"
#include "../data/ogg_wrap.hpp"
#include "../data/serialize.hpp"

#include <vorbis/codec.h>

#include <string_view>
#include <vector>

namespace sndx::audio {
	namespace vorbis {
		[[nodiscard]]
		inline std::string_view version() noexcept {
			return vorbis_version_string();
		}

		class Comment {
		protected:
			vorbis_comment m_comment{};

		public:
			[[nodiscard]]
			vorbis_comment* get() noexcept {
				return &m_comment;
			}

			Comment() noexcept {
				vorbis_comment_init(&m_comment);
			}

			~Comment() noexcept {
				vorbis_comment_clear(&m_comment);
			}

			void add(const std::string& key, const std::string& value) noexcept {
				vorbis_comment_add_tag(&m_comment, key.c_str(), value.c_str());
			}

			[[nodiscard]]
			int count(const std::string& key) noexcept {
				return vorbis_comment_query_count(&m_comment, key.c_str());
			}

			[[nodiscard]]
			std::optional<std::string> query(const std::string& key, size_t index = 0) noexcept {
				auto cstr = vorbis_comment_query(&m_comment, key.c_str(), int(index));

				if (!cstr)
					return std::nullopt;

				return std::string(cstr);
			}

			bool out(ogg::Packet& packet) noexcept {
				return vorbis_commentheader_out(&m_comment, packet.get()) == 0;
			}
		};

		struct Info {
			vorbis_info m_info{};

			[[nodiscard]]
			vorbis_info* get() noexcept {
				return &m_info;
			}

			explicit Info() noexcept {
				vorbis_info_init(&m_info);
			}

			~Info() noexcept {
				vorbis_info_clear(&m_info);
			}

			[[nodiscard]] // -1 on error
			int blockSize(bool longBlock) noexcept {
				return vorbis_info_blocksize(&m_info, int(longBlock));
			}

			[[nodiscard]]
			long blockSize(ogg::Packet& packet) noexcept {
				return vorbis_packet_blocksize(&m_info, packet.get());
			}
			
			bool halfRate(bool enable) noexcept {
				return vorbis_synthesis_halfrate(&m_info, int(enable)) == 0;
			}

			bool isHalfRate() noexcept {
				return vorbis_synthesis_halfrate_p(&m_info) == 1;
			}

			bool headerIn(Comment& comment, ogg::Packet& in) {
				return vorbis_synthesis_headerin(&m_info, comment.get(), in.get()) == 0;
			}
		};

		class State {
		protected:
			vorbis_dsp_state m_state{};
			bool init = false;

		public:
			[[nodiscard]]
			vorbis_dsp_state* get() noexcept {
				return &m_state;
			}

			explicit State() noexcept = default;

			explicit State(Info& info) {
				if (vorbis_synthesis_init(&m_state, info.get()) != 0)
					throw std::runtime_error("Could not initialize vorbis State");

				init = true;
			}

			State& operator=(State&& other) noexcept {
				std::swap(m_state, other.m_state);
				std::swap(init, other.init);
				return *this;
			}

			~State() noexcept {
				if (init) {
					vorbis_dsp_clear(&m_state);
					init = false;
				}
			}

			// return time in seconds (or -1)
			double granuleTime(ogg_int64_t granulePos) noexcept {
				return vorbis_granule_time(&m_state, granulePos);
			}
		};

		class Block {
		protected:
			vorbis_block m_block{};
			bool init = false;

		public:
			[[nodiscard]]
			vorbis_block* get() noexcept {
				return &m_block;
			}

			explicit Block() noexcept = default;

			explicit Block(State& state) {
				if (vorbis_block_init(state.get(), &m_block) != 0)
					throw std::runtime_error("Could not init block");

				init = true;
			};

			Block& operator=(Block&& other) noexcept {
				std::swap(m_block, other.m_block);
				std::swap(init, other.init);
				return *this;
			}

			~Block() noexcept {
				if (init) {
					vorbis_block_clear(&m_block);
					init = false;
				}
			}

			bool decode(ogg::Packet& packet) noexcept {
				return vorbis_synthesis(&m_block, packet.get()) == 0;
			}

			bool trackOnly(ogg::Packet& packet) noexcept {
				return vorbis_synthesis_trackonly(&m_block, packet.get()) == 0;
			}
		};

		class DecodeState : public State {
		public:
			using State::State;

			bool blockIn(Block& block) noexcept {
				return vorbis_synthesis_blockin(&m_state, block.get()) == 0;
			}

			static bool isFirstPacket(ogg::Packet& packet) noexcept {
				return vorbis_synthesis_idheader(packet.get()) == 1;
			}

			[[nodiscard]]
			int availableSamples() noexcept {
				return vorbis_synthesis_pcmout(&m_state, nullptr);
			}

			[[nodiscard]]
			std::vector<std::span<float>> lapout(const Info& info) noexcept {
				float** pcm;
				auto samples = vorbis_synthesis_lapout(&m_state, &pcm);

				std::vector<std::span<float>> out{};
				out.resize(info.m_info.channels);

				for (size_t i = 0; i < out.size(); ++i) {
					out[i] = std::span<float>{pcm[i], size_t(samples)};
				}

				return out;
			}

			[[nodiscard]]
			std::vector<std::span<float>> out(const Info& info) noexcept {
				float** pcm;
				auto samples = vorbis_synthesis_pcmout(&m_state, &pcm);

				std::vector<std::span<float>> out{};
				out.resize(info.m_info.channels);

				if (samples == 0)
					return out;

				for (size_t i = 0; i < out.size(); ++i) {
					out[i] = std::span<float>{ pcm[i], size_t(samples) };
				}

				return out;
			}

			bool consumeSamples(int samples) {
				if (samples < 0)
					throw std::invalid_argument("samples cannot be negative");

				return vorbis_synthesis_read(&m_state, samples) == 0;
			}
		};
	}

	class VorbisDecoder : public AudioDecoder {
	protected:
		ogg::Decoder m_decoder;

		vorbis::Info m_info{};
		vorbis::Comment m_comment{};
		vorbis::DecodeState m_state;
		vorbis::Block m_block;

	public:
		explicit VorbisDecoder(std::istream& stream) :
			m_decoder(stream) {

			for (size_t i = 0; i < 3; ++i) {
				auto opckt = m_decoder.getNextPacket();

				if (!opckt)
					throw deserialize_error("Could not get first 3 packets");

				if (!m_info.headerIn(m_comment, *opckt))
					throw deserialize_error("Could not header in");
			}

			m_state = vorbis::DecodeState(m_info);
			m_block = vorbis::Block(m_state);
		}

		[[nodiscard]]
		const auto& getMeta() const noexcept {
			return m_info.m_info;
		}

		[[nodiscard]]
		constexpr size_t getBitDepth() const noexcept override {
			return sizeof(float) * 8;
		}

		[[nodiscard]]
		constexpr size_t getSampleAlignment() const noexcept override {
			return sizeof(float);
		}


		[[nodiscard]]
		size_t getChannels() const noexcept override {
			return getMeta().channels;
		}

		[[nodiscard]]
		size_t getSampleRate() const noexcept override {
			return getMeta().rate;
		}

		[[nodiscard]]
		DataFormat getFormat() const noexcept override {
			return DataFormat::iee_float;
		}

		size_t seek(size_t) override {
			throw std::runtime_error("Unimplemented");
		}

		[[nodiscard]]
		size_t tell() const override {
			throw std::runtime_error("Unimplemented");
		}

		[[nodiscard]]
		bool done() const noexcept override {
			return m_decoder.done();
		}

		[[nodiscard]]
		std::vector<std::byte> readRawBytes(size_t count) override {
			std::vector<std::byte> out{};

			while (count > 0) {
				auto opckt = m_decoder.getNextPacket();
				if (!opckt)
					return out;

				if (m_block.decode(*opckt) && !m_state.blockIn(m_block))
					return out;

				while (true) {
					auto data = m_state.out(m_info);

					if (!data.empty() && data[0].size() > 0) {
						for (size_t i = 0; i < data[0].size(); ++i) {
							for (size_t c = 0; c < data.size(); ++c) {
								int val = int(std::floor(data[c][i] * 32767.0f + 0.5f));

								val = glm::clamp(val, -32768, 32767);

								val = utility::fromEndianess<std::endian::little>(val);

								auto bytes = std::bit_cast<std::array<std::byte, 2>>(int16_t(val));
								out.emplace_back(bytes[0]);
								out.emplace_back(bytes[1]);
							}
						}

						m_state.consumeSamples(int(data[0].size()));
						count -= std::min(count, data.size() * data[0].size() * sizeof(float));
					}
					else {
						break;
					}
				}
			}

			return out;
		}

		[[nodiscard]]
		ALaudioData readSamples(size_t count) override {
			ALaudioMeta meta{ getSampleRate(), determineALformat(16, short(getChannels())) };

			return ALaudioData{ std::move(meta), readRawBytes(count * getSampleAlignment() * getChannels()) };
		}
	};

	inline const bool _vorbisDecoderRegisterer = [](){
		return registerDecoder<VorbisDecoder>(".ogg");
	}();
}