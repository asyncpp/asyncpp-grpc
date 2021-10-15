#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/util.h>
#include <asyncpp/ptr_tag.h>

namespace asyncpp::grpc {

	template<typename TService, typename TRequest, typename TResponse>
	struct start_unary_tag {
		void (TService::*m_function)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*, ::grpc::CompletionQueue*,
									 ::grpc::ServerCompletionQueue*, void*);
		TService* m_service;
		::grpc::ServerCompletionQueue* m_cq;
	};

	template<typename TResponse>
	struct server_unary_task {
		// Promise type of this task
		class promise_type : asyncpp::grpc::calldata_interface {
			// Coroutine and task each have a reference
			std::atomic<size_t> m_ref_count{2};

			::grpc::ServerContext m_context{};
			TResponse m_response{};
			::grpc::ServerAsyncResponseWriter<TResponse> m_writer{&m_context};

			bool m_start_ok{false};

			void handle_event(size_t evt, bool ok) noexcept override {
				switch (evt) {
				case 0: m_start_ok = ok; return asyncpp::coroutine_handle<promise_type>::from_promise(*this).resume();
				case 1: return this->unref();
				case 2: return this->unref();
				}
			}

		public:
			promise_type() noexcept = default;
			promise_type(const promise_type&) = delete;
			promise_type(promise_type&&) = delete;
			~promise_type() noexcept = default;

			asyncpp::coroutine_handle<promise_type> get_return_object() noexcept { return asyncpp::coroutine_handle<promise_type>::from_promise(*this); }
			auto initial_suspend() noexcept { return asyncpp::suspend_always{}; }
			auto final_suspend() noexcept {
				struct await {
					bool ready;
					constexpr bool await_ready() const noexcept { return ready; }
					constexpr void await_suspend(asyncpp::coroutine_handle<>) const noexcept {}
					constexpr void await_resume() const noexcept {}
				};
				return await{m_ref_count.fetch_sub(1) == 1};
			}
			void return_value(::grpc::Status s) noexcept {
				if (!m_start_ok) return;
				this->ref();
				m_writer.Finish(m_response, s, asyncpp::ptr_tag<2>(this));
			}
			void unhandled_exception() {
				if (!m_start_ok) return;
				this->ref();
				m_writer.FinishWithError(util::exception_to_status(std::current_exception()), asyncpp::ptr_tag<1>(this));
			}

			template<typename TService, typename TRequest>
			auto await_transform(start_unary_tag<TService, TRequest, TResponse> s) noexcept {
				struct awaiter : asyncpp::grpc::calldata_interface {
					awaiter(promise_type* self, decltype(s) start) : m_self{self}, m_start{start} {}
					promise_type* m_self;
					decltype(s) m_start;
					TRequest m_request{};
					asyncpp::coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						m_self->m_start_ok = ok;
						m_handle.resume();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(asyncpp::coroutine_handle<> h) noexcept {
						m_handle = h;
						(m_start.m_service->*(m_start.m_function))(&m_self->m_context, &m_request, &m_self->m_writer, m_start.m_cq, m_start.m_cq, this);
					}
					std::tuple<bool, const TRequest, TResponse&, ::grpc::ServerContext&> await_resume() const noexcept {
						return {m_self->m_start_ok, m_request, m_self->m_response, m_self->m_context};
					}
				};
				return awaiter{this, s};
			}

			template<typename U>
			U&& await_transform(U&& awaitable) noexcept {
				return static_cast<U&&>(awaitable);
			}

			void unref() noexcept {
				auto res = m_ref_count.fetch_sub(1);
				if (res == 1) { asyncpp::coroutine_handle<promise_type>::from_promise(*this).destroy(); }
			}

			void ref() noexcept { m_ref_count.fetch_add(1); }
		};

		using handle_t = asyncpp::coroutine_handle<promise_type>;

		/// \brief Construct from a handle
		server_unary_task(handle_t h) : m_coro{h} {
			if (!m_coro) throw std::invalid_argument("m_coro is invalid");
			m_coro.resume();
		}

		/// \brief Move constructor
		server_unary_task(server_unary_task&& t) noexcept : m_coro(std::exchange(t.m_coro, {})) {}

		/// \brief Move assignment
		server_unary_task& operator=(server_unary_task&& t) noexcept {
			m_coro = std::exchange(t.m_coro, m_coro);
			return *this;
		}

		server_unary_task(const server_unary_task& o) : m_coro{o.m_coro} {
			if (m_coro) m_coro.promise().ref();
		}

		server_unary_task& operator=(const server_unary_task& o) {
			if (m_coro) m_coro.promise().unref();
			m_coro = o.m_coro;
			if (m_coro) m_coro.promise().ref();
			return *this;
		}

		/// \brief Destructor
		~server_unary_task() {
			// Since we still have the coroutine, it has never been fire_and_forgeted, so we need to destroy it
			if (m_coro) m_coro.promise().unref();
		}

	private:
		handle_t m_coro;
	};

	template<typename TService, typename TRequest, typename TResponse, typename TService2>
	start_unary_tag<TService, TRequest, TResponse> start(void (TService::*fn)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*,
																			  ::grpc::CompletionQueue*, ::grpc::ServerCompletionQueue*, void*),
														 TService2* s, ::grpc::ServerCompletionQueue* cq) {
		return start_unary_tag<TService, TRequest, TResponse>{fn, s, cq};
	}
} // namespace asyncpp::grpc