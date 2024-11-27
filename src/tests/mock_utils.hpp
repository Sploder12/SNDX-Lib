#pragma once

#include <gmock/gmock.h>

#include <stdexcept>

struct MockError : public std::logic_error {
	using std::logic_error::logic_error;
};

// CRTP mixin that enables proxying for a mock.
// the size_t allows for having multiple proxys of a Mock at a time
template <class Mock, size_t ProxyID = 0>
class ProxyableMock {
private:
	inline static Mock* mock{ nullptr };

	static auto inject(Mock* newMock) noexcept {
		return std::exchange(mock, newMock);
	}
public:
	ProxyableMock() {
		if (inject(static_cast<Mock*>(this)) != nullptr)
			throw MockError("Attempting to proxy mock multiple times!");
	}

	~ProxyableMock() {
		inject(nullptr);
	}

	ProxyableMock(const ProxyableMock&) = delete;
	ProxyableMock(ProxyableMock&&) = delete;
	ProxyableMock& operator=(const ProxyableMock&) = delete;
	ProxyableMock& operator=(ProxyableMock&&) = delete;

	template <class Func>
	static decltype(auto) callMock(Func&& func) {
		if (mock) {
			return func(*mock);
		}
		
		throw MockError("Tried to call proxy mock that isn't mocked!");
	}

	[[nodiscard]]
	static constexpr size_t proxyID() noexcept {
		return ProxyID;
	}
};

#define PROXY_CALL_MOCK(MockT, method, ...) \
	MockT::callMock([&](MockT& _proxmock) { \
		return _proxmock.method(__VA_ARGS__); \
	})
