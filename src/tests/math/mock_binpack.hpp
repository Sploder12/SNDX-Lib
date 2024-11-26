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
		MockT::callMock([](MockT& m) {
			m.mock_constructor();
		});
	}

	~ProxyBinPacker() {
		MockT::callMock([](MockT& m) {
			m.mock_destructor();
		});
	}

	void add(const IdT& id, size_t width, size_t height) {
		MockT::callMock([&](MockT& m) {
			m.add(id, width, height);
		});
	}

	MockT::Packing pack(size_t dimConstraint, size_t padding = 0) const {
		return MockT::callMock([&](MockT& m) {
			return m.pack(dimConstraint, padding);
		});
	}
};