#pragma once

#include <iostream>
#include <atomic>
#include <functional>
#include <string_view>

#include <vector>
#include <mutex>

#include <algorithm>

namespace sndx {

	template <typename CharT = char>
	using Formatter = std::function<std::basic_string<CharT>(std::basic_string_view<CharT>)>;

	template <typename CharT = char>
	std::basic_string<CharT> formatterNone(std::basic_string_view<CharT> str) { return str; }

	template <typename CharT = char>
	class Logger {
	protected:
		std::basic_ostream<CharT> stream;

	public:
		std::atomic_bool active;

		Formatter<CharT> formatter = formatterNone;

		Logger(std::basic_streambuf<CharT>* target) :
			active(false), stream(target) {}

		void log(std::basic_string_view<CharT> msg) {
			if (active) stream << formatter(msg);
		}

		template <typename T>
		Logger& operator<<(const T& msg) {
			stream << msg;
			return *this;
		}
	};

	// non-owning container
	template <typename CharT = char>
	class LoggingContext {
	protected:
		std::vector<Logger<CharT>*> loggers;
		std::mutex contextMtx;

	public:
		using LoggerT = Logger<CharT>;

		void addLogger(LoggerT* logger) {
			if (logger == nullptr) throw std::runtime_error("Attempted to add null logger");
			std::unique_lock lock(contextMtx);
			loggers.emplace_back(logger);
		}

		void activateLogger(LoggerT* logger) {
			if (logger == nullptr) throw std::runtime_error("Attempted to activate null logger");
			std::unique_lock lock(contextMtx);
			logger->active = true;
			loggers.emplace_back(logger);
		}

		void removeLogger(LoggerT* logger) {
			std::unique_lock lock(contextMtx);
			loggers.erase(std::remove(loggers.begin(), loggers.end(), logger));
		}

		void log(std::basic_string_view<CharT> msg) {
			std::unique_lock lock(contextMtx);
			for (auto& logger : loggers) {
				logger->log(msg);
			}
		}
	};
}