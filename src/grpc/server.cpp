#include <asyncpp/grpc/server.h>

namespace asyncpp::grpc {
	server::server(size_t thread_count, ::grpc::ServerBuilder& builder, std::vector<std::function<void(::grpc::ServerCompletionQueue*)>> cqinits)
		: m_cq_initializers(std::move(cqinits)) {
		m_cqs.reserve(thread_count);
		m_threads.reserve(thread_count);
		for (size_t i = 0; i < thread_count; i++) {
			m_cqs.push_back(builder.AddCompletionQueue());
		}
		m_server = builder.BuildAndStart();

		for (size_t i = 0; i < thread_count; i++) {
			m_threads.emplace_back([this, i]() mutable {
				dispatcher::current(this);
#ifdef __linux__
				pthread_setname_np(pthread_self(), ("rpcworker_" + std::to_string(i)).c_str());
#endif
				auto cq = m_cqs[i].get();
				for (auto& e : m_cq_initializers) {
					e(cq);
				}
				if (m_threads_online.fetch_add(1) + 1 == m_cqs.size()) m_cq_initializers.clear();

				void* tag = nullptr;
				bool ok = false;
				while (cq->Next(&tag, &ok)) {
					auto [i, t] = ptr_untag<calldata_interface>(tag);
					if (i) i->handle_event(t, ok);
				}
				dispatcher::current(nullptr);
			});
		}
	}

	void server::handle_event(size_t evt, bool ok) noexcept {
		while (true) {
			auto cb = m_dispatch_queue.pop();
			if (!cb) break;
			if (*cb) (*cb)();
		}
		m_dispatch_need_alarm.store(true);
		while (true) {
			auto cb = m_dispatch_queue.pop();
			if (!cb) break;
			if (*cb) (*cb)();
		}
	}

	::grpc::Server* server::raw_server() noexcept { return m_server.get(); }

	const ::grpc::Server* server::raw_server() const noexcept { return m_server.get(); }

	::grpc::ServerCompletionQueue* server::raw_cq(size_t idx) noexcept {
		if (idx >= m_cqs.size()) return nullptr;
		return m_cqs[idx].get();
	}

	const ::grpc::ServerCompletionQueue* server::raw_cq(size_t idx) const noexcept {
		if (idx >= m_cqs.size()) return nullptr;
		return m_cqs[idx].get();
	}

	size_t server::thread_count() const noexcept { return m_threads.size(); }

	bool server::is_running() const noexcept { return m_server != nullptr; }

	void server::stop() {
		for (auto& e : m_cqs) {
			if (e) e->Shutdown();
		}
		if (m_server) m_server->Shutdown();
		for (auto& e : m_threads) {
			if (e.joinable()) e.join();
		}
		m_cqs.clear();
		m_threads.clear();
		m_server.reset();
	}

	server::~server() { stop(); }

	void server::push(std::function<void()> fn) { push(std::move(fn), 0); }

	void server::push(std::function<void()> fn, size_t cq_idx) {
		m_dispatch_queue.emplace(std::move(fn));
		if (m_dispatch_need_alarm.exchange(false)) {
			cq_idx %= m_cqs.size();
			m_dispatch_alarm.Set(m_cqs[cq_idx].get(), gpr_time_0(gpr_clock_type::GPR_CLOCK_MONOTONIC), static_cast<calldata_interface*>(this));
		}
	}

	server_builder& server_builder::set_thread_count(size_t nthreads) noexcept {
		m_thread_count = nthreads;
		return *this;
	}

	::grpc::ServerBuilder& server_builder::raw_builder() noexcept { return m_builder; }

	const ::grpc::ServerBuilder& server_builder::raw_builder() const noexcept { return m_builder; }

	server_builder& server_builder::add_port(const std::string& addr_uri, std::shared_ptr<::grpc::ServerCredentials> creds, int* selected_port) {
		m_builder.AddListeningPort(addr_uri, creds, selected_port);
		return *this;
	}

	server_builder& server_builder::set_max_receive_message_size(size_t max_size) {
		m_builder.SetMaxReceiveMessageSize(max_size);
		return *this;
	}

	server_builder& server_builder::set_max_send_message_size(size_t max_size) {
		m_builder.SetMaxSendMessageSize(max_size);
		return *this;
	}

	server_builder& server_builder::set_max_message_size(size_t max_size) {
		m_builder.SetMaxMessageSize(max_size);
		return *this;
	}

	server_builder& server_builder::set_compression_algorithm_support_status(grpc_compression_algorithm algorithm, bool enabled) {
		m_builder.SetCompressionAlgorithmSupportStatus(algorithm, enabled);
		return *this;
	}

	server_builder& server_builder::set_default_compression_level(grpc_compression_level level) {
		m_builder.SetDefaultCompressionLevel(level);
		return *this;
	}

	server_builder& server_builder::set_default_compression_algorithm(grpc_compression_algorithm algorithm) {
		m_builder.SetDefaultCompressionAlgorithm(algorithm);
		return *this;
	}

	server_builder& server_builder::set_resource_quota(const ::grpc::ResourceQuota& quota) {
		m_builder.SetResourceQuota(quota);
		return *this;
	}

	server_builder& server_builder::set_option(std::unique_ptr<::grpc::ServerBuilderOption> option) {
		m_builder.SetOption(std::move(option));
		return *this;
	}

	server_builder& server_builder::enable_workaround(grpc_workaround_list id) {
		m_builder.EnableWorkaround(id);
		return *this;
	}

	std::unique_ptr<server> server_builder::build() { return std::unique_ptr<server>{new server(m_thread_count, m_builder, std::move(m_cq_initializers))}; }

	server_builder& server_builder::service_builder_base::add_port(const std::string& addr_uri, std::shared_ptr<::grpc::ServerCredentials> creds,
																   int* selected_port) {
		return m_parent->add_port(addr_uri, creds, selected_port);
	}

	server_builder& server_builder::service_builder_base::set_max_receive_message_size(size_t max_size) {
		return m_parent->set_max_receive_message_size(max_size);
	}

	server_builder& server_builder::service_builder_base::set_max_send_message_size(size_t max_size) { return m_parent->set_max_send_message_size(max_size); }

	server_builder& server_builder::service_builder_base::set_max_message_size(size_t max_size) { return m_parent->set_max_message_size(max_size); }

	server_builder& server_builder::service_builder_base::set_compression_algorithm_support_status(grpc_compression_algorithm algorithm, bool enabled) {
		return m_parent->set_compression_algorithm_support_status(algorithm, enabled);
	}

	server_builder& server_builder::service_builder_base::set_default_compression_level(grpc_compression_level level) {
		return m_parent->set_default_compression_level(level);
	}

	server_builder& server_builder::service_builder_base::set_default_compression_algorithm(grpc_compression_algorithm algorithm) {
		return m_parent->set_default_compression_algorithm(algorithm);
	}

	server_builder& server_builder::service_builder_base::set_resource_quota(const ::grpc::ResourceQuota& quota) { return m_parent->set_resource_quota(quota); }

	server_builder& server_builder::service_builder_base::set_option(std::unique_ptr<::grpc::ServerBuilderOption> option) {
		return m_parent->set_option(std::move(option));
	}

	server_builder& server_builder::service_builder_base::enable_workaround(grpc_workaround_list id) { return m_parent->enable_workaround(id); }

	std::unique_ptr<server> server_builder::service_builder_base::build() { return m_parent->build(); }
} // namespace asyncpp::grpc
