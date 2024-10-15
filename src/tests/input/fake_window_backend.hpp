#pragma once

#include "input/window_backend.hpp"

using namespace sndx::input;

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <functional>
#include <string>

#include <glm/glm.hpp>

class FakeWindowHints : public WindowHints<FakeWindowHints> {
protected:
	friend WindowHints<FakeWindowHints>;

	std::unordered_map<std::string, int> hints{};

	static void restoreDefaultsImpl() {
		throw std::runtime_error("this is meaningless");
	}

	std::optional<int> getHintImpl(const std::string& hint) const {
		if (auto it = hints.find(hint); it != hints.end()) {
			return it->second;
		}

		return std::nullopt;
	}

	int getHintOrImpl(const std::string& hint, int val) const {
		return getHint(hint).value_or(val);
	}

	void setHintImpl(const std::string& hint, int val) {
		hints[hint] = val;
	}

	bool removeHintImpl(const std::string& hint) {
		return hints.erase(hint) > 0;
	}

	void applyImpl() const {

	}
};

class FakeWindow : public Window<FakeWindow> {
public:
	struct Viewport {
		glm::vec2 offset{}, size{};

		bool operator==(const Viewport& other) const {
			return offset == other.offset && size == other.size;
		}
	};

	friend Window<FakeWindow>;

public:
	mutable Viewport viewport{};
	Viewport tmpViewport{};
	glm::vec2 m_pos{}, m_size{};

	int hint{};
	int m_fake{};
	bool m_visible = false;

	FakeWindow(int width, int height, int fake, bool visible) {
		m_fake = fake;
		m_visible = visible;
		hint = 0x1337;
		m_size = glm::vec2{ width, height };
		viewport.size = m_size;
		tmpViewport = viewport;
	}

	FakeWindow(int width, int height, int fake, const FakeWindowHints& hints) {
		m_fake = fake;
		m_visible = bool(hints.getHintOr("visible", 0));
		hint = hints.getHintOr("hint", 0xDEAD);
		m_size = glm::vec2{ width, height };
		viewport.size = m_size;
		tmpViewport = viewport;
	}

private:
	void updateViewportImpl(std::function<void()> updateFn) const {
		updateFn();
		viewport = tmpViewport;
	};

	void bindImpl() const {
		// nothing
	};

	void setViewportOffsetImpl(const glm::vec2& offset) {
		tmpViewport.offset = offset;
	}

	void setViewportSizeImpl(const glm::vec2& size) {
		tmpViewport.size = size;
	}

	void setPositionImpl(const glm::vec2& pos) {
		m_pos = pos;
	}

	void resizeImpl(const glm::vec2& size, std::function<void()> updateFn) {
		updateViewportImpl(updateFn);
		m_size = size;
	}

	glm::vec2 getPositionImpl() const noexcept {
		return m_pos;
	}

	glm::vec2 getSizeImpl() const noexcept {
		return m_size;
	}

	Viewport getViewportImpl() const noexcept {
		return viewport;
	}

	glm::vec2 NDCtoPixImpl(const glm::vec2& ndc) const noexcept{
		return 1.0f - ndc;
	}

	glm::vec2 PixToNDCImpl(const glm::vec2& pix) const noexcept {
		return 1.0f - pix;
	}
};

class FakeWindowBuilder : public WindowBuilder<FakeWindowBuilder> {
protected:
	friend WindowBuilder<FakeWindowBuilder>;

	int m_fake{};

	FakeWindow buildImpl(bool visible) const {
		return FakeWindow(getWidth(), getHeight(), m_fake, visible);
	}

	FakeWindow buildImpl(const FakeWindowHints& hints) const {
		return FakeWindow(getWidth(), getHeight(), m_fake, hints);
	}

public:
	// useful to make sure CRTP is returning this class instead of itself.
	FakeWindowBuilder& setFakeAttrib(int fake) {
		m_fake = fake;
		return *this;
	}
};