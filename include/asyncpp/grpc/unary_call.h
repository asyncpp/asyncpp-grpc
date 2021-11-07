#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/traits.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace asyncpp::grpc {
	template<auto FN>
	struct unary_call {
		std::unique_ptr<::grpc::ClientContext> context;
		typename method_traits<decltype(FN)>::request_type request{};
		typename method_traits<decltype(FN)>::response_type response{};

        unary_call() { context = std::make_unique<::grpc::ClientContext>(); }
		unary_call(::grpc::ServerContext& ctx, ::grpc::PropagationOptions opts = {}) {
			context = ::grpc::ClientContext::FromServerContext(ctx, opts);
		}

		unary_call(const unary_call&) = delete;
		unary_call(unary_call&&) = delete;
		unary_call& operator=(const unary_call&) = delete;
		unary_call& operator=(unary_call&&) = delete;

		auto invoke(typename method_traits<decltype(FN)>::service_type* stub, ::grpc::CompletionQueue* cq) {
			struct awaiter : calldata_interface {
				unary_call* m_parent;
				typename method_traits<decltype(FN)>::service_type* m_stub{};
				::grpc::CompletionQueue* m_cq{};
				std::unique_ptr<::grpc::ClientAsyncResponseReader<decltype(response)>> m_reader{};
				coroutine_handle<> m_handle{};
		        ::grpc::Status m_status{};

				awaiter(unary_call* that, decltype(m_stub) s, ::grpc::CompletionQueue* cq) : m_parent{that}, m_stub{s}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					if (!ok) { m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false"); }
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_parent);
					m_handle = h;

					m_reader = (m_stub->*FN)(m_parent->context.get(), m_parent->request, m_cq);
					m_reader->Finish(&m_parent->response, &m_status, this);
				}
				::grpc::Status await_resume() noexcept { return m_status; }
			};
			return awaiter{this, stub, cq};
		}

		auto invoke(const std::unique_ptr<typename method_traits<decltype(FN)>::service_type>& stub, ::grpc::CompletionQueue* cq) {
			return this->invoke(stub.get(), cq);
		}

		auto invoke(typename method_traits<decltype(FN)>::service_type* stub, ::grpc::CompletionQueue& cq) { return this->invoke(stub, &cq); }

		auto invoke(const std::unique_ptr<typename method_traits<decltype(FN)>::service_type>& stub, ::grpc::CompletionQueue& cq) {
			return this->invoke(stub.get(), &cq);
		}
	};

} // namespace asyncpp::grpc
