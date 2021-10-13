#include "test_async_server.h"
#include <asyncpp/grpc/server_client_streaming_task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

server_client_streaming_task<DummyClientStreamingRequest, DummyClientStreamingResponse> run_async_dummy_client_streaming(DummyService::AsyncService* service,
																														 ::grpc::ServerCompletionQueue* cq) {
	auto [ok, resp, ctx] = co_await start(&DummyService::AsyncService::RequestDummyClientStreaming, service, cq);
	if (!ok) co_return ::grpc::Status::CANCELLED;
	run_async_dummy_client_streaming(service, cq);

	// App logic
	DummyClientStreamingRequest req;
	while (co_await read(req)) {
		resp.set_count(resp.count() + req.msg().size());
	}
	co_return ::grpc::Status::OK;
}

TEST(ASYNCPP_GRPC, GrpcServerClientStreamingTask) {
	test_async_server server;

	// Start initial call
	run_async_dummy_client_streaming(&server, server.cq());

	DummyClientStreamingRequest req;
	DummyClientStreamingResponse resp;
	::grpc::ClientContext ctx;
	auto stream = server.stub()->DummyClientStreaming(&ctx, &resp);
	for (int i = 0; i < 10; i++) {
		req.set_msg("Dummy");
		stream->Write(req);
	}
	stream->WritesDone();
	auto res = stream->Finish();
	ASSERT_TRUE(res.ok());
	ASSERT_EQ(50, resp.count());
}
