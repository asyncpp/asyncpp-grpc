#pragma once
#include <asyncpp/dispatcher.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/ptr_tag.h>
#include <asyncpp/threadsafe_queue.h>

#include <grpc/impl/codegen/compression_types.h>
#include <grpc/support/time.h>
#include <grpc/support/workaround_list.h>
#include <grpcpp/alarm.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/channel_arguments.h>

#include <atomic>
#include <thread>

namespace asyncpp::grpc {

	class server_builder;
	class server : public dispatcher, private calldata_interface {
	public:
		server(const server&) = delete;
		server(server&&) = delete;
		server& operator=(const server&) = delete;
		server& operator=(server&&) = delete;

		::grpc::Server* raw_server() noexcept;
		const ::grpc::Server* raw_server() const noexcept;
		::grpc::ServerCompletionQueue* raw_cq(size_t idx) noexcept;
		const ::grpc::ServerCompletionQueue* raw_cq(size_t idx) const noexcept;
		size_t thread_count() const noexcept;
		bool is_running() const noexcept;
		void stop();
		~server();
		void push(std::function<void()> fn) override;
		void push(std::function<void()> fn, size_t cq_idx);

	private:
		friend class server_builder;
		std::unique_ptr<::grpc::Server> m_server;
		std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> m_cqs;
		std::vector<std::thread> m_threads;
		std::atomic<size_t> m_threads_online{0};
		std::vector<std::function<void(::grpc::ServerCompletionQueue*)>> m_cq_initializers;

		threadsafe_queue<std::function<void()>> m_dispatch_queue;
		::grpc::Alarm m_dispatch_alarm;
		std::atomic<bool> m_dispatch_need_alarm{true};

		server(size_t thread_count, ::grpc::ServerBuilder& builder, std::vector<std::function<void(::grpc::ServerCompletionQueue*)>> cqinits);
		void handle_event(size_t evt, bool ok) noexcept override;
	};

	class server_builder {
		size_t m_thread_count{std::thread::hardware_concurrency()};
		::grpc::ServerBuilder m_builder;
		std::vector<std::function<void(::grpc::ServerCompletionQueue*)>> m_cq_initializers;

	public:
		template<typename T>
		class service_builder;
		class service_builder_base;

		server_builder() noexcept = default;
		server_builder(const server_builder&) = delete;
		server_builder(server_builder&&) = delete;
		server_builder& operator=(const server_builder&) = delete;
		server_builder& operator=(server_builder&&) = delete;

		server_builder& set_thread_count(size_t nthreads) noexcept;
		::grpc::ServerBuilder& raw_builder() noexcept;
		const ::grpc::ServerBuilder& raw_builder() const noexcept;

		template<typename S>
		service_builder<S> add_service(S* instance) noexcept {
			m_builder.RegisterService(instance);
			return service_builder<S>(this, instance);
		}

		template<typename S>
		service_builder<S> add_service(const std::string& host, S* instance) noexcept {
			m_builder.RegisterService(host, instance);
			return service_builder<S>(this, instance);
		}

		server_builder& add_port(const std::string& addr_uri, std::shared_ptr<::grpc::ServerCredentials> creds, int* selected_port = nullptr);
		server_builder& set_max_receive_message_size(size_t max_size);
		server_builder& set_max_send_message_size(size_t max_size);
		server_builder& set_max_message_size(size_t max_size);
		server_builder& set_compression_algorithm_support_status(grpc_compression_algorithm algorithm, bool enabled);
		server_builder& set_default_compression_level(grpc_compression_level level);
		server_builder& set_default_compression_algorithm(grpc_compression_algorithm algorithm);
		server_builder& set_resource_quota(const ::grpc::ResourceQuota& quota);
		server_builder& set_option(std::unique_ptr<::grpc::ServerBuilderOption> option);
		server_builder& enable_workaround(grpc_workaround_list id);
		std::unique_ptr<server> build();
	};

	class server_builder::service_builder_base {
		friend class server_builder;

	protected:
		server_builder* m_parent;
		service_builder_base(server_builder* parent) noexcept : m_parent(parent) {}

	public:
		service_builder_base(const service_builder_base&) = delete;
		service_builder_base(service_builder_base&&) = delete;
		service_builder_base& operator=(const service_builder_base&) = delete;
		service_builder_base& operator=(service_builder_base&&) = delete;

		server_builder& add_port(const std::string& addr_uri, std::shared_ptr<::grpc::ServerCredentials> creds, int* selected_port = nullptr);
		server_builder& set_max_receive_message_size(size_t max_size);
		server_builder& set_max_send_message_size(size_t max_size);
		server_builder& set_max_message_size(size_t max_size);
		server_builder& set_compression_algorithm_support_status(grpc_compression_algorithm algorithm, bool enabled);
		server_builder& set_default_compression_level(grpc_compression_level level);
		server_builder& set_default_compression_algorithm(grpc_compression_algorithm algorithm);
		server_builder& set_resource_quota(const ::grpc::ResourceQuota& quota);
		server_builder& set_option(std::unique_ptr<::grpc::ServerBuilderOption> option);
		server_builder& enable_workaround(grpc_workaround_list id);
		std::unique_ptr<server> build();
	};

	template<typename T>
	class server_builder::service_builder : public service_builder_base {
		friend class server_builder;
		T* m_service;

		service_builder(server_builder* parent, T* instance) noexcept : service_builder_base(parent), m_service(instance) {}

	public:
		template<typename S>
		service_builder<S> add_service(S* instance) noexcept {
			return m_parent->add_service(instance);
		}
		template<typename S>
		service_builder<S> add_service(const std::string& host, S* instance) noexcept {
			return m_parent->add_service(host, instance);
		}
		template<typename... Args, typename FN>
		service_builder& add_task(FN&& fn, Args... args)
			requires(std::is_invocable<FN, T*, ::grpc::ServerCompletionQueue*, Args...>::value)
		{
			m_parent->m_cq_initializers.emplace_back(
				[fn, service = m_service, args...](::grpc::ServerCompletionQueue* cq) mutable { fn(service, cq, args...); });
			return *this;
		}
		template<typename Task, typename Class, typename... Args>
		service_builder& add_task(Task (Class::*fn)(T*, ::grpc::ServerCompletionQueue*, Args...), Class* instance, Args... args) {
			m_parent->m_cq_initializers.emplace_back(
				[fn, service = m_service, instance, args...](::grpc::ServerCompletionQueue* cq) mutable { (instance->*fn)(service, cq, args...); });
			return *this;
		}
	};
} // namespace asyncpp::grpc
