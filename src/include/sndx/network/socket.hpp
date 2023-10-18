#pragma once

#ifdef _WIN32

#include <winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#endif

#include <utility>
#include <system_error>
#include <string>
#include <concepts>
#include <bit>
#include <array>
#include <algorithm>
#include <type_traits>

#include <vector>

namespace sndx {

	[[nodiscard]] // C++23's byteswap
	inline constexpr auto byteswap(std::integral auto v) {
		static_assert(std::has_unique_object_representations_v<decltype(v)>);

		auto val = std::bit_cast<std::array<std::byte, sizeof(decltype(v))>>(v);
		std::reverse(val.begin(), val.end());
		return std::bit_cast<decltype(v)>(val);
	}

	enum class Protocol {
		TCP, UDP
	};

	#ifdef _WIN32
	using SocketType = SOCKET;
	static constexpr auto invalidSocket = INVALID_SOCKET;
	static constexpr auto queueSizeSocket = SOMAXCONN;
	#else
	using SocketType = int;
	static constexpr auto invalidSocket = -1;
	static constexpr auto queueSizeSocket = 4096;
	#endif

	struct _socketContext {
		#ifdef _WIN32
		WSAData context;

		_socketContext() {
			if (auto err = WSAStartup(MAKEWORD(2, 2), &context); err != 0) {
				throw std::system_error(err, std::system_category(), "WSAStartup Error");
			}
		}

		~_socketContext() {
			WSACleanup();
		}

		static void close(SocketType& socket) {
			::closesocket(socket);
			socket = invalidSocket;
		}

		[[nodiscard]]
		static auto protocol(Protocol pro) {
			switch (pro) {
			case sndx::Protocol::TCP:
				return IPPROTO_TCP;
			case sndx::Protocol::UDP:
				return IPPROTO_UDP;
			default:
				return IPPROTO_TCP;
			}
		}

		#else
		static void close(SocketType& socket) {
			::close(socket);
			socket = invalidSocket;
		}

		[[nodiscard]]
		static auto protocol(Protocol pro) {
			switch (pro) {
			case sndx::Protocol::TCP:
				if (auto ipptr = getprotobyname("tcp"); ipptr) {
					return ipptr->p_proto;
				}
				return 0;
			case sndx::Protocol::UDP:
				if (auto ipptr = getprotobyname("udp"); ipptr) {
					return ipptr->p_proto;
				}
				return 0;
			default:
				return 0;
			}
		}

		#endif
	};



	class Socket {
	protected:
		static const _socketContext context;
		SocketType socket;

		Socket(SocketType socket) :
			socket(socket) {}

	public:
		Socket() noexcept :
			socket(invalidSocket) {}

		Socket(Socket&& other) noexcept :
			socket(std::exchange(other.socket, invalidSocket)) {}

		Socket& operator=(Socket&& other) noexcept {
			std::swap(socket, other.socket);
			return *this;
		}

		bool alive() const {
			return socket != invalidSocket;
		}
		
		~Socket() {
			if (socket != invalidSocket) {
				context.close(socket);
			}
		}
	};

	template <bool Blocks = true>
	class ClientSocket : public Socket {
	protected:
		explicit ClientSocket(SocketType socket) :
			Socket(socket) {}

		template <bool T>
		friend class HostSocket;
	
	public:
		static constexpr bool Blocking = Blocks;

		// Warning, check if alive, connecting may fail without throwing.
		ClientSocket(const std::string& addr, unsigned short port, int domain = AF_INET, int type = SOCK_STREAM, Protocol protocol = Protocol::TCP) {
			auto protocolID = context.protocol(protocol);

			#ifdef _WIN32
			addrinfo hints{};
			addrinfo* res = nullptr;

			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = type;
			hints.ai_protocol = protocolID;

			if (auto err = getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &res); err != 0) {
				throw std::system_error(err, std::system_category(), "Could Not Create Socket Address");
			}

			auto cur = res;
			for (; cur; cur = cur->ai_next) {
				this->socket = ::socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
				if (this->socket == invalidSocket) {
					::freeaddrinfo(res);
					throw std::system_error(-1, std::system_category(), "Could Not Create Socket");
				}

				if (auto err = ::connect(this->socket, cur->ai_addr, (int)cur->ai_addrlen)) {
					context.close(this->socket);
					continue;
				}
				break;
			}

			::freeaddrinfo(res);

			if constexpr (!Blocking) {
				if (this->socket != invalidSocket) {
					u_long mode = 1;
					::ioctlsocket(this->socket, FIONBIO, &mode);
				}
			}

			#else

			errno = 0;

			if constexpr (Blocking) {
				this->socket = ::socket(domain, type, protocolID);
			}
			else {
				this->socket = ::socket(domain, type | SOCK_NONBLOCK, protocolID);
			}

			if (this->socket < 0 || errno != 0) {
				throw std::system_error(errno, std::system_category(), "Could Not Create Socket");
			}

			if constexpr (std::endian::native != std::endian::big) {
				port = byteswap(port);
			}

			sockaddr_in saddr{ domain, port, INADDR_ANY };

			::inet_pton(domain, addr.c_str(), &saddr.sin_addr);
			if (errno != 0) {
				throw std::system_error(errno, std::system_category(), "Could Resolve Socket Address");
			}

			::connect(socket, (const sockaddr*)&saddr, sizeof(sadder));
			if (errno != 0) {
				socket = invaildSocket;
			}

			#endif
		}

		// blocks until some data is recieved, check alive after calling, connection may drop
		[[nodiscard]]
		std::vector<char> recieve(size_t count, int flags = 0) {
			std::vector<char> out{};
			out.resize(count);

			auto amount = ::recv(this->socket, out.data(), count * sizeof(decltype(out)::value_type), flags);

			#ifdef _WIN32
			if (amount == SOCKET_ERROR) {
				auto err = WSAGetLastError();
				if constexpr (!Blocking) {
					if (err == WSAEWOULDBLOCK) {
						return {};
					}
				}

				if (err == WSAECONNABORTED || err == WSAETIMEDOUT || err == WSAECONNRESET) {
					context.close(this->socket);
					return {};
				}
				
				throw std::system_error(err, std::system_category(), "Socket Recieve Failed");
			}
			#else
			if (amount == -1) {
				if constexpr (!Blocking) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						return {};
					}
				}

				context.close(this->socket);
				return {};
			}
			#endif

			out.resize(amount);
			return out;
		}

		[[nodiscard]]
		std::vector<char> recieveExactly(size_t count, int flags = 0) {
			std::vector<char> out{};
			out.resize(count);
			
			size_t recvd = 0;

			while (recvd < count) {
				if (!this->alive()) {
					break;
				}

				auto tmp = recieve(count - recvd, flags);

				size_t iters = std::min(tmp.size(), count - recvd);
				for (size_t i = 0; i < iters; ++i) {
					out[recvd + i] = tmp[i];
				}

				recvd += iters;
			}

			return out;
		}

		// check alive after calling, connection may die
		template <class T> [[nodiscard]]
		auto send(const T& buffer, int flags = 0, size_t offset = 0) {
			size_t len = buffer.size() * sizeof(T::value_type) - offset;

			if (offset >= buffer.size() * sizeof(T::value_type)) [[unlikely]]
				return 0;

			auto res = ::send(this->socket, (const char*)(buffer.data()) + offset, len, flags);

			#ifdef _WIN32
			if (res == SOCKET_ERROR) {
				auto err = WSAGetLastError();
				if constexpr (!Blocking) {
					if (err == WSAEWOULDBLOCK) {
						return 0;
					}
				}

				if (err == WSAECONNABORTED || err == WSAETIMEDOUT || err == WSAECONNRESET) {
					context.close(this->socket);
					return -1;
				}

				throw std::system_error(err, std::system_category(), "Socket Send Failed");
			}

			#else
			if (res == -1) {
				if constexpr (!Blocking) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						return 0;
					}
				}

				context.close(this->socket);
				return -1;
			}

			#endif

			return res;
		}

		// sends the entire buffer (pretty much guarenteed to block)
		template <class T>
		bool sendAll(const T& buffer, int flags = 0) {
			size_t loc = 0;
			size_t len = buffer.size() * sizeof(T::value_type);

			while (loc < len) {
				auto res = send(buffer, flags, loc);
				if (res == -1)
					return false;

				loc += res;
			}

			return true;
		}
	};

	template <bool Blocks = true>
	class HostSocket : public Socket {
	public:

		static constexpr bool Blocking = Blocks;

		// Warning, check if alive, binding may fail without throwing.
		HostSocket(unsigned short port, int domain = AF_INET, int type = SOCK_STREAM, Protocol protocol = Protocol::TCP) {
			auto protocolID = context.protocol(protocol);

			#ifdef _WIN32
			addrinfo hints{};
			addrinfo* res = nullptr;

			hints.ai_family = domain;
			hints.ai_socktype = type;
			hints.ai_protocol = protocolID;
			hints.ai_flags = AI_PASSIVE;

			if (auto err = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &res); err != 0) {
				throw std::system_error(err, std::system_category(), "Could Not Create Socket Address");
			}

			this->socket = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (this->socket == invalidSocket) {
				::freeaddrinfo(res);
				throw std::system_error(-1, std::system_category(), "Could Not Create Socket");
			}

			auto err = ::bind(this->socket, res->ai_addr, (int)res->ai_addrlen);
			if (err != 0 && err != WSAEADDRINUSE) {
				::freeaddrinfo(res);
				throw std::system_error(err, std::system_category(), "Could Not Bind Socket");
			}

			// address in use is not exceptional, expected to a degree
			if (err == WSAEADDRINUSE) {
				this->socket = invalidSocket;
			}

			::freeaddrinfo(res);

			if constexpr (!Blocking) {
				if (this->socket != invalidSocket) {
					u_long mode = 1;
					::ioctlsocket(this->socket, FIONBIO, &mode);
				}
			}

			#else

			errno = 0;

			if constexpr (Blocking) {
				this->socket = ::socket(domain, type, protocolID);
			}
			else {
				this->socket = ::socket(domain, type | SOCK_NONBLOCK, protocolID);
			}

			if (this->socket < 0 || errno != 0) {
				throw std::system_error(errno, std::system_category(), "Could Not Create Socket");
			}

			if constexpr (std::endian::native != std::endian::big) {
				port = byteswap(port);
			}

			sockaddr_in addr{ domain, port, INADDR_ANY };
			::bind(this->socket, (const sockaddr*)(&addr), sizeof(addr));
			if (errno != 0 && errno != EADDRINUSE) {
				throw std::system_error(errno, std::system_category(), "Could Not Bind Socket");
			}

			// address in use is not exceptional, expected to a degree
			if (errno == EADDRINUSE) {
				this->socket = invalidSocket;
			}

			#endif
		}

		// please call this before calling accept
		auto listen(std::decay_t<decltype(queueSizeSocket)> queueSize = queueSizeSocket) const {
			return ::listen(this->socket, queueSize);
		}

		// this will block, check the returned socket for being alive, accept may fail without exception
		[[nodiscard]]
		ClientSocket<Blocking> accept() const {
			

			#ifdef _WIN32
			auto outs = ::accept(this->socket, nullptr, nullptr);
			#else
			SocketType outs;
			if constexpr (Blocking) {
				outs = ::accept(this->socket, nullptr, nullptr);
			}
			else {
				outs = ::accept4(this->socket, nullptr, nullptr, SOCK_NONBLOCK);
			}
			#endif

			if (outs == invalidSocket) {
				#ifdef _WIN32
				if (auto err = WSAGetLastError(); err != WSAECONNRESET && err != WSAEWOULDBLOCK) {
					throw std::system_error(err, std::system_category(), "Socket Accept Failed");
				}
				#else
				if (errno != ECONNABORTED && errno != EAGAIN && errno != EWOULDBLOCK) {
					throw std::system_error(errno, std::system_category(), "Socket Accept Failed");
				}
				#endif
				return ClientSocket<Blocking>(invalidSocket);
			}

			return ClientSocket<Blocking>(outs);
		}
	};

	// these are to ensure that ::select doesn't need to do any allocation
	static_assert(sizeof(Socket) == sizeof(SocketType));
	static_assert(sizeof(ClientSocket<>) == sizeof(SocketType));
	static_assert(sizeof(HostSocket<>) == sizeof(SocketType));
}