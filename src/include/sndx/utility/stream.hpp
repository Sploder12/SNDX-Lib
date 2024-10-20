#pragma once

#include <cstdint>
#include <iostream>

namespace sndx::utility {
	struct MemoryBuf : public std::streambuf {
		MemoryBuf(uint8_t* begin, std::streamsize size) {
			setBuf(reinterpret_cast<char*>(begin), size);
		}

		MemoryBuf(std::nullptr_t, std::streamsize) = delete;

		std::streambuf* setBuf(char_type* begin, std::streamsize n) {
			this->setg(begin, begin, begin + n);
			this->setp(begin, begin + n);
			return this;
		}

		std::streambuf* setBuf(std::nullptr_t, std::streamsize) = delete;

		[[nodiscard]]
		std::streamsize showmanyc() override {
			auto p = std::streamsize(egptr() - gptr());
			
			return (p <= 0) ? -1 : p;
		}

		int_type underflow() override {
			return (gptr() == egptr()) ? traits_type::eof() : traits_type::to_int_type(*gptr());
		}

		std::streamsize xsputn(const char_type* s, std::streamsize n) override {
			std::streamsize count = std::min(n, epptr() - pptr());
			
			for (std::streamsize i = 0; i < count; ++i) {
				pptr()[i] = s[i];
			}

			pbump(int(count));
			return count;
		}

		int_type overflow(traits_type::int_type ch = traits_type::eof()) override {
			if (ch == traits_type::eof())
				return traits_type::not_eof(ch);

			if (pptr() == epptr())
				return traits_type::eof();

			pptr()[0] = traits_type::to_char_type(ch);
			return ch;
		}

		int_type pbackfail(int_type chr = traits_type::eof()) override {
			if (gptr() == eback() || (chr != traits_type::eof() && chr != gptr()[-1]))
				return traits_type::eof();

			auto ret = traits_type::to_int_type(*gptr());
			gbump(-1);
			return ret;
		}

		pos_type seekpos(pos_type pos, std::ios::openmode which) override {
			return seekoff(pos, std::ios::beg, which);
		}

		pos_type seekoff(off_type off, std::ios::seekdir dir, std::ios::openmode which) override {
			off_type newOffsetg = off;
			off_type newOffsetp = off;

			switch (dir) {
			case std::ios::cur:
				newOffsetg += off_type(gptr() - eback());
				newOffsetp += off_type(pptr() - eback());
				break;

			case std::ios::beg:
				break;

			case std::ios::end:
				newOffsetg += off_type(egptr() - eback());
				newOffsetp += off_type(epptr() - eback());
				break;

			default:
				throw std::invalid_argument("Seekoff dir invalid");
			}

			bool worked = false;
			
			if (which & std::ios::in) {
				if (newOffsetg < 0 || newOffsetg > off_type(egptr() - eback()))
					return pos_type(off_type(-1));

				setg(eback(), eback() + newOffsetg, egptr());
				worked = true;
			}

			if (which & std::ios::out) {
				if (newOffsetp < 0 || newOffsetp > off_type(epptr() - eback()))
					return pos_type(off_type(-1));

				setp(eback(), epptr());
				pbump(int(newOffsetp));
				worked = true;
			}
			
			if (worked) {
				return gptr() - eback();
			}

			return pos_type(off_type(-1));
		}
	};

	class MemoryStream : public std::iostream {
	private:
		MemoryBuf m_buffer;

	public:
		MemoryStream(uint8_t* begin, size_t size) :
			std::iostream(&m_buffer),
			m_buffer(begin, size) {

			rdbuf(&m_buffer);
		}

		MemoryStream(std::nullptr_t, size_t) = delete;
	};
}