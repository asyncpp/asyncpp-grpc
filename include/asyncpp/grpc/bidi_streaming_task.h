#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/grpc/rw_tag.h>
#include <asyncpp/grpc/util.h>

namespace asyncpp::grpc {

	template<typename TService, typename TRequest, typename TResponse>
	struct start_bidi_streaming_tag {
		void (TService::*m_function)(::grpc::ServerContext*, ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>*, ::grpc::CompletionQueue*,
									 ::grpc::ServerCompletionQueue*, void*);
		TService* m_service;
		::grpc::ServerCompletionQueue* m_cq;
	};

	template<typename TRequest, typename TResponse>
	struct bidi_streaming_task {
		// Promise type of this task
		class promise_type : asyncpp::grpc::calldata_interface {
			// Coroutine and task each have a reference
			std::atomic<size_t> m_ref_count{2};

			::grpc::ServerContext m_context{};
			::grpc::ServerAsyncReaderWriter<TResponse, TRequest> m_stream{&m_context};

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
				m_stream.Finish(s, asyncpp::ptr_tag<2>(this));
			}
			void unhandled_exception() {
				if (!m_start_ok) return;
				this->ref();
				m_stream.Finish(util::exception_to_status(std::current_exception()), asyncpp::ptr_tag<1>(this));
			}

			template<typename TService>
			auto await_transform(start_bidi_streaming_tag<TService, TRequest, TResponse> s) noexcept {
				struct awaiter : asyncpp::grpc::calldata_interface {
					awaiter(promise_type* self, decltype(s) start) : m_self{self}, m_start{start} {}
					promise_type* m_self;
					decltype(s) m_start;
					asyncpp::coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						m_self->m_start_ok = ok;
						m_handle.resume();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(asyncpp::coroutine_handle<> h) noexcept {
						m_handle = h;
						(m_start.m_service->*(m_start.m_function))(&m_self->m_context, &m_self->m_stream, m_start.m_cq, m_start.m_cq, this);
					}
					std::tuple<bool, ::grpc::ServerContext&> await_resume() const noexcept { return {m_self->m_start_ok, m_self->m_context}; }
				};
				return awaiter{this, s};
			}

			auto await_transform(write_tag<TResponse> t) noexcept {
				struct awaiter : asyncpp::grpc::calldata_interface {
					awaiter(promise_type* self, const TResponse* r) : m_self{self}, m_msg{r} {}
					promise_type* m_self;
					const TResponse* m_msg;
					bool m_ok{false};
					asyncpp::coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						m_ok = ok;
						m_handle.resume();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(asyncpp::coroutine_handle<> h) noexcept {
						m_handle = h;
						m_self->m_stream.Write(*m_msg, this);
					}
					constexpr bool await_resume() const noexcept { return m_ok; }
				};
				return awaiter{this, t.m_msg};
			}

			auto await_transform(read_tag<TRequest> t) noexcept {
				struct awaiter : asyncpp::grpc::calldata_interface {
					awaiter(promise_type* self, TRequest* r) : m_self{self}, m_msg{r} {}
					promise_type* m_self;
					TRequest* m_msg;
					bool m_ok{false};
					asyncpp::coroutine_handle<> m_handle{};
					void handle_event(size_t, bool ok) noexcept override {
						m_ok = ok;
						m_handle.resume();
					}
					constexpr bool await_ready() const noexcept { return false; }
					constexpr void await_suspend(asyncpp::coroutine_handle<> h) noexcept {
						m_handle = h;
						m_self->m_stream.Read(m_msg, this);
					}
					constexpr bool await_resume() const noexcept { return m_ok; }
				};
				return awaiter{this, t.m_msg};
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
		bidi_streaming_task(handle_t h) : m_coro{h} {
			if (!m_coro) throw std::invalid_argument("m_coro is invalid");
			m_coro.resume();
		}

		/// \brief Move constructor
		bidi_streaming_task(bidi_streaming_task&& t) noexcept : m_coro(std::exchange(t.m_coro, {})) {}

		/// \brief Move assignment
		bidi_streaming_task& operator=(bidi_streaming_task&& t) noexcept {
			m_coro = std::exchange(t.m_coro, m_coro);
			return *this;
		}

		bidi_streaming_task(const bidi_streaming_task& o) : m_coro{o.m_coro} {
			if (m_coro) m_coro.promise().ref();
		}

		bidi_streaming_task& operator=(const bidi_streaming_task& o) {
			if (m_coro) m_coro.promise().unref();
			m_coro = o.m_coro;
			if (m_coro) m_coro.promise().ref();
			return *this;
		}

		/// \brief Destructor
		~bidi_streaming_task() {
			// Since we still have the coroutine, it has never been fire_and_forgeted, so we need to destroy it
			if (m_coro) m_coro.promise().unref();
		}

	private:
		handle_t m_coro;
	};

	template<typename TService, typename TRequest, typename TResponse, typename TService2>
	start_bidi_streaming_tag<TService, TRequest, TResponse> start(void (TService::*fn)(::grpc::ServerContext*,
																					   ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>*,
																					   ::grpc::CompletionQueue*, ::grpc::ServerCompletionQueue*, void*),
																  TService2* s, ::grpc::ServerCompletionQueue* cq) {
		return start_bidi_streaming_tag<TService, TRequest, TResponse>{fn, s, cq};
	}
} // namespace asyncpp::grpc
