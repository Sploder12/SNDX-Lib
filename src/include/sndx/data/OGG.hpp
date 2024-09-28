#pragma once

#include <ogg/ogg.h>

#include <stdexcept>
#include <optional>
#include <span>

namespace sndx::OGG {
	class Bitpack {
	protected:
		oggpack_buffer m_buffer;

	public:
		virtual ~Bitpack() noexcept = default;

		[[nodiscard]]
		oggpack_buffer* get() noexcept {
			return &m_buffer;
		}

		void advance(int bits = 1) {
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
		friend class Sync;
		friend class Stream;

		ogg_page m_page;

		explicit Page() noexcept = default;
	public:
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

		[[nodiscard]]
		void setChecksum() noexcept {
			return ogg_page_checksum_set(&m_page);
		}
	};

	class Packet {
	protected:
		ogg_packet m_packet;

	public:
		[[nodiscard]]
		ogg_packet* get() noexcept {
			return &m_packet;
		}
	};

	class Sync {
	protected:
		ogg_sync_state m_sync;

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

		template <size_t extent = std::dynamic_extent>
		bool write(std::span<char, extent> buf) noexcept {
			auto ptr = ogg_sync_buffer(&m_sync, buf.size());

			if (!ptr)
				return false;

			for (size_t i = 0; i < buf.size(); ++i) {
				ptr[i] = buf[i];
			}

			return ogg_sync_wrote(&m_sync, buf.size()) == 0;
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
		ogg_stream_state m_stream;

	public:
		[[nodiscard]]
		ogg_stream_state* get() noexcept {
			return &m_stream;
		}

		explicit Stream(int serialNumber) noexcept {
			ogg_stream_init(&m_stream, serialNumber);
		}

		~Stream() noexcept {
			ogg_stream_clear(&m_stream);
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
}