#pragma once
#include <asyncpp/detail/promise_allocator_base.h>
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/rw_tag.h>
#include <asyncpp/grpc/traits.h>
#include <asyncpp/grpc/util.h>
#include <type_traits>

namespace asyncpp::grpc {

	template<rpc_method_type Type, typename TRequest, typename TResponse>
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

		auto send_initial_metadata() noexcept {
			struct awaiter : calldata_interface {
				constexpr awaiter(task_rpc_context* p) noexcept : m_self{p} {}
				task_rpc_context* m_self;
				coroutine_handle<> m_handle{};
				bool m_ok{false};
				void handle_event(size_t, bool ok) noexcept override {
					m_ok = ok;
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
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
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
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
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				constexpr void await_suspend(coroutine_handle<> h) noexcept {
					m_handle = h;
					m_self->m_stream.Write(m_msg, this);
				}
				constexpr bool await_resume() const noexcept { return m_ok; }
			};
			return awaiter{this, msg};
		}

	private:
		task_rpc_context(::grpc::ServerCompletionQueue* cq) : m_context{}, m_cq{cq} {}
		::grpc::ServerContext m_context;
		::grpc::ServerCompletionQueue* m_cq;
		typename traits::stream_type m_stream{&m_context};
		class empty_type {};
		[[no_unique_address]] std::conditional_t<traits::is_client_streaming, empty_type, typename traits::request_type> m_request{};
		[[no_unique_address]] std::conditional_t<traits::is_server_streaming, empty_type, typename traits::response_type> m_response{};

		template<auto FN, ByteAllocator Allocator>
		friend class task;

		task_rpc_context(const task_rpc_context&) = delete;
		task_rpc_context& operator=(const task_rpc_context&) = delete;
		task_rpc_context(task_rpc_context&&) = delete;
		task_rpc_context& operator=(task_rpc_context&&) = delete;
	};

	/** \brief Coroutine type for grpc server tasks */
	template<auto FN, ByteAllocator Allocator = default_allocator_type>
	struct task {
		using traits = method_traits<decltype(FN)>;
		// Promise type of this task
		class promise_type : public detail::promise_allocator_base<Allocator> {
			using context_type = task_rpc_context<traits::type, typename traits::request_type, typename traits::response_type>;

		public:
			static_assert(traits::is_server_side, "Method provided to grpc::task is not a serverside task");

			template<typename... Args>
			constexpr promise_type(typename traits::service_type* service, ::grpc::ServerCompletionQueue* cq, Args&...) noexcept
				: m_service{service}, m_context{cq}, m_status{} {}

			~promise_type() noexcept = default;

			coroutine_handle<promise_type> get_return_object() noexcept { return coroutine_handle<promise_type>::from_promise(*this); }

			void return_value(::grpc::Status s) noexcept { m_status = std::move(s); }

			void unhandled_exception() { m_status = util::exception_to_status(std::current_exception()); }

			auto initial_suspend() noexcept {
				struct awaiter : calldata_interface {
					awaiter(promise_type* self) : m_self{self} {}
					promise_type* m_self;
					coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						if (ok) m_handle.resume();
						// The request was cancelled, never enter user code
						else
							m_handle.destroy();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(coroutine_handle<> h) noexcept {
						auto& ctx = m_self->m_context;
						m_handle = h;
						if constexpr (traits::is_client_streaming) {
							(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_stream, ctx.m_cq, ctx.m_cq, this);
						} else {
							(m_self->m_service->*(FN))(&ctx.m_context, &ctx.m_request, &ctx.m_stream, ctx.m_cq, ctx.m_cq, this);
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
							ctx.m_stream.Finish(m_self->m_status, this);
						} else {
							if (m_self->m_status.ok()) {
								ctx.m_stream.Finish(ctx.m_response, m_self->m_status, this);
							} else {
								ctx.m_stream.FinishWithError(m_self->m_status, this);
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
			::grpc::Status m_status;

			promise_type(const promise_type&) = delete;
			promise_type& operator=(const promise_type&) = delete;
			promise_type(promise_type&&) = delete;
			promise_type& operator=(promise_type&&) = delete;
		};

		/// \brief Construct from a handle
		task(coroutine_handle<promise_type> h) {
			if (!h) throw std::invalid_argument("handle is invalid");
		}
	};
} // namespace asyncpp::grpc
