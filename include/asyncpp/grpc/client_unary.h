#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/traits.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace asyncpp::grpc {
	template<typename TResp>
	auto finish(const std::unique_ptr<::grpc::ClientAsyncResponseReader<TResp>>& stub, TResp& resp) {
		struct awaiter : calldata_interface {
			::grpc::ClientAsyncResponseReader<TResp>* m_reader;
			TResp* m_response;
			::grpc::Status m_status{};
			coroutine_handle<> m_handle{};

			awaiter(::grpc::ClientAsyncResponseReader<TResp>* s, TResp* resp) : m_reader{s}, m_response{resp} {}

			void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
				assert(evt == 0);
				assert(m_handle);
				if (!ok) { m_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false"); }
				m_handle.resume();
			}
			bool await_ready() noexcept { return false; }
			void await_suspend(coroutine_handle<> h) noexcept {
				assert(m_reader);
				assert(m_response);
				m_handle = h;
				m_reader->Finish(m_response, &m_status, this);
			}
			::grpc::Status await_resume() noexcept { return std::move(m_status); }
		};
		return awaiter{stub.get(), &resp};
	}

	template<auto FN>
	struct unary_call {
		::grpc::ClientContext context;
		typename method_traits<decltype(FN)>::request_type request{};

		typename method_traits<decltype(FN)>::response_type response{};
		::grpc::Status status;

		auto invoke(typename method_traits<decltype(FN)>::service_type* stub, ::grpc::CompletionQueue* cq) {
			struct awaiter : calldata_interface {
				unary_call* m_parent;
				typename method_traits<decltype(FN)>::service_type* m_stub{};
				::grpc::CompletionQueue* m_cq{};
				std::unique_ptr<::grpc::ClientAsyncResponseReader<decltype(response)>> m_reader{};
				coroutine_handle<> m_handle{};

				awaiter(unary_call* that, decltype(m_stub) s, ::grpc::CompletionQueue* cq) : m_parent{that}, m_stub{s}, m_cq{cq} {}

				void handle_event([[maybe_unused]] size_t evt, bool ok) noexcept override {
					assert(evt == 0);
					assert(m_handle);
					if (!ok) { m_parent->status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Event returned ok=false"); }
					m_handle.resume();
				}
				bool await_ready() noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					assert(m_parent);
					m_handle = h;

					m_reader = (m_stub->*FN)(&m_parent->context, m_parent->request, m_cq);
					m_reader->Finish(&m_parent->response, &m_parent->status, this);
				}
				::grpc::Status await_resume() noexcept { return m_parent->status; }
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
