#include "test_async_server.h"
#include <asyncpp/fire_and_forget.h>
#include <asyncpp/grpc/generic_method_awaiter.h>
#include <asyncpp/grpc/unary_task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

unary_task<DummyUnaryResponse> run_async_dummy_unary(DummyService::AsyncService* service, ::grpc::ServerCompletionQueue* cq) {
	auto [ok, req, resp, ctx] = co_await start(&DummyService::AsyncService::RequestDummyUnary, service, cq);
	if (!ok) co_return ::grpc::Status::CANCELLED;
	run_async_dummy_unary(service, cq);

	// App logic
	resp.set_response("Hello " + req.name());
	co_return ::grpc::Status::OK;
}

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

asyncpp::fire_and_forget_task run_async_dummy_unary_generic(DummyService::AsyncService* service, ::grpc::ServerCompletionQueue* cq) {
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
