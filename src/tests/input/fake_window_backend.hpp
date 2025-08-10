#pragma once

#include "input/window_backend.hpp"

using namespace sndx::input;

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <functional>
#include <string>

#include <glm/glm.hpp>

class FakeWindow : public Window<FakeWindow> {
public:
	friend Window<FakeWindow>;

public:
	glm::vec2 m_pos{}, m_size{};

	std::string m_title{};
	int hint{};
	int m_fake{};
	bool m_visible = false;
	mutable int m_binds = 0;

	FakeWindow(std::string_view title, int width, int height, int fake, bool visible) {
		m_title = title;
		m_fake = fake;
		m_visible = visible;
		hint = 0x1337;
		m_size = glm::vec2{ width, height };
	}

private:
	void bindImpl() const {
		++m_binds;
	};

	void setPositionImpl(const glm::vec2& pos) {
		m_pos = pos;
	}

	void resizeImpl(const glm::vec2& size) {
		m_size = size;
	}

	void focusWindowImpl() {

	}

	void requestAttentionImpl() {

	}

	void setVisibilityImpl(bool visible) {
		m_visible = visible;
	}

	void tryCloseImpl() {

	}

	glm::vec2 getPositionImpl() const noexcept {
		return m_pos;
	}

	glm::vec2 getSizeImpl() const noexcept {
		return m_size;
	}

	bool isVisibleImpl() const noexcept {
		return m_visible;
	}

	const std::string& getTitleImpl() const noexcept {
		return m_title;
	}
};

class FakeWindowBuilder : public WindowBuilder<FakeWindowBuilder> {
protected:
	friend WindowBuilder;

	bool m_visible = false;
	int m_fake{};

	[[nodiscard]]
	FakeWindow buildImpl() const {
		return {getTitle(), getWidth(), getHeight(), m_fake, m_visible};
	}

public:
	// useful to make sure CRTP is returning this class instead of itself.
	FakeWindowBuilder& setFakeAttrib(int fake) {
		m_fake = fake;
		return *this;
	}

	FakeWindowBuilder& setVisible(bool visible) {
		m_visible = visible;
		return *this;
	}
};