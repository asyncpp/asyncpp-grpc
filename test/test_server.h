#pragma once
#include <dummy.grpc.pb.h>
#include <grpcpp/server_builder.h>

class test_server : public DummyService::Service {
	std::unique_ptr<::grpc::Server> m_server;

public:
	test_server() {
		::grpc::ServerBuilder builder;
		builder.RegisterService(this);
		m_server = builder.BuildAndStart();
	}

	auto channel() { return m_server->InProcessChannel(::grpc::ChannelArguments{}); }

	std::unique_ptr<DummyService::Stub> stub() { return DummyService::NewStub(channel()); }

	~test_server() {
		m_server->Shutdown();
		m_server.reset();
	}

	::grpc::Status DummyUnary(::grpc::ServerContext*, const ::DummyUnaryRequest* request, ::DummyUnaryResponse* response) override {
		response->set_response("Hello " + request->name());
		return ::grpc::Status::OK;
	}
};
