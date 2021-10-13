#include "test_async_server.h"
#include <asyncpp/grpc/server_bidi_streaming_task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

server_bidi_streaming_task<DummyBidiStreamingRequest, DummyBidiStreamingResponse> run_async_dummy_bidi_streaming(DummyService::AsyncService* service,
																												 ::grpc::ServerCompletionQueue* cq) {
	auto [ok, ctx] = co_await start(&DummyService::AsyncService::RequestDummyBidiStreaming, service, cq);
	if (!ok) co_return ::grpc::Status::CANCELLED;
	run_async_dummy_bidi_streaming(service, cq);

	// App logic
	DummyBidiStreamingRequest req;
	DummyBidiStreamingResponse resp;
	while (co_await read(req)) {
		resp.set_count(req.msg().size());
		co_await write(resp);
	}
	co_return ::grpc::Status::OK;
}

TEST(ASYNCPP_GRPC, GrpcServerBidiStreamingTask) {
	test_async_server server;

	// Start initial call
	run_async_dummy_bidi_streaming(&server, server.cq());

	::grpc::ClientContext ctx;
	auto stream = server.stub()->DummyBidiStreaming(&ctx);
	for (int i = 0; i < 10; i++) {
		DummyBidiStreamingRequest req;
		req.set_msg("Dummy");
		stream->Write(req);
		DummyBidiStreamingResponse resp;
		stream->Read(&resp);
		ASSERT_EQ(resp.count(), 5);
	}
	stream->WritesDone();
	auto res = stream->Finish();
	ASSERT_TRUE(res.ok());
}
