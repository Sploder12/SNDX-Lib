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
	std::basic_string<CharT> formatterNone(std::basic_string_view<CharT> str) { return std::basic_string<CharT>(str); }

	template <typename CharT = char>
	class Logger {
	protected:
		std::basic_ostream<CharT> stream;

	public:
		std::atomic_bool active;

		Logger(std::basic_streambuf<CharT>* target) :
			active(false), stream(target) {}

		virtual void log(std::basic_string_view<CharT> msg) {
			if (active) stream << msg;
		}

		bool setActive(bool newVal) {
			return active.exchange(newVal);
		}

		template <typename T>
		Logger& operator<<(const T& msg) {
			if (active) {
				stream << msg;
			}
			return *this;
		}
	};

	// Warning: operator<< is still raw logging
	template <typename CharT = char>
	class FormatLogger : public Logger<CharT> {
	public:
		Formatter<CharT> formatter;

		FormatLogger(std::basic_streambuf<CharT>* target, Formatter<CharT> formatter = formatterNone<CharT>) :
			Logger<CharT>(target), formatter(formatter) {}

		void log(std::basic_string_view<CharT> msg) {
			Logger<CharT>::log(formatter(msg));
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

		void addLogger(LoggerT& logger) {
			std::unique_lock lock(contextMtx);
			loggers.emplace_back(&logger);
		}

		void activateLogger(LoggerT& logger) {
			logger.setActive(true);
			std::unique_lock lock(contextMtx);
			loggers.emplace_back(&logger);
		}

		void removeLogger(LoggerT& logger) {
			std::unique_lock lock(contextMtx);
			loggers.erase(std::remove(loggers.begin(), loggers.end(), &logger));
		}

		void log(std::basic_string_view<CharT> msg) {
			std::unique_lock lock(contextMtx);
			for (auto& logger : loggers) {
				logger->log(msg);
			}
		}
	};
}