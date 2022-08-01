#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <tuple>
#include <cassert>

namespace asyncpp::grpc {

	template<typename TFunction, typename... Args>
	struct grpc_generic_method_awaiter : calldata_interface {
		grpc_generic_method_awaiter(TFunction fn, Args... args) requires(std::is_invocable_v<TFunction, Args..., void*>)
			: m_function{fn}, m_args{std::forward_as_tuple(args...)} {}

		TFunction m_function;
		std::tuple<Args...> m_args;

		bool m_was_ok{false};
		coroutine_handle<> m_handle{};

		void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
			assert(evt == 0);
			assert(m_handle);
			m_was_ok = ok;
			m_handle.resume();
		}
		bool await_ready() noexcept { return false; }
		void await_suspend(coroutine_handle<> h) noexcept {
			m_handle = h;
			std::apply(m_function, std::tuple_cat(m_args, std::tuple{this}));
		}
		bool await_resume() noexcept { return m_was_ok; }
	};
} // namespace asyncpp::grpc