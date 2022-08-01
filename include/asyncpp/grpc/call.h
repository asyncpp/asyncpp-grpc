#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/traits.h>
#include <asyncpp/ptr_tag.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <memory>
#include <type_traits>

namespace asyncpp::grpc {
	template<auto FN>
	struct unary_call {
		using traits = method_traits<decltype(FN)>;
		std::unique_ptr<::grpc::ClientContext> m_context;
		typename traits::service_type* m_stub;

		unary_call(typename traits::service_type* stub) : m_stub{stub}, m_context{std::make_unique<::grpc::ClientContext>()} {}
		unary_call(typename traits::service_type* stub, ::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {})
			: m_stub{stub}, m_context{::grpc::ClientContext::FromServerContext(ctx, opts)} {}
		unary_call(const std::unique_ptr<typename traits::service_type>& stub) : unary_call{stub.get()} {}
		unary_call(const std::unique_ptr<typename traits::service_type>& stub, ::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {})
			: unary_call{stub.get(), ctx, opts} {}

		unary_call(const unary_call&) = delete;
		unary_call(unary_call&&) = delete;
		unary_call& operator=(const unary_call&) = delete;
		unary_call& operator=(unary_call&&) = delete;

		auto operator()(const typename traits::request_type& request, typename traits::response_type& response, ::grpc::CompletionQueue* cq) {
			struct awaiter : calldata_interface {
				::grpc::ClientContext* m_context;
				typename traits::service_type* m_stub;
				const typename traits::request_type& m_request;
				typename traits::response_type& m_response;
				::grpc::CompletionQueue* m_cq{};
				typename traits::stream_type m_reader{};
				::grpc::Status m_status{};
				coroutine_handle<> m_handle{};

				awaiter(::grpc::ClientContext* context, decltype(m_stub) s, const typename traits::request_type& request,
						typename traits::response_type& response, ::grpc::CompletionQueue* cq)
					: m_context{context}, m_stub{s}, m_request{request}, m_response{response}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					if (!ok) { m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false"); }
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_context);
					m_handle = h;

					m_reader = (m_stub->*FN)(m_context, m_request, m_cq);
					m_reader->Finish(&m_response, &m_status, this);
				}
				::grpc::Status await_resume() noexcept { return m_status; }
			};
			return awaiter{m_context.get(), m_stub, request, response, cq};
		}

		auto operator()(const typename traits::request_type& request, typename traits::response_type& response, ::grpc::CompletionQueue& cq) {
			return (*this)(request, response, &cq);
		}
	};

	template<auto FN>
	struct streaming_call {
		using traits = method_traits<decltype(FN)>;
		static_assert(!traits::is_server_side, "server side method passed to streaming_call");
		static_assert(traits::is_streaming, "method passed to streaming_call is not a streaming call");

	private:
		struct state : calldata_interface {
			std::unique_ptr<::grpc::ClientContext> context{};
			typename traits::service_type* stub;
			typename traits::stream_type stream{};
			bool m_need_writes_done{false};

			void destruct() {
				if (stream) {
					context->TryCancel();
					if constexpr (traits::is_client_streaming) {
						if (m_need_writes_done) {
							stream->WritesDone(ptr_tag<0>(this));
						} else {
							stream->Finish(&m_exit_status, ptr_tag<1>(this));
						}
					} else
						stream->Finish(&m_exit_status, ptr_tag<1>(this));
				} else {
					delete this;
				}
			}

		private:
			::grpc::Status m_exit_status{};
			void handle_event(size_t evt, bool ok) noexcept override {
				if (evt == 0) {
					return stream->Finish(&m_exit_status, ptr_tag<1>(this));
				} else {
					stream.reset();
					context.reset();
					// Last action, delete self
					delete this;
				}
			}
		};
		std::unique_ptr<state> m_state = std::make_unique<state>();

	public:
		streaming_call(typename traits::service_type* stub) {
			m_state->stub = stub;
			m_state->context = std::make_unique<::grpc::ClientContext>();
		}
		streaming_call(typename traits::service_type* stub, ::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {}) {
			m_state->stub = stub;
			m_state->context = ::grpc::ClientContext::FromServerContext(ctx, opts);
		}
		streaming_call(const std::unique_ptr<typename traits::service_type>& stub) : streaming_call{stub.get()} {}
		streaming_call(const std::unique_ptr<typename traits::service_type>& stub, ::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {})
			: streaming_call{stub.get(), ctx, opts} {}
		~streaming_call() { m_state.release()->destruct(); }

		streaming_call(const streaming_call&) = delete;
		streaming_call(streaming_call&&) = delete;
		streaming_call& operator=(const streaming_call&) = delete;
		streaming_call& operator=(streaming_call&&) = delete;

		::grpc::ClientContext& context() noexcept { return *m_state->context; }
		const ::grpc::ClientContext& context() const noexcept { return *m_state->context; }

		auto start(const typename traits::request_type& req,
				   ::grpc::CompletionQueue* cq) requires(!traits::is_client_streaming && traits::is_server_streaming) {
			struct awaiter : calldata_interface {
				state* m_state;
				const typename traits::request_type& m_request{};
				::grpc::CompletionQueue* m_cq{};
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state, const typename traits::request_type& req, ::grpc::CompletionQueue* cq) : m_state{state}, m_request{req}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					if (!ok) m_state->stream.reset();
					m_handle.resume();
				}
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream = (m_state->stub->*FN)(m_state->context.get(), m_request, m_cq, this);
				}
				bool await_resume() const noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), req, cq};
		}

		auto start(typename traits::response_type& resp, ::grpc::CompletionQueue* cq) requires(traits::is_client_streaming && !traits::is_server_streaming) {
			struct awaiter : calldata_interface {
				state* m_state;
				typename traits::response_type& m_response;
				::grpc::CompletionQueue* m_cq{};
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state, typename traits::response_type& resp, ::grpc::CompletionQueue* cq) : m_state{state}, m_response{resp}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_state->m_need_writes_done = ok;
					if (!ok) m_state->stream.reset();
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream = (m_state->stub->*FN)(m_state->context.get(), &m_response, m_cq, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), resp, cq};
		}

		auto start(::grpc::CompletionQueue* cq) requires(traits::is_client_streaming&& traits::is_server_streaming) {
			struct awaiter : calldata_interface {
				state* m_state;
				::grpc::CompletionQueue* m_cq{};
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state, ::grpc::CompletionQueue* cq) : m_state{state}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_state->m_need_writes_done = ok;
					if (!ok) m_state->stream.reset();
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream = (m_state->stub->*FN)(m_state->context.get(), m_cq, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), cq};
		}

		auto read(typename traits::response_type& resp) requires(traits::is_server_streaming) {
			struct awaiter : calldata_interface {
				state* m_state;
				typename traits::response_type* m_resp;
				coroutine_handle<> m_handle{};
				bool m_was_ok;

				awaiter(state* state, typename traits::response_type* resp) : m_state{state}, m_resp{resp} {}

				void handle_event(size_t evt, bool ok) noexcept override {
					assert(m_handle);
					assert(evt == 0);
					m_was_ok = ok;
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream->Read(m_resp, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), &resp};
		}

		auto write(const typename traits::request_type& msg) requires(traits::is_client_streaming) {
			if (!m_state || !m_state->stream) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				state* m_state;
				const typename traits::request_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state, decltype(m_msg) m) : m_state{state}, m_msg{m} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream->Write(m_msg, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), msg};
		}

		auto write_last(const typename traits::request_type& msg) requires(traits::is_client_streaming) {
			if (!m_state || !m_state->stream) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				state* m_state;
				const typename traits::request_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state, decltype(m_msg) m) : m_state{state}, m_msg{m} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_state->m_need_writes_done = false;
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream->WriteLast(m_msg, ::grpc::WriteOptions{}, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get(), msg};
		}

		auto writes_done() requires(traits::is_client_streaming) {
			if (!m_state || !m_state->stream) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				state* m_state;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(state* state) : m_state{state} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_state->m_need_writes_done = false;
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->stream->WritesDone(this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state.get()};
		}

		auto finish() {
			struct awaiter : calldata_interface {
				state* m_state;
				coroutine_handle<> m_handle{};
				::grpc::Status m_status{};

				awaiter(state* state) : m_state{state} {}

				void handle_event(size_t evt, bool ok) noexcept override {
					assert(m_handle);
					if (!ok) m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false");
					switch (evt) {
					case 0: return m_state->stream->Finish(&m_status, asyncpp::ptr_tag<1>(this)); break;
					case 1: m_state->stream.reset(); break;
					default: assert(false); break;
					}
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					if constexpr (traits::is_client_streaming) {
						if (m_state->m_need_writes_done) {
							m_state->stream->WritesDone(ptr_tag<0>(this));
						} else {
							m_state->stream->Finish(&m_status, ptr_tag<1>(this));
						}
					} else {
						m_state->stream->Finish(&m_status, ptr_tag<1>(this));
					}
				}
				::grpc::Status await_resume() noexcept { return m_status; }
			};
			return awaiter{m_state.get()};
		}
	};

	template<auto FN>
	using call = std::conditional_t<method_traits<decltype(FN)>::is_streaming, streaming_call<FN>, unary_call<FN>>;

} // namespace asyncpp::grpc
