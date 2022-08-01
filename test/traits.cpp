#include <dummy.grpc.pb.h>
#include <asyncpp/grpc/traits.h>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;

TEST(ASYNCPP_GRPC, GrpcTraits) {
	using Stub = DummyService::Stub;
	using StubInterface = DummyService::StubInterface;

	// Unary call
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyUnary)>::request_type, DummyUnaryRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyUnary)>::response_type, DummyUnaryResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyUnary)>::service_type, Stub>);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyUnary)>::is_server_side);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyUnary)>::is_client_streaming);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyUnary)>::is_server_streaming);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyUnary)>::is_streaming);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyUnary)>::request_type, DummyUnaryRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyUnary)>::response_type, DummyUnaryResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyUnary)>::service_type, StubInterface>);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyUnary)>::is_server_side);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyUnary)>::is_client_streaming);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyUnary)>::is_server_streaming);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyUnary)>::is_streaming);

	// Client streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::request_type, DummyClientStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::response_type, DummyClientStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::service_type, Stub>);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::is_client_streaming);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&Stub::AsyncDummyClientStreaming)>::is_streaming);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::request_type, DummyClientStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::response_type, DummyClientStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::service_type, StubInterface>);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::is_client_streaming);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyClientStreaming)>::is_streaming);

	// Server streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::request_type, DummyServerStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::response_type, DummyServerStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::service_type, Stub>);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::is_server_side);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&Stub::AsyncDummyServerStreaming)>::is_streaming);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::request_type, DummyServerStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::response_type, DummyServerStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::service_type, StubInterface>);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::is_server_side);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyServerStreaming)>::is_streaming);

	// Bidi streaming call
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::request_type, DummyBidiStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::response_type, DummyBidiStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::service_type, Stub>);
	static_assert(!method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&Stub::AsyncDummyBidiStreaming)>::is_streaming);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::request_type, DummyBidiStreamingRequest>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::response_type, DummyBidiStreamingResponse>);
	static_assert(std::is_same_v<typename method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::service_type, StubInterface>);
	static_assert(!method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::is_server_side);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::is_client_streaming);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::is_server_streaming);
	static_assert(method_traits<decltype(&StubInterface::AsyncDummyBidiStreaming)>::is_streaming);
}
