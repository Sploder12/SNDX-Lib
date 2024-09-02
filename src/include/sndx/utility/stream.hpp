#pragma once

#include <iostream>

namespace sndx::utility {
	struct MemoryViewBuf : public std::streambuf {
		MemoryViewBuf(const uint8_t* begin, std::streamsize size) {
			setBuf(reinterpret_cast<const char*>(begin), size);
		}

		MemoryViewBuf(std::nullptr_t, std::streamsize) = delete;

		std::streambuf* setBuf(const char_type* s, std::streamsize n) {
			auto begin = const_cast<char_type*>(s);
			this->setg(begin, begin, begin + n);
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

		int_type pbackfail(int_type chr) {
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
			if (which & std::ios::in) {
				off_type newOffset = off;
				switch (dir) {
					case std::ios::cur:
						newOffset += off_type(gptr() - eback());
						break;

					case std::ios::beg:
						break;

					case std::ios::end:
						newOffset += off_type(egptr() - eback());
						break;

					default:
						throw std::invalid_argument("Seekoff dir invalid");
				}

				if (newOffset < 0 || newOffset > off_type(egptr() - eback()))
					return pos_type(off_type(-1));

				setg(eback(), eback() + newOffset, egptr());
				return gptr() - eback();
			}
			
			return pos_type(off_type(-1));
		}
	};

	class MemoryIStream : public std::istream {
	private:
		MemoryViewBuf m_buffer;

	public:
		MemoryIStream(const uint8_t* begin, size_t size):
			std::istream(&m_buffer),
			m_buffer(begin, size) {

			rdbuf(&m_buffer);
		}

		MemoryIStream(std::nullptr_t, size_t) = delete;
	};
}