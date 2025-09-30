#pragma once

#include <atomic>
#include <concepts>
#include <format>
#include <functional>
#include <string>
#include <string_view>

namespace sndx::utility {
	struct LogLevel {
		using T = intmax_t;

		static constexpr T increment = 10;

		static constexpr T Info = 0;
		static constexpr T Debug = Info - increment;
		static constexpr T Trace = Debug - increment;

		static constexpr T Warning = Info + increment;
		static constexpr T Error = Warning + increment;

#ifdef _DEBUG
		static constexpr T Default = Debug;
#else
		static constexpr T Default = Info;
#endif
		[[nodiscard]]
		static constexpr std::string_view toString(T level) {
			if (level >= Error) {
				return "Error";
			}
			if (level >= Warning) {
				return "Warning";
			}
			if (level >= Info) {
				return "Info";
			}
			if (level >= Debug) {
				return "Debug";
			}
			return "Trace";
		}
	};
	using LogLevelT = LogLevel::T;

	class Logger {
	private:
		std::atomic<LogLevelT> m_level{ LogLevel::Default };

	protected:
		virtual void log_impl(LogLevelT level, std::string&& str) = 0;

	public:

		template <class... Args>
		void log(LogLevelT level, std::format_string<Args...> fmt, Args&&... args) {
			if (level >= m_level.load(std::memory_order_acquire)) {
				log_impl(level, std::format(fmt, std::forward<Args>(args)...));
			}
		}

		template <class... Args>
		void vlog(LogLevelT level, std::string_view fmt, Args&&... args) {
			if (level >= m_level.load(std::memory_order_acquire)) {
				log_impl(level, std::vformat(fmt, std::make_format_args(args...)));
			}
		}

		auto setLevel(LogLevelT level) noexcept {
			return m_level.exchange(level, std::memory_order_acq_rel);
		}

		[[nodiscard]]
		auto level() const noexcept {
			return m_level.load(std::memory_order_acquire);
		}

		virtual ~Logger() = default;

	};

	template <std::invocable F> 
	struct LazyArg {
		F fnc;
	};
}

#define SNDX_MAKE_LAZY(func) \
	sndx::utility::LazyArg{[&](){ return (func); }}

namespace std {
	template <std::invocable F>
	struct formatter<sndx::utility::LazyArg<F>> {
		constexpr auto parse(std::format_parse_context& ctx) {
			return ctx.begin();
		}

		auto format(const sndx::utility::LazyArg<F>& arg, std::format_context& ctx) const {
			return std::format_to(ctx.out(), "{}", arg.fnc());
		}
	};
}
