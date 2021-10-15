#include "test_cq.h"
#include "test_server.h"
#include <asyncpp/grpc/client_unary.h>
#include <asyncpp/grpc/util.h>
#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

TEST(ASYNCPP_GRPC, GrpcAsyncUnaryFinish) {
	test_server server;
	test_cq cq;
	asyncpp::as_promise([](std::unique_ptr<DummyService::Stub> stub, ::grpc::CompletionQueue& cq) -> asyncpp::task<> {
		DummyUnaryRequest req;
		DummyUnaryResponse resp;
		::grpc::ClientContext ctx;

		req.set_name("Dummy");
		auto res = stub->AsyncDummyUnary(&ctx, req, &cq);
		auto status = co_await finish(res, resp);

		if (!status.ok()) throw std::runtime_error("!status.ok()");
		if (resp.response() != "Hello " + req.name()) throw std::runtime_error("resp.response() != expected");
		co_return;
	}(server.stub(), cq));
}

TEST(ASYNCPP_GRPC, GrpcAsyncUnaryCall) {
	test_server server;
	test_cq cq;

	unary_call<&DummyService::Stub::AsyncDummyUnary> c;
	c.request.set_name("Dummy");
	auto status =
		asyncpp::as_promise<::grpc::Status>([&c](std::unique_ptr<DummyService::Stub> stub, ::grpc::CompletionQueue& cq) -> asyncpp::task<::grpc::Status> {
			co_return co_await c.invoke(stub, cq);
		}(server.stub(), cq))
			.get();
	ASSERT_TRUE(status.ok());
	ASSERT_EQ(c.response.response(), "Hello " + c.request.name());
	ASSERT_EQ(c.status.error_code(), status.error_code());
	ASSERT_EQ(c.status.error_details(), status.error_details());
	ASSERT_EQ(c.status.error_message(), status.error_message());
}

TEST(ASYNCPP_GRPC, GrpcTraits) {
	// Unary call
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::request_type, DummyUnaryRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::response_type, DummyUnaryResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::service_type, DummyService::Stub>);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::is_server_side);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::is_client_streaming);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::is_server_streaming);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyUnary)>::is_streaming);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::request_type, DummyUnaryRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::response_type, DummyUnaryResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::service_type, DummyService::StubInterface>);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::is_server_side);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::is_client_streaming);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::is_server_streaming);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyUnary)>::is_streaming);

	// Client streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::request_type, DummyClientStreamingRequest>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::response_type, DummyClientStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::service_type, DummyService::Stub>);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::is_client_streaming);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyClientStreaming)>::is_streaming);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::request_type, DummyClientStreamingRequest>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::response_type, DummyClientStreamingResponse>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::service_type, DummyService::StubInterface>);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::is_client_streaming);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyClientStreaming)>::is_streaming);

	// Server streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::request_type, DummyServerStreamingRequest>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::response_type, DummyServerStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::service_type, DummyService::Stub>);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::is_server_side);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyServerStreaming)>::is_streaming);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::request_type, DummyServerStreamingRequest>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::response_type, DummyServerStreamingResponse>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::service_type, DummyService::StubInterface>);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::is_server_side);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyServerStreaming)>::is_streaming);

	// Bidi streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::request_type, DummyBidiStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::response_type, DummyBidiStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::service_type, DummyService::Stub>);
	static_assert(!method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::Stub::AsyncDummyBidiStreaming)>::is_streaming);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::request_type, DummyBidiStreamingRequest>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::response_type, DummyBidiStreamingResponse>);
	static_assert(
		std::is_same_v<typename method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::service_type, DummyService::StubInterface>);
	static_assert(!method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&DummyService::StubInterface::AsyncDummyBidiStreaming)>::is_streaming);
}
