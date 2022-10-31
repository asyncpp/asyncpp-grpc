#pragma once
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/ptr_tag.h>
#include <dummy.grpc.pb.h>
#include <grpcpp/channel.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <thread>

class test_async_server : public DummyService::AsyncService {
	std::unique_ptr<::grpc::Server> m_server;
	std::unique_ptr<::grpc::ServerCompletionQueue> m_cq;
	std::thread m_th;

	void event_loop() {
		void* tag = nullptr;
		bool ok = false;
		while (m_cq->Next(&tag, &ok)) {
			auto [i, t] = asyncpp::ptr_untag<asyncpp::grpc::calldata_interface>(tag);
			if (!i) continue;
			i->handle_event(t, ok);
		}
	}

public:
	test_async_server() {
		::grpc::ServerBuilder builder;
		builder.RegisterService(this);
		m_cq = builder.AddCompletionQueue();
		if (!m_cq) throw std::runtime_error("failed to add completion queue");
		m_server = builder.BuildAndStart();
		if (!m_server) throw std::runtime_error("failed to start server");
		m_th = std::thread(std::bind(&test_async_server::event_loop, this));
	}

	~test_async_server() {
		if (m_server) m_server->Shutdown();
		if (m_cq) m_cq->Shutdown();
		if (m_th.joinable()) m_th.join();
		m_server.reset();
		m_cq.reset();
	}

	auto cq() { return m_cq.get(); }
	auto channel() { return m_server->InProcessChannel(::grpc::ChannelArguments{}); }
	std::unique_ptr<DummyService::Stub> stub() { return DummyService::NewStub(channel()); }
};
