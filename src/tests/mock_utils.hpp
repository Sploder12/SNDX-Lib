#pragma once

#include <gmock/gmock.h>

#include <stdexcept>

struct MockError : public std::logic_error {
	using std::logic_error::logic_error;
};

template <class Mock>
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

	template <class Func, class... Ts>
	static decltype(auto) callMock(Func&& func) {
		if (mock) {
			return func(*mock);
		}
		
		throw MockError("Tried to call proxy mock that isn't mocked!");
	}
};