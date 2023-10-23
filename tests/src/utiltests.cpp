#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "util.hpp"
#include <string_view>
#include <sstream>

namespace Util_Tests
{
	TEST_CLASS(String_Tests) {
	public:

		TEST_METHOD(TestStrip) {

			Assert::AreEqual(std::string_view(""), sndx::strip(""));
			Assert::AreEqual(std::string_view(""), sndx::strip("      "));
			Assert::AreEqual(std::string_view(""), sndx::strip("\t\t\t\t\t"));
			Assert::AreEqual(std::string_view(""), sndx::strip(" \t \t\t  \t"));
			Assert::AreEqual(std::string_view(""), sndx::strip("apple", "elap"));

			Assert::AreEqual(std::string_view("hello"), sndx::strip("hello"));
			Assert::AreEqual(std::string_view("hello"), sndx::strip("hello\t\t "));
			Assert::AreEqual(std::string_view("hello"), sndx::strip("  \t \t hello"));
			Assert::AreEqual(std::string_view("hello"), sndx::strip("  \t \t hello\t\t "));

			Assert::AreEqual(std::string_view("hel  \t lo"), sndx::strip("hel  \t lo"));
			Assert::AreEqual(std::string_view("hel  \t lo"), sndx::strip("hel  \t lo\t\t "));
			Assert::AreEqual(std::string_view("hel  \t lo"), sndx::strip(" \t\t\t hel  \t lo"));
			Assert::AreEqual(std::string_view("hel  \t lo"), sndx::strip("\t \thel  \t lo \t\t"));
		
			Assert::AreEqual(std::string_view("\thello "), sndx::strip("\thello ", ""));
		}

		TEST_METHOD(TestSplitFirst) {
			using p = std::pair<std::string_view, std::string_view>;

			Assert::IsTrue(p{ "", "" } == sndx::splitFirst("", ','));
			Assert::IsTrue(p{ "hello", "" } == sndx::splitFirst("hello", ','));
			Assert::IsTrue(p{ "hel", "lo" } == sndx::splitFirst("hel,lo", ','));
			Assert::IsTrue(p{ "hel", "lo,there" } == sndx::splitFirst("hel,lo,there", ','));
			Assert::IsTrue(p{ "", "hello" } == sndx::splitFirst(",hello", ','));

			Assert::IsTrue(p{ "", "" } == sndx::splitFirst(" \t\t   ", ','));
			Assert::IsTrue(p{ "hello", "" } == sndx::splitFirst("\thello \t\t", ','));
			Assert::IsTrue(p{ "hel", "lo" } == sndx::splitFirst("\t hel \t,\t\t lo", ','));
			Assert::IsTrue(p{ "hel", "lo,there" } == sndx::splitFirst("\t hel, \tlo,there  \t", ','));
			Assert::IsTrue(p{ "", "hello" } == sndx::splitFirst("\t\t   ,hello  \t", ','));
		}

		TEST_METHOD(TestSplit) {
			using vec = std::vector<std::string_view>;

			Assert::IsTrue(vec{} == sndx::splitStrip("", ','));
			Assert::IsTrue(vec{ "", "" } == sndx::splitStrip(",", ','));
			Assert::IsTrue(vec{ "hello" } == sndx::splitStrip("hello", ','));
			Assert::IsTrue(vec{ "hello", ""} == sndx::splitStrip("hello,", ','));
			Assert::IsTrue(vec{ "hel", "lo" } == sndx::splitStrip("hel,lo", ','));
			Assert::IsTrue(vec{ "hel", "lo", "there"} == sndx::splitStrip("hel,lo,there", ','));
			Assert::IsTrue(vec{ "", "hello" } == sndx::splitStrip(",hello", ','));

			Assert::IsTrue(vec{} == sndx::splitStrip(" \t\t   ", ','));
			Assert::IsTrue(vec{ "hello" } == sndx::splitStrip("\thello \t\t", ','));
			Assert::IsTrue(vec{ "hel", "lo" } == sndx::splitStrip("\t hel \t,\t\t lo", ','));
			Assert::IsTrue(vec{ "hel", "lo", "there" } == sndx::splitStrip("\t hel, \tlo,there  \t", ','));
			Assert::IsTrue(vec{ "", "hello" } == sndx::splitStrip("\t\t   ,hello  \t", ','));
		}


	};

	TEST_CLASS(Logging_Tests) {

	public:

		TEST_METHOD(TestBasicLog) {
			std::stringstream buf;

			sndx::Logger logger(buf.rdbuf());
			logger.setActive(true);

			logger << "This" << " is a log thing ";
			logger.log(":)");

			Assert::IsTrue("This is a log thing :)" == buf.str());
		}

		TEST_METHOD(TestBasicFormatLog) {
			std::stringstream buf;

			sndx::FormatLogger logger(buf.rdbuf());
			logger.setActive(true);

			logger.log("This");
			Assert::IsTrue("This" == buf.str());

			logger.formatter = [](std::string_view str) {
				return std::string(str) + " :)";
			};

			logger.log(" is a log thing");

			Assert::IsTrue("This is a log thing :)" == buf.str());

			logger << " no fmt!";
			Assert::IsTrue("This is a log thing :) no fmt!" == buf.str());
		}

		TEST_METHOD(TestActive) {
			std::stringstream buf;

			sndx::Logger logger(buf.rdbuf());
			logger.setActive(false);

			logger << "This is a log thing :(";

			Assert::IsTrue("" == buf.str());

			logger.setActive(true);

			logger << "This";
			Assert::IsTrue("This" == buf.str());

			logger.setActive(false);

			logger.log(" is a log thing :(");
			Assert::IsTrue("This" == buf.str());

			logger.setActive(true);

			logger.log(" is a log thing :)");
			Assert::IsTrue("This is a log thing :)" == buf.str());
		}

		TEST_METHOD(TestContext) {
			std::stringstream buf1, buf2;

			sndx::LoggingContext cntx{};

			sndx::Logger logger(buf1.rdbuf());
			cntx.activateLogger(logger);

			sndx::FormatLogger fLogger(buf2.rdbuf());
			cntx.activateLogger(fLogger);

			fLogger.formatter = [](std::string_view str) {
				return std::string(str) + " :)";
			};

			cntx.log("This is a log thing");

			Assert::IsTrue("This is a log thing" == buf1.str());
			Assert::IsTrue("This is a log thing :)" == buf2.str());
		}

	};
}
