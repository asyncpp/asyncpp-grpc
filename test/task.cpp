#include "debug_allocator.h"
#include "test_async_server.h"
#include <asyncpp/fire_and_forget.h>
#include <asyncpp/grpc/generic_method_awaiter.h>
#include <asyncpp/grpc/task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

namespace {
	using Service = DummyService::AsyncService;
	template<auto FN>
	using alloc_task = task<FN, allocator_ref<debug_allocator>>;

	task<&Service::RequestDummyUnary> run_async_dummy_unary(Service* service, ::grpc::ServerCompletionQueue* cq) {
		// Initiate next call
		run_async_dummy_unary(service, cq);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		context.response().set_response("Hello " + context.request().name());
		co_return ::grpc::Status::OK;
	}

	alloc_task<&Service::RequestDummyUnary> run_async_dummy_unary_alloc(Service* service, ::grpc::ServerCompletionQueue* cq, debug_allocator& alloc) {
		// Initiate next call
		run_async_dummy_unary_alloc(service, cq, alloc);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		context.response().set_response("Hello " + context.request().name());
		co_return ::grpc::Status::OK;
	}

	asyncpp::fire_and_forget_task<> run_async_dummy_unary_generic(DummyService::AsyncService* service, ::grpc::ServerCompletionQueue* cq) {
		::grpc::ServerContext ctx;
		DummyUnaryRequest request;
		DummyUnaryResponse response;
		::grpc::ServerAsyncResponseWriter<DummyUnaryResponse> writer{&ctx};
		auto ok = co_await grpc_generic_method_awaiter{&DummyService::AsyncService::RequestDummyUnary, service, &ctx, &request, &writer, cq, cq};
		if (!ok) co_return;
		run_async_dummy_unary_generic(service, cq).start();

		response.set_response("Hello " + request.name());

		co_await grpc_generic_method_awaiter{&decltype(writer)::Finish, &writer, response, ::grpc::Status::OK};
	}

	task<&Service::RequestDummyBidiStreaming> run_async_dummy_bidi_streaming(Service* service, ::grpc::ServerCompletionQueue* cq) {
		// Initiate next call
		run_async_dummy_bidi_streaming(service, cq);

		// App logic
		co_await send_initial_metadata(); // Optional
		DummyBidiStreamingResponse resp;
		for (DummyBidiStreamingRequest req; co_await read(req);) {
			resp.set_count(req.msg().size());
			co_await write(resp);
		}
		co_return ::grpc::Status::OK;
	}

	alloc_task<&Service::RequestDummyBidiStreaming> run_async_dummy_bidi_streaming_alloc(Service* service, ::grpc::ServerCompletionQueue* cq,
																						 debug_allocator& alloc) {
		// Initiate next call
		run_async_dummy_bidi_streaming_alloc(service, cq, alloc);

		// App logic
		co_await send_initial_metadata(); // Optional
		DummyBidiStreamingResponse resp;
		for (DummyBidiStreamingRequest req; co_await read(req);) {
			resp.set_count(req.msg().size());
			co_await write(resp);
		}
		co_return ::grpc::Status::OK;
	}

	task<&Service::RequestDummyClientStreaming> run_async_dummy_client_streaming(Service* service, ::grpc::ServerCompletionQueue* cq) {
		// Initiate next call
		run_async_dummy_client_streaming(service, cq);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		for (DummyClientStreamingRequest req{}; co_await req;) {
			context.response().set_count(context.response().count() + req.msg().size());
		}
		co_return ::grpc::Status::OK;
	}

	alloc_task<&Service::RequestDummyClientStreaming> run_async_dummy_client_streaming_alloc(Service* service, ::grpc::ServerCompletionQueue* cq,
																							 debug_allocator& alloc) {
		// Initiate next call
		run_async_dummy_client_streaming_alloc(service, cq, alloc);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		for (DummyClientStreamingRequest req{}; co_await req;) {
			context.response().set_count(context.response().count() + req.msg().size());
		}
		co_return ::grpc::Status::OK;
	}

	task<&Service::RequestDummyServerStreaming> run_async_dummy_server_streaming(Service* service, ::grpc::ServerCompletionQueue* cq) {
		// Initiate next call
		run_async_dummy_server_streaming(service, cq);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		DummyServerStreamingResponse resp;
		// Both functional using co_await,
		for (int i = 0; i < 5; i++) {
			resp.set_response(context.request().format() + std::to_string(i));
			// Both return true if the write was successful and false if not.
			auto ok = co_await write(resp);
			if (!ok) break;
		}
		// as well as co_yield is supported for writing the result.
		for (int i = 5; i < 10; i++) {
			resp.set_response(context.request().format() + std::to_string(i));
			// Both return true if the write was successful and false if not.
			auto ok = co_yield resp;
			if (!ok) break;
		}
		co_return ::grpc::Status::OK;
	}

	alloc_task<&Service::RequestDummyServerStreaming> run_async_dummy_server_streaming_alloc(Service* service, ::grpc::ServerCompletionQueue* cq,
																							 debug_allocator& alloc) {
		// Initiate next call
		run_async_dummy_server_streaming_alloc(service, cq, alloc);

		// App logic
		auto& context = co_await rpc_info();
		co_await send_initial_metadata(); // Optional
		DummyServerStreamingResponse resp;
		// Both functional using co_await,
		for (int i = 0; i < 5; i++) {
			resp.set_response(context.request().format() + std::to_string(i));
			// Both return true if the write was successful and false if not.
			auto ok = co_await write(resp);
			if (!ok) break;
		}
		// as well as co_yield is supported for writing the result.
		for (int i = 5; i < 10; i++) {
			resp.set_response(context.request().format() + std::to_string(i));
			// Both return true if the write was successful and false if not.
			auto ok = co_yield resp;
			if (!ok) break;
		}
		co_return ::grpc::Status::OK;
	}

} // namespace

TEST(ASYNCPP_GRPC, GrpcServerUnaryTask) {
	test_async_server server;

	// Start initial call
	run_async_dummy_unary(&server, server.cq());

	DummyUnaryRequest req;
	DummyUnaryResponse resp;
	::grpc::ClientContext ctx;
	req.set_name("Dummy");
	auto res = server.stub()->DummyUnary(&ctx, req, &resp);
	ASSERT_TRUE(res.ok());
	ASSERT_EQ(resp.response(), "Hello " + req.name());
}

TEST(ASYNCPP_GRPC, GrpcServerUnaryTaskAllocator) {
	debug_allocator alloc{};
	{
		test_async_server server;

		// Start initial call
		run_async_dummy_unary_alloc(&server, server.cq(), alloc);

		DummyUnaryRequest req;
		DummyUnaryResponse resp;
		::grpc::ClientContext ctx;
		req.set_name("Dummy");
		auto res = server.stub()->DummyUnary(&ctx, req, &resp);
		ASSERT_TRUE(res.ok());
		ASSERT_EQ(resp.response(), "Hello " + req.name());
	}
	ASSERT_EQ(alloc.allocated_sum, alloc.released_sum);
	ASSERT_EQ(alloc.allocated_count, alloc.released_count);
	ASSERT_NE(0, alloc.released_sum);
	ASSERT_NE(0, alloc.released_count);
}

TEST(ASYNCPP_GRPC, GrpcServerGenericAwaiter) {
	test_async_server server;

	// Start initial call
	run_async_dummy_unary_generic(&server, server.cq()).start();

	DummyUnaryRequest req;
	DummyUnaryResponse resp;
	::grpc::ClientContext ctx;
	req.set_name("Dummy");
	auto res = server.stub()->DummyUnary(&ctx, req, &resp);
	ASSERT_TRUE(res.ok());
	ASSERT_EQ(resp.response(), "Hello " + req.name());
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

TEST(ASYNCPP_GRPC, GrpcServerBidiStreamingTaskAllocator) {
	debug_allocator alloc{};
	{
		test_async_server server;

		// Start initial call
		run_async_dummy_bidi_streaming_alloc(&server, server.cq(), alloc);

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
	ASSERT_EQ(alloc.allocated_sum, alloc.released_sum);
	ASSERT_EQ(alloc.allocated_count, alloc.released_count);
	ASSERT_NE(0, alloc.released_sum);
	ASSERT_NE(0, alloc.released_count);
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

TEST(ASYNCPP_GRPC, GrpcServerClientStreamingTaskAllocator) {
	debug_allocator alloc{};
	{
		test_async_server server;

		// Start initial call
		run_async_dummy_client_streaming_alloc(&server, server.cq(), alloc);

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
	ASSERT_EQ(alloc.allocated_sum, alloc.released_sum);
	ASSERT_EQ(alloc.allocated_count, alloc.released_count);
	ASSERT_NE(0, alloc.released_sum);
	ASSERT_NE(0, alloc.released_count);
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

TEST(ASYNCPP_GRPC, GrpcServerServerStreamingTaskAllocator) {
	debug_allocator alloc{};
	{
		test_async_server server;

		// Start initial call
		run_async_dummy_server_streaming_alloc(&server, server.cq(), alloc);

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
	ASSERT_EQ(alloc.allocated_sum, alloc.released_sum);
	ASSERT_EQ(alloc.allocated_count, alloc.released_count);
	ASSERT_NE(0, alloc.released_sum);
	ASSERT_NE(0, alloc.released_count);
}
