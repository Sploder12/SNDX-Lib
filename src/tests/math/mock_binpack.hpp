#pragma once

#include "../mock_utils.hpp"

#include "math/binpack.hpp"

template <class IdT = std::string>
struct MockBinPacker : public ProxyableMock<MockBinPacker<IdT>> {
	using Packing = sndx::math::Packing<IdT>;

	MOCK_METHOD(void, mock_constructor, ());
	MOCK_METHOD(void, mock_destructor, ());

	MOCK_METHOD(void, add, (const IdT&, size_t, size_t));
	MOCK_METHOD(Packing, pack, (size_t, size_t), (const));
};

template <class IdT = std::string>
struct ProxyBinPacker {
	using MockT = MockBinPacker<IdT>;

	ProxyBinPacker() {
		PROXY_CALL_MOCK(MockT, mock_constructor);
	}

	~ProxyBinPacker() {
		PROXY_CALL_MOCK(MockT, mock_destructor);
	}

	void add(const IdT& id, size_t width, size_t height) {
		PROXY_CALL_MOCK(MockT, add, id, width, height);
	}

	MockT::Packing pack(size_t dimConstraint, size_t padding = 0) const {
		return PROXY_CALL_MOCK(MockT, pack, dimConstraint, padding);
	}
};