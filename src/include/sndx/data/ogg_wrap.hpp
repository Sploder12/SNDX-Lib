#pragma once

#include <ogg/ogg.h>

#include "../data/serialize.hpp"

#include <stdexcept>
#include <optional>
#include <span>

#ifndef OGG_STREAM_READ_SIZE
#define OGG_STREAM_READ_SIZE 4096
#endif

namespace sndx::ogg {
	class Bitpack {
	protected:
		oggpack_buffer m_buffer{};

	public:
		virtual ~Bitpack() noexcept = default;

		[[nodiscard]]
		oggpack_buffer* get() noexcept {
			return &m_buffer;
		}

		void advance(int bits = 1) noexcept {
			oggpack_adv(&m_buffer, bits);
		}

		[[nodiscard]]
		auto look(int bits = 1) {
			if (bits < 0 || bits > 32)
				throw std::invalid_argument("look can only handle [0, 32] bits");

			return oggpack_look(&m_buffer, bits);
		}

		[[nodiscard]]
		auto bytes() noexcept {
			return oggpack_bytes(&m_buffer);
		}

		[[nodiscard]]
		auto bitCount() noexcept {
			return oggpack_bits(&m_buffer);
		}

		[[nodiscard]]
		auto getBuffer() noexcept {
			return oggpack_get_buffer(&m_buffer);
		}
	};

	class ReadBitpack : public Bitpack {
	public:
		template <size_t extent = std::dynamic_extent>
		explicit ReadBitpack(std::span<unsigned char, extent> buf) noexcept {
			oggpack_readinit(&m_buffer, buf.data(), buf.size());
		}

		[[nodiscard]]
		auto read(int bits = 1) {
			if (bits < 0 || bits > 32)
				throw std::invalid_argument("read can only handle [0, 32] bits");

			return oggpack_read(&m_buffer, bits);
		}
	};

	class WriteBitpack : public Bitpack {
	public:
		explicit WriteBitpack() noexcept {
			oggpack_writeinit(&m_buffer);
		}

		~WriteBitpack() noexcept {
			oggpack_writeclear(&m_buffer);
		}

		[[nodiscard]]
		bool check() noexcept {
			return oggpack_writecheck(&m_buffer) == 0;
		}

		template <size_t extent = std::dynamic_extent>
		void copy(std::span<unsigned char, extent> source, long bits) {
			if (bits < 0 || bits > source.size() * 8) [[unlikely]]
				throw std::invalid_argument("source has less bits than bits argument");

			// note: the API spec says this can only write 32 bits at a time, but the source disagrees
			oggpack_writecopy(&m_buffer, source.data(), source.size());
		}

		void align() noexcept {
			oggpack_writealign(&m_buffer);
		}

		void truncate(long bits) {
			if (bits < 0 || bits > bitCount())
				throw std::invalid_argument("truncate supplied invalid bit count");

			// note: this function is very dangerous
			oggpack_writetrunc(&m_buffer, bits);
		}

		void write(unsigned long value, int bits) {
			if (bits < 0 || bits > 32)
				throw std::invalid_argument("write can only handle [0, 32] bits");

			oggpack_write(&m_buffer, value, bits);
		}
	};

	class Page {
	protected:
		ogg_page m_page{};

	public:
		explicit Page() noexcept = default;

		[[nodiscard]]
		ogg_page* get() noexcept {
			return &m_page;
		}

		[[nodiscard]]
		int version() const noexcept {
			return ogg_page_version(&m_page);
		}

		[[nodiscard]]
		bool continued() const noexcept {
			return ogg_page_continued(&m_page) == 1;
		}

		[[nodiscard]]
		int packets() const noexcept {
			return ogg_page_packets(&m_page);
		}

		[[nodiscard]]
		bool atBegin() const noexcept {
			return ogg_page_bos(&m_page) > 0;
		}

		[[nodiscard]]
		bool atEnd() const noexcept {
			return ogg_page_eos(&m_page) > 0;
		}

		[[nodiscard]]
		ogg_int64_t granulePos() const noexcept {
			return ogg_page_granulepos(&m_page);
		}

		[[nodiscard]]
		int serialNumber() const noexcept {
			return ogg_page_serialno(&m_page);
		}

		[[nodiscard]]
		long pageNumber() const noexcept {
			return ogg_page_pageno(&m_page);
		}

		void setChecksum() noexcept {
			return ogg_page_checksum_set(&m_page);
		}
	};

	class Packet {
	protected:
		ogg_packet m_packet{};

	public:
		[[nodiscard]]
		ogg_packet* get() noexcept {
			return &m_packet;
		}
	};

	class Sync {
	protected:
		ogg_sync_state m_sync{};

	public:
		[[nodiscard]]
		ogg_sync_state* get() noexcept {
			return &m_sync;
		}

		explicit Sync() noexcept {
			ogg_sync_init(&m_sync);
		}

		~Sync() noexcept {
			ogg_sync_clear(&m_sync);
		}

		[[nodiscard]]
		bool check() noexcept {
			return ogg_sync_check(&m_sync) == 0;
		}

		bool reset() noexcept {
			return ogg_sync_reset(&m_sync) == 0;
		}

		// WriteFn is a function callable with std::span<char, dynamic_extent> as argument,
		// if the return is can be converted to long, that much is repoted as witten
		// WriteFn should attempt to write size bytes into the argument buffer
		// returns amount written or -1 on error
		template <class WriteFn>
		long write(size_t size, WriteFn writeFn) {
			auto ptr = ogg_sync_buffer(&m_sync, long(size));

			if (!ptr)
				return -1;

			std::span<char> buffer{ ptr, size };

			using fnRet = decltype(writeFn(buffer));

			if constexpr (!std::is_convertible_v<fnRet, long>) {
				writeFn(buffer);
				return long((ogg_sync_wrote(&m_sync, long(size)) == 0) ? size : -1);
			}
			else {
				long wrote = long(std::max(fnRet(0), writeFn(buffer)));
				return (ogg_sync_wrote(&m_sync, wrote) == 0) ? wrote : -1;
			}
		}

		template <size_t extent = std::dynamic_extent>
		long write(std::span<char, extent> buf) noexcept {
			return write(buf.size(), [buf](std::span<char> out) {
				for (size_t i = 0; i < out.size(); ++i) {
					out[i] = buf[i];
				}
			});
		}

		int seek(Page& page) noexcept {
			return ogg_sync_pageseek(&m_sync, page.get());
		}

		[[nodiscard]]
		std::pair<Page, int> pageout() noexcept {
			Page out{};
			int ret = ogg_sync_pageout(&m_sync, out.get());

			return std::make_pair(std::move(out), ret);
		}
	};

	class Stream {
	protected:
		ogg_stream_state m_stream{};
		bool init = false;

	public:
		[[nodiscard]]
		ogg_stream_state* get() noexcept {
			return &m_stream;
		}

		explicit Stream() noexcept = default;
		
		explicit Stream(int serialNumber) {
			if (ogg_stream_init(&m_stream, serialNumber) != 0)
				throw std::runtime_error("Could not init ogg stream");

			init = true;
		}

		Stream& operator=(Stream&& other) noexcept {
			std::swap(m_stream, other.m_stream);
			std::swap(init, other.init);
			return *this;
		}

		~Stream() noexcept {
			if (init) {
				ogg_stream_clear(&m_stream);
				init = false;
			}
		}

		bool reset() noexcept {
			return ogg_stream_reset(&m_stream) == 0;
		}

		bool resetSerialNumber(int serialNumber) noexcept {
			return ogg_stream_reset_serialno(&m_stream, serialNumber);
		}

		bool pageIn(Page& page) noexcept {
			return ogg_stream_pagein(&m_stream, page.get()) == 0;
		}

		/* Decoding */

		[[nodiscard]]
		std::pair<Packet, int> packetOut() noexcept {
			Packet out{};
			auto ret = ogg_stream_packetout(&m_stream, out.get());

			return std::make_pair(std::move(out), ret);
		}

		[[nodiscard]]
		std::pair<Packet, int> packetPeek() noexcept {
			Packet out{};
			auto ret = ogg_stream_packetpeek(&m_stream, out.get());

			return std::make_pair(std::move(out), ret);
		}

		[[nodiscard]]
		bool packetAvailable() noexcept {
			return ogg_stream_packetpeek(&m_stream, nullptr) == 1;
		}

		/* Encoding */

		bool packetIn(Packet& packet) noexcept {
			return ogg_stream_packetin(&m_stream, packet.get()) == 0;
		}

		[[nodiscard]]
		std::optional<Page> pageOut() noexcept {
			Page out{};

			if (ogg_stream_pageout(&m_stream, out.get()) == 0)
				return std::nullopt;
				
			return out;
		}

		[[nodiscard]]
		std::optional<Page> pageOut(int fillBytes) noexcept {
			Page out{};

			if (ogg_stream_pageout_fill(&m_stream, out.get(), fillBytes) == 0)
				return std::nullopt;

			return out;
		}

		std::optional<Page> flush() noexcept {
			Page out{};

			if (ogg_stream_flush(&m_stream, out.get()) == 0)
				return std::nullopt;

			return out;
		}

		std::optional<Page> flush(int fillBytes) noexcept {
			Page out{};

			if (ogg_stream_flush_fill(&m_stream, out.get(), fillBytes) == 0)
				return std::nullopt;

			return out;
		}
	};


	class Decoder {
	protected:
		std::istream m_in;

		ogg::Sync m_sync{};
		ogg::Stream m_stream;
		ogg::Page m_curPage;

		bool m_done = false;
	public:

		[[nodiscard]]
		bool done() const noexcept {
			return m_done;
		}

		[[nodiscard]]
		std::optional<ogg::Packet> getNextPacket() {
			int ret = 0;
			while (ret == 0) {
				auto [packet, packetRet] = m_stream.packetOut();
				ret = packetRet;

				if (ret < 0) {
					m_done = true;
					return std::nullopt;
				}

				if (ret != 0)
					return packet;

				// more data!
				do {
					auto wrote = m_sync.write(OGG_STREAM_READ_SIZE, [this](std::span<char> buf) {
						m_in.read(buf.data(), buf.size());
						return m_in.gcount();
					});

					if (wrote <= 0) {
						m_done = true;
						return std::nullopt;
					}

					auto [nextPage, pageRet] = m_sync.pageout();

					if (pageRet < 0) {
						m_done = true;
						return std::nullopt;
					}

					if (pageRet != 0) {
						m_curPage = std::move(nextPage);

						if (!m_stream.pageIn(m_curPage)) {
							m_done = true;
							return std::nullopt;
						}
					}

					break;
				} while (true);
			}

			return std::nullopt;
		}

		explicit Decoder(std::istream& stream) :
			m_in(stream.rdbuf()) {

			while (true) {
				auto wrote = m_sync.write(OGG_STREAM_READ_SIZE, [this](std::span<char> buf) {
					m_in.read(buf.data(), buf.size());
					return m_in.gcount();
				});

				if (wrote <= 0) {
					m_done = true;
					throw deserialize_error("Ogg not enough data");
				}

				auto [page, ret] = m_sync.pageout();

				if (ret == 1) {
					m_curPage = std::move(page);
					break;
				}
			}

			m_stream = Stream(m_curPage.serialNumber());
			
			if (!m_stream.pageIn(m_curPage)) {
				m_done = true;
				throw deserialize_error("First page in failed");
			}
		}
	};
}