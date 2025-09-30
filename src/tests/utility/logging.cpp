#include "utility/logging.hpp"

#include <gtest/gtest.h>

#include "../mock_utils.hpp"

using namespace sndx::utility;

struct MockLogger : Logger {
	MOCK_METHOD(void, log_impl, (LogLevelT level, std::string&& str), (override));
};

TEST(Logging, logLogs) {
	MockLogger logger{};

	EXPECT_CALL(logger, log_impl(LogLevel::Error, testing::StrEq("Warning: 123")));

	logger.log(LogLevel::Error, "{} {}", "Warning:", 123);
}

TEST(Logging, vlogLogs) {
	MockLogger logger{};

	EXPECT_CALL(logger, log_impl(LogLevel::Error, testing::StrEq("Warning: 123")));

	std::string vfmt = "{}";
	vfmt += "{}";

	logger.vlog(LogLevel::Error, vfmt, "Warning: ", 123);
}

TEST(Logging, logLevelIsRespected) {
	MockLogger logger{};

	EXPECT_CALL(logger, log_impl(5, testing::StrEq("banana")));
	EXPECT_CALL(logger, log_impl(6, testing::StrEq("apple")));

	logger.setLevel(5);

	logger.log(5, "banana");
	logger.log(6, "apple");
	logger.log(4, "square");
}

TEST(Logging, lazyArgIsLazy) {
	MockLogger logger{};

	logger.setLevel(0);

	size_t i = 0;

	auto func = [&i]() {
		++i;
		return "hi";
	};

	EXPECT_CALL(logger, log_impl(1, testing::StrEq("hi")));

	logger.log(1, "{}", LazyArg{func});
	logger.log(-1, "{}", LazyArg{func});

	EXPECT_EQ(i, 1);
}

TEST(Logging, makeLazyArgWorks) {
	MockLogger logger{};

	logger.setLevel(0);

	size_t i = 0;
	auto func = [&](size_t b) {
		i += b;
		return i;
	};

	EXPECT_CALL(logger, log_impl(1, testing::StrEq("6")));

	auto lazy = SNDX_MAKE_LAZY(func(func(3)));
	EXPECT_EQ(i, 0);

	logger.log(1, "{}", lazy);
	EXPECT_EQ(i, 6);
}
