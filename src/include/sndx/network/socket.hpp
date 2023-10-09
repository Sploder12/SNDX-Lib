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
		
		~Socket() {
			if (socket != invalidSocket) {
				context.close(socket);
			}
		}
	};

	class ClientSocket : public Socket {
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
					throw std::system_error(invalidSocket, std::system_category(), "Could Not Create Socket");
				}

				if (auto err = ::connect(this->socket, cur->ai_addr, cur->ai_addrlen)) {
					context.close(this->socket);
					continue;
				}
				break;
			}

			::freeaddrinfo(res);

			if (socket == invalidSocket) {
				throw std::system_error(invalidSocket, std::system_category(), "Could Not Connect To Server");
			}

			#else

			errno = 0;
			this->socket = ::socket(domain, type, protocolID);

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
				throw std::system_error(errno, std::system_category(), "Could Not Connect To Server");
			}

			#endif
		}
	};

	class HostSocket : public Socket {
	public:
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
				throw std::system_error(invalidSocket, std::system_category(), "Could Not Create Socket");
			}

			if (auto err = ::bind(this->socket, res->ai_addr, (int)res->ai_addrlen); err != 0) {
				::freeaddrinfo(res);
				throw std::system_error(err, std::system_category(), "Could Not Bind Socket");
			}

			::freeaddrinfo(res);

			#else

			errno = 0;
			this->socket = ::socket(domain, type, protocolID);

			if (this->socket < 0 || errno != 0) {
				throw std::system_error(errno, std::system_category(), "Could Not Create Socket");
			}

			if constexpr (std::endian::native != std::endian::big) {
				port = byteswap(port);
			}

			sockaddr_in addr{ domain, port, INADDR_ANY };
			::bind(this->socket, (const sockaddr*)(&addr), sizeof(addr));
			if (errno != 0) {
				throw std::system_error(errno, std::system_category(), "Could Not Bind Socket");
			}
			#endif
		}

		void listen(std::decay_t<decltype(queueSizeSocket)> queueSize = queueSizeSocket) {
			::listen(this->socket, queueSize);
		}
	};
}