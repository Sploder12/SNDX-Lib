#pragma once

#include <gtest/gtest.h>

#ifndef SNDX_TEST_WEIGHT_LIMIT
#define SNDX_TEST_WEIGHT_LIMIT 1337
#endif

enum class TestWeight : size_t {
	None = 0,
	BasicUnit = 1, // quick, nothing evil
	Unit = 2, // evil unit tests
	BasicIntegration = 3, // quick API calls, no writing to disk
	Integration = 4, // full API calls, might write to disk
	
	All // EVERY test
};

template <TestWeight weight>
constexpr auto weight_desc() {
	if constexpr (weight == TestWeight::None) {
		return "No tests are enabled.";
	}
	else if constexpr (weight == TestWeight::BasicUnit) {
		return "Only basic unit tests enabled.";
	}
	else if constexpr (weight == TestWeight::Unit) {
		return "Only unit tests enabled.";
	}
	else if constexpr (weight == TestWeight::BasicIntegration) {
		return "Only basic integration tests enabled.";
	}
	else if constexpr (weight == TestWeight::Integration) {
		return "Only integration tests enabled.";
	}
	else {
		return "All tests enabled";
	}
}

template <TestWeight weight>
constexpr auto weight_skip_msg() {
	if constexpr (weight == TestWeight::None) {
		return "Everything is disabled?";
	}
	else if constexpr (weight == TestWeight::BasicUnit) {
		return "Unit tests are disabled.";
	}
	else if constexpr (weight == TestWeight::Unit) {
		return "Heavy unit tests are disabled.";
	}
	else if constexpr (weight == TestWeight::BasicIntegration) {
		return "Integration tests are disabled.";
	}
	else if constexpr (weight == TestWeight::Integration) {
		return "Heavy integration tests are disabled.";
	}
	else {
		return "All tests enabled?";
	}
}

template <TestWeight weight = TestWeight::All>
void set_test_weight() {
	if constexpr (size_t(weight) > SNDX_TEST_WEIGHT_LIMIT) {
		GTEST_SKIP() << weight_skip_msg<weight>();
	}
}