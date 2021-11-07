#include "test_async_server.h"
#include <asyncpp/grpc/server_streaming_task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

server_streaming_task<DummyServerStreamingResponse> run_async_dummy_server_streaming(DummyService::AsyncService* service,
																							::grpc::ServerCompletionQueue* cq) {
	auto [ok, req, ctx] = co_await start(&DummyService::AsyncService::RequestDummyServerStreaming, service, cq);
	if (!ok) co_return ::grpc::Status::CANCELLED;
	run_async_dummy_server_streaming(service, cq);

	// App logic
	DummyServerStreamingResponse resp;
	for (int i = 0; i < 10; i++) {
		resp.set_response(req.format() + std::to_string(i));
		co_await write(resp);
	}
	co_return ::grpc::Status::OK;
}

TEST(ASYNCPP_GRPC, GrpcServerServerStreamingTask) {
	test_async_server server;

	// Start initial call
	run_async_dummy_server_streaming(&server, server.cq());

	DummyServerStreamingRequest req;
	::grpc::ClientContext ctx;
	req.set_format("Dummy ");
	auto stream = server.stub()->DummyServerStreaming(&ctx, req);
	DummyServerStreamingResponse resp;
	int i = 0;
	while (stream->Read(&resp)) {
		ASSERT_EQ(resp.response(), req.format() + std::to_string(i++));
	}
	ASSERT_EQ(i, 10);
	auto res = stream->Finish();
	ASSERT_TRUE(res.ok());
}
