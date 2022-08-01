#include "dummy.pb.h"
#include "test_cq.h"
#include "test_server.h"
#include <asyncpp/grpc/call.h>
#include <asyncpp/grpc/util.h>
#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>
#include <gtest/gtest.h>
#include <string>

using namespace asyncpp::grpc;
using namespace asyncpp;

TEST(ASYNCPP_GRPC, GrpcAsyncUnaryCall) {
	test_server server;
	test_cq cq;

	DummyUnaryRequest req;
	DummyUnaryResponse resp;
	call<&DummyService::Stub::AsyncDummyUnary> c{server.stub()};
	req.set_name("Dummy");
	auto status = as_promise(c(req, resp, &cq)).get();
	ASSERT_TRUE(status.ok());
	ASSERT_EQ(resp.response(), "Hello " + req.name());
}

TEST(ASYNCPP_GRPC, GrpcAsyncServerStreamingCall) {
	test_server server;
	test_cq cq;

	DummyServerStreamingRequest req;
	req.set_format("test_");
	DummyServerStreamingResponse resp;
	call<&DummyService::Stub::AsyncDummyServerStreaming> c{server.stub()};
	as_promise(c.start(req, &cq)).get();
	for (size_t i{0}; as_promise(c.read(resp)).get(); i++) {
		ASSERT_EQ(resp.response(), "test_" + std::to_string(i));
	}
	auto status = as_promise(c.finish()).get();
	ASSERT_TRUE(status.ok());
}

TEST(ASYNCPP_GRPC, GrpcAsyncClientStreamingCall) {
	test_server server;
	test_cq cq;

	DummyClientStreamingRequest req;
	req.set_msg("01234");
	DummyClientStreamingResponse resp;
	call<&DummyService::Stub::AsyncDummyClientStreaming> c{server.stub()};
	as_promise(c.start(resp, &cq)).get();
	for (int i = 0; i < 10; i++) {
		req.set_msg("Dummy");
		ASSERT_TRUE(as_promise(c.write(req)).get());
	}
	auto res = as_promise(c.finish()).get();
	ASSERT_TRUE(res.ok());
	ASSERT_EQ(50, resp.count());
}

TEST(ASYNCPP_GRPC, GrpcAsyncBidiStreamingCall) {
	test_server server;
	test_cq cq;

	call<&DummyService::Stub::AsyncDummyBidiStreaming> c{server.stub()};
	as_promise(c.start(&cq)).get();
	for (int i = 0; i < 10; i++) {
		DummyBidiStreamingRequest req;
		req.set_msg("Dummy");
		ASSERT_TRUE(as_promise(c.write(req)).get());
		DummyBidiStreamingResponse resp;
		as_promise(c.read(resp)).get();
		ASSERT_EQ(resp.count(), 5);
	}
	auto res = as_promise(c.finish()).get();
	ASSERT_TRUE(res.ok());
}
