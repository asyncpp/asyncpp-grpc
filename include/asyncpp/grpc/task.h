#pragma once
#include "asyncpp/ptr_tag.h"
#include <asyncpp/detail/promise_allocator_base.h>
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/rw_tag.h>
#include <asyncpp/grpc/traits.h>
#include <asyncpp/grpc/util.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/status.h>
#include <type_traits>

namespace asyncpp::grpc {

	struct noop_task_hook {
		template<typename... Args>
		constexpr noop_task_hook(const Args&...) noexcept {}

		template<typename Context, typename FN>
		constexpr void on_authenticate(const Context&, FN&& cb) const noexcept {
			cb(::grpc::Status::OK);
		}

		template<typename Context>
		constexpr void on_start(const Context&) const noexcept {}

		template<typename Context>
		constexpr void on_finish(const Context&, const ::grpc::Status&) const noexcept {}

		template<typename Context, typename TMessage>
		constexpr void on_finish(const Context&, const ::grpc::Status&, const TMessage&) const noexcept {}

		template<typename Context>
		constexpr void on_send_initial_metadata_start(const Context&) const noexcept {}

		template<typename Context>
		constexpr void on_send_initial_metadata_done(const Context&, bool) const noexcept {}

		template<typename Context>
		constexpr void on_read_start(const Context&) const noexcept {}

		template<typename Context, typename TMessage>
		constexpr void on_read_done(const Context&, const TMessage&, bool) const noexcept {}

		template<typename Context, typename TMessage>
		constexpr void on_write_start(const Context&, const TMessage&) const noexcept {}

		template<typename Context>
		constexpr void on_write_done(const Context&, bool) const noexcept {}
	};

	template<rpc_method_type Type, typename TRequest, typename TResponse, typename THooks>
	class task_rpc_context {
	public:
		using traits = server_stream_traits<Type, TRequest, TResponse>;

		::grpc::ServerContext& context() noexcept { return m_context; }
		const ::grpc::ServerContext& context() const noexcept { return m_context; }
		::grpc::ServerCompletionQueue* completion_queue() const noexcept { return m_cq; }

		typename traits::request_type& request() noexcept requires(!traits::is_client_streaming) { return m_request; }
		const typename traits::request_type& request() const noexcept requires(!traits::is_client_streaming) { return m_request; }
		typename traits::response_type& response() noexcept requires(!traits::is_server_streaming) { return m_response; }
		const typename traits::response_type& response() const noexcept requires(!traits::is_server_streaming) { return m_response; }
		THooks& hooks() noexcept { return m_hooks; }
		const THooks& hooks() const noexcept { return m_hooks; }

		auto send_initial_metadata() noexcept {
			struct awaiter : calldata_interface {
				constexpr awaiter(task_rpc_context* p) noexcept : m_self{p} {}
				task_rpc_context* m_self;
				coroutine_handle<> m_handle{};
				bool m_ok{false};
				void handle_event(size_t, bool ok) noexcept override {
					m_ok = ok;
					m_self->m_hooks.on_send_initial_metadata_done(*m_self, m_ok);
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
					m_self->m_hooks.on_send_initial_metadata_start(*m_self);
					m_self->m_stream.SendInitialMetadata(this);
				}
				constexpr bool await_resume() const noexcept { return m_ok; }
			};
			return awaiter{this};
		}

		auto read(typename traits::request_type& msg) noexcept requires(traits::is_client_streaming) {
			struct awaiter : calldata_interface {
				awaiter(task_rpc_context* self, typename traits::request_type& msg) : m_self{self}, m_msg{msg} {}
				task_rpc_context* m_self;
				typename traits::request_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_ok{false};
				void handle_event(size_t, bool ok) noexcept override {
					m_ok = ok;
					m_self->m_hooks.on_read_done(*m_self, m_msg, m_ok);
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
					m_self->m_hooks.on_read_start(*m_self);
					m_self->m_stream.Read(&m_msg, this);
				}
				constexpr bool await_resume() const noexcept { return m_ok; }
			};
			return awaiter{this, msg};
		}

		auto write(const typename traits::response_type& msg) noexcept requires(traits::is_server_streaming) {
			struct awaiter : calldata_interface {
				awaiter(task_rpc_context* self, const typename traits::response_type& msg) : m_self{self}, m_msg{msg} {}
				task_rpc_context* m_self;
				const typename traits::response_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_ok{false};
				void handle_event(size_t, bool ok) noexcept override {
					m_ok = ok;
					m_self->m_hooks.on_write_done(*m_self, m_ok);
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
					m_self->m_hooks.on_write_start(*m_self, m_msg);
					m_self->m_stream.Write(m_msg, this);
				}
				constexpr bool await_resume() const noexcept { return m_ok; }
			};
			return awaiter{this, msg};
		}

	private:
		template<typename TService, typename... Args>
		task_rpc_context(TService* service, ::grpc::ServerCompletionQueue* cq, Args&... args) : m_cq{cq}, m_hooks{service, cq, args...} {}
		::grpc::ServerCompletionQueue* m_cq;
		::grpc::ServerContext m_context{};
		::grpc::Status m_status{};
		typename traits::stream_type m_stream{&m_context};
		class empty_type {};
		[[no_unique_address]] std::conditional_t<traits::is_client_streaming, empty_type, typename traits::request_type> m_request{};
		[[no_unique_address]] std::conditional_t<traits::is_server_streaming, empty_type, typename traits::response_type> m_response{};

		[[no_unique_address]] THooks m_hooks;

		template<auto FN, ByteAllocator Allocator, typename Hooks>
		friend class task;

		task_rpc_context(const task_rpc_context&) = delete;
		task_rpc_context& operator=(const task_rpc_context&) = delete;
		task_rpc_context(task_rpc_context&&) = delete;
		task_rpc_context& operator=(task_rpc_context&&) = delete;
	};

	/** \brief Coroutine type for grpc server tasks */
	template<auto FN, ByteAllocator Allocator = default_allocator_type, typename Hooks = noop_task_hook>
	struct task {
		using traits = method_traits<decltype(FN)>;
		// Promise type of this task
		class promise_type : public detail::promise_allocator_base<Allocator> {
			using context_type = task_rpc_context<traits::type, typename traits::request_type, typename traits::response_type, Hooks>;

		public:
			static_assert(traits::is_server_side, "Method provided to grpc::task is not a serverside task");

			template<typename TService, typename... Args>
			requires(std::is_base_of_v<typename traits::service_type, TService>) constexpr promise_type(TService* service, ::grpc::ServerCompletionQueue* cq,
																										Args&... args) noexcept
				: m_service{service}, m_context{service, cq, args...} {}

			~promise_type() noexcept = default;

			coroutine_handle<> get_return_object() noexcept { return coroutine_handle<promise_type>::from_promise(*this); }

			void return_value(::grpc::Status s) noexcept { m_context.m_status = std::move(s); }

			void unhandled_exception() { m_context.m_status = util::exception_to_status(std::current_exception()); }

			auto initial_suspend() noexcept {
				struct awaiter : calldata_interface {
					awaiter(promise_type* self) : m_self{self} {}
					promise_type* m_self;
					coroutine_handle<> m_handle{};
					void handle_event(size_t idx, bool ok) noexcept override {
						auto& ctx = m_self->m_context;
						switch (idx) {
						case 0:
							// The request was cancelled, never enter user code
							if (!ok) return m_handle.destroy();
							ctx.m_hooks.on_authenticate(ctx, [this](::grpc::Status result) {
								auto& ctx = m_self->m_context;
								if (result.ok()) {
									ctx.m_hooks.on_start(ctx);
									m_handle.resume();
								} else {
									if constexpr (traits::is_server_streaming) {
										ctx.m_stream.Finish(result, ptr_tag<1, calldata_interface>(this));
									} else {
										ctx.m_stream.FinishWithError(result, ptr_tag<1, calldata_interface>(this));
									}
								}
							});
							break;
						case 1:
							// Reset the state of context and stream
							ctx.m_context.~ServerContext();
							new (&ctx.m_context)::grpc::ServerContext{};
							ctx.m_stream = typename traits::stream_type{&ctx.m_context};
							if constexpr (traits::is_client_streaming) {
								(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_stream, ctx.m_cq, ctx.m_cq, ptr_tag<0, calldata_interface>(this));
							} else {
								ctx.m_request.Clear();
								(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_request, &ctx.m_stream, ctx.m_cq, ctx.m_cq,
														   ptr_tag<0, calldata_interface>(this));
							}
						}
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(coroutine_handle<> h) noexcept {
						auto& ctx = m_self->m_context;
						m_handle = h;
						if constexpr (traits::is_client_streaming) {
							(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_stream, ctx.m_cq, ctx.m_cq, ptr_tag<0, calldata_interface>(this));
						} else {
							(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_request, &ctx.m_stream, ctx.m_cq, ctx.m_cq, ptr_tag<0, calldata_interface>(this));
						}
					}
					void await_resume() const noexcept {}
				};
				return awaiter{this};
			}

			auto final_suspend() noexcept {
				struct awaiter : calldata_interface {
					awaiter(promise_type* self) : m_self{self} {}
					promise_type* m_self;
					coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						if (!ok) {
							// TODO: Signal error
						}
						m_handle.destroy();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(coroutine_handle<> h) noexcept {
						auto& ctx = m_self->m_context;
						m_handle = h;
						if constexpr (traits::is_server_streaming) {
							ctx.m_hooks.on_finish(ctx, ctx.m_status);
							ctx.m_stream.Finish(ctx.m_status, this);
						} else {
							if (ctx.m_status.ok()) {
								ctx.m_hooks.on_finish(ctx, ctx.m_status, ctx.m_response);
								ctx.m_stream.Finish(ctx.m_response, ctx.m_status, this);
							} else {
								ctx.m_hooks.on_finish(ctx, ctx.m_status);
								ctx.m_stream.FinishWithError(ctx.m_status, this);
							}
						}
					}
					void await_resume() const noexcept {}
				};
				return awaiter{this};
			}

			auto await_transform(rpc_info_tag) noexcept {
				struct awaiter {
					promise_type* m_self;
					constexpr bool await_ready() const noexcept { return true; }
					constexpr void await_suspend(asyncpp::coroutine_handle<> h) noexcept { assert(false); }
					constexpr context_type& await_resume() const noexcept { return m_self->m_context; }
				};
				return awaiter{this};
			}

			auto await_transform(send_initial_metadata_tag) noexcept { return m_context.send_initial_metadata(); }

			auto await_transform(read_tag<typename traits::request_type> t) noexcept requires(traits::is_client_streaming) { return m_context.read(*t.m_msg); }

			auto await_transform(typename traits::request_type& msg) noexcept requires(traits::is_client_streaming) { return m_context.read(msg); }

			auto await_transform(write_tag<typename traits::response_type> t) noexcept requires(traits::is_server_streaming) {
				return m_context.write(*t.m_msg);
			}

			auto await_transform(const typename traits::response_type& msg) noexcept requires(traits::is_server_streaming) { return m_context.write(msg); }

			template<typename U>
			U&& await_transform(U&& awaitable) noexcept {
				return static_cast<U&&>(awaitable);
			}

			auto yield_value(const typename traits::response_type& value) noexcept requires(traits::is_server_streaming) { return m_context.write(value); }

		private:
			typename traits::service_type* m_service;
			context_type m_context;

			promise_type(const promise_type&) = delete;
			promise_type& operator=(const promise_type&) = delete;
			promise_type(promise_type&&) = delete;
			promise_type& operator=(promise_type&&) = delete;
		};

		/// \brief Construct from a handle
		task(coroutine_handle<> h) {
			if (!h) throw std::invalid_argument("handle is invalid");
		}
	};
} // namespace asyncpp::grpc
