#pragma once
#include <dummy.grpc.pb.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>
#include <memory>
#include <string>

class test_server : public DummyService::Service {
	std::unique_ptr<::grpc::Server> m_server;
	std::shared_ptr<::grpc::Channel> m_channel;
	std::unique_ptr<DummyService::Stub> m_stub;

public:
	test_server() {
		::grpc::ServerBuilder builder;
		builder.RegisterService(this);
		m_server = builder.BuildAndStart();
		m_channel = m_server->InProcessChannel(::grpc::ChannelArguments{});
		m_stub = DummyService::NewStub(m_channel);
	}

	const std::shared_ptr<::grpc::Channel>& channel() { return m_channel; }
	const std::unique_ptr<DummyService::Stub>& stub() { return m_stub; }

	~test_server() {
		m_stub.reset();
		m_channel.reset();
		m_server->Shutdown();
		m_server.reset();
	}

	::grpc::Status DummyUnary(::grpc::ServerContext*, const ::DummyUnaryRequest* request, ::DummyUnaryResponse* response) override {
		response->set_response("Hello " + request->name());
		return ::grpc::Status::OK;
	}

	::grpc::Status DummyServerStreaming(::grpc::ServerContext* context, const ::DummyServerStreamingRequest* request,
										::grpc::ServerWriter<::DummyServerStreamingResponse>* writer) override {
		DummyServerStreamingResponse resp;
		for (size_t i = 0; i < 10; i++) {
			resp.set_response(request->format() + std::to_string(i));
			writer->Write(resp);
		}
		return ::grpc::Status::OK;
	}

	::grpc::Status DummyClientStreaming(::grpc::ServerContext* context, ::grpc::ServerReader<::DummyClientStreamingRequest>* reader,
										::DummyClientStreamingResponse* response) override {
		for (DummyClientStreamingRequest req{}; reader->Read(&req);) {
			response->set_count(response->count() + req.msg().size());
		}
		return ::grpc::Status::OK;
	}

	::grpc::Status DummyBidiStreaming(::grpc::ServerContext* context,
									  ::grpc::ServerReaderWriter<::DummyBidiStreamingResponse, ::DummyBidiStreamingRequest>* stream) override {
		DummyBidiStreamingResponse resp;
		for (DummyBidiStreamingRequest req; stream->Read(&req);) {
			resp.set_count(req.msg().size());
			stream->Write(resp);
		}
		return ::grpc::Status::OK;
	}
};
