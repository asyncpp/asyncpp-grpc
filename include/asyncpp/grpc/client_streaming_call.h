#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/traits.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace asyncpp::grpc {
	template<auto FN>
	struct client_streaming_call {
		struct state : calldata_interface {
			std::unique_ptr<::grpc::ClientContext> context{};
			std::unique_ptr<::grpc::ClientAsyncWriterInterface<typename method_traits<decltype(FN)>::request_type>> writer{};
			typename method_traits<decltype(FN)>::response_type response{};
			bool m_need_writes_done{false};

			void destruct(std::shared_ptr<state> s) {
				if (writer) {
					context->TryCancel();
					if (m_need_writes_done)
						writer->WritesDone(this);
					else
						writer->Finish(&m_exit_status, asyncpp::ptr_tag<1>(this));
					m_self = s;
				}
			}

		private:
			std::shared_ptr<state> m_self{};
			::grpc::Status m_exit_status{};
			void handle_event(size_t evt, bool ok) noexcept override {
				assert(m_self);
				// We should never reach this if there are external references
				assert(m_self.unique());
				if (evt == 0)
					return writer->Finish(&m_exit_status, asyncpp::ptr_tag<1>(this));
				else {
					writer.reset();
					context.reset();
					// Last action, will delete self
					m_self.reset();
				}
			}
		};
		std::shared_ptr<state> m_state = std::make_shared<state>();

	public:
		client_streaming_call() { m_state->context = std::make_unique<::grpc::ClientContext>(); }
		client_streaming_call(::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {}) {
			m_state->context = ::grpc::ClientContext::FromServerContext(ctx, opts);
		}
		~client_streaming_call() { m_state->destruct(m_state); }

		client_streaming_call(const client_streaming_call&) = delete;
		client_streaming_call(client_streaming_call&&) = delete;
		client_streaming_call& operator=(const client_streaming_call&) = delete;
		client_streaming_call& operator=(client_streaming_call&&) = delete;

		std::shared_ptr<::grpc::ClientContext> context() noexcept { return std::shared_ptr<::grpc::ClientContext>(m_state, m_state->context.get()); }
		std::shared_ptr<const ::grpc::ClientContext> context() const noexcept {
			return std::shared_ptr<const ::grpc::ClientContext>(m_state, m_state->context.get());
		}

		auto start(typename method_traits<decltype(FN)>::service_type* stub, ::grpc::CompletionQueue* cq) {
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				typename method_traits<decltype(FN)>::service_type* m_stub{};
				::grpc::CompletionQueue* m_cq{};
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(std::shared_ptr<state> state, decltype(m_stub) s, ::grpc::CompletionQueue* cq) : m_state{state}, m_stub{s}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					m_was_ok = ok;
					m_state->m_need_writes_done = ok;
					if (!ok) m_state->writer.reset();
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					m_state->writer = (m_stub->*FN)(m_state->context.get(), &m_state->response, m_cq, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state, stub, cq};
		}

		auto write(const typename method_traits<decltype(FN)>::request_type& msg) {
			if (!m_state || !m_state->writer) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				const typename method_traits<decltype(FN)>::request_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(std::shared_ptr<state> state, decltype(m_msg) m) : m_state{state}, m_msg{m} {}

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
					m_state->writer->Write(m_msg, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state, msg};
		}

		auto write_last(const typename method_traits<decltype(FN)>::request_type& msg) {
			if (!m_state || !m_state->writer) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				const typename method_traits<decltype(FN)>::request_type& m_msg;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(std::shared_ptr<state> state, decltype(m_msg) m) : m_state{state}, m_msg{m} {}

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
					m_state->writer->WriteLast(m_msg, ::grpc::WriteOptions{}, this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state, msg};
		}

		auto writes_done() {
			if (!m_state || !m_state->writer) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				coroutine_handle<> m_handle{};
				bool m_was_ok = false;

				awaiter(std::shared_ptr<state> state) : m_state{state} {}

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
					m_state->writer->WritesDone(this);
				}
				bool await_resume() noexcept { return m_was_ok; }
			};
			return awaiter{m_state};
		}

		auto finish() {
			if (!m_state || !m_state->writer) throw std::logic_error("No active call");
			struct awaiter : calldata_interface {
				std::shared_ptr<state> m_state;
				coroutine_handle<> m_handle{};
				::grpc::Status m_status{};

				awaiter(std::shared_ptr<state> state) : m_state{state} {}

				void handle_event(size_t evt, bool ok) noexcept override {
					assert(m_handle);
					if (!ok) m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false");
					switch (evt) {
					case 0: return m_state->writer->Finish(&m_status, asyncpp::ptr_tag<1>(this)); break;
					case 1: m_state->writer.reset(); break;
					default: assert(false); break;
					}
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_state);
					m_handle = h;
					if (m_state->m_need_writes_done)
						m_state->writer->WritesDone(this);
					else
						m_state->writer->Finish(&m_status, asyncpp::ptr_tag<1>(this));
				}
				::grpc::Status await_resume() noexcept { return m_status; }
			};
			return awaiter{m_state};
		}

	};

} // namespace asyncpp::grpc
