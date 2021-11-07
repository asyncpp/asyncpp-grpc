#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/traits.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace asyncpp::grpc {
	template<auto FN>
	struct server_streaming_call {
	private:
		struct state : calldata_interface {
			std::unique_ptr<::grpc::ClientContext> context{};
			std::unique_ptr<::grpc::ClientAsyncReaderInterface<typename method_traits<decltype(FN)>::response_type>> reader{};

			void destruct(std::shared_ptr<state> s) {
				if (reader) {
					context->TryCancel();
					reader->Finish(&m_exit_status, this);
					m_self = s;
				}
			}

		private:
			std::shared_ptr<state> m_self{};
			::grpc::Status m_exit_status{};
			void handle_event(size_t evt, bool ok) noexcept override {
				assert(evt == 0);
				assert(m_self);
				reader.reset();
				context.reset();
				// We should never reach this if there are external references
				assert(m_self.unique());
				// Last action, will delete self
				m_self.reset();
			}
		};
		std::shared_ptr<state> m_state = std::make_shared<state>();

	public:
		server_streaming_call() { m_state->context = std::make_unique<::grpc::ClientContext>(); }
		server_streaming_call(::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {}) {
			m_state->context = ::grpc::ClientContext::FromServerContext(ctx, opts);
		}
		~server_streaming_call() { m_state->destruct(m_state); }

		server_streaming_call(const server_streaming_call&) = delete;
		server_streaming_call(server_streaming_call&&) = delete;
		server_streaming_call& operator=(const server_streaming_call&) = delete;
		server_streaming_call& operator=(server_streaming_call&&) = delete;

		std::shared_ptr<::grpc::ClientContext> context() noexcept { return std::shared_ptr<::grpc::ClientContext>(m_state, m_state->context.get()); }
		std::shared_ptr<const ::grpc::ClientContext> context() const noexcept {
			return std::shared_ptr<const ::grpc::ClientContext>(m_state, m_state->context.get());
		}

		auto start(typename method_traits<decltype(FN)>::service_type* stub, const typename method_traits<decltype(FN)>::request_type& req,
				   ::grpc::CompletionQueue* cq) {
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				typename method_traits<decltype(FN)>::service_type* m_stub{};
				const typename method_traits<decltype(FN)>::request_type& m_request{};
				::grpc::CompletionQueue* m_cq{};
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(std::shared_ptr<state> state, decltype(m_stub) s, const typename method_traits<decltype(FN)>::request_type& req,
						::grpc::CompletionQueue* cq)
					: m_state{state}, m_stub{s}, m_request{req}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					if (!ok) m_state->reader.reset();
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->reader = (m_stub->*FN)(m_state->context.get(), m_request, m_cq, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state, stub, req, cq};
		}

		auto read(typename method_traits<decltype(FN)>::response_type& resp) {
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				typename method_traits<decltype(FN)>::response_type* m_resp;
				coroutine_handle<> m_handle{};
				bool m_was_ok;

				awaiter(std::shared_ptr<state> state, typename method_traits<decltype(FN)>::response_type* resp) : m_state{state}, m_resp{resp} {}

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
					m_state->reader->Read(m_resp, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state, &resp};
		}

		auto finish() {
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				coroutine_handle<> m_handle{};
				::grpc::Status m_status{};

				awaiter(std::shared_ptr<state> state) : m_state{state} {}

				void handle_event(size_t evt, bool ok) noexcept override {
					assert(m_handle);
					if (!ok) m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false");
					m_state->reader.reset();
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->reader->Finish(&m_status, this);
				}
				::grpc::Status await_resume() noexcept { return m_status; }
			};
			return awaiter{m_state};
		}
	};

} // namespace asyncpp::grpc
