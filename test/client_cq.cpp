#include <asyncpp/grpc/client_cq.h>
#include <future>
#include <gtest/gtest.h>

using namespace asyncpp::grpc;
using namespace asyncpp;

TEST(ASYNCPP_GRPC, ClientCQPush) {
	auto cq = client_cq::get_default();
	std::promise<std::thread::id> p;
	auto future = p.get_future();
	cq->push([&p]() { p.set_value(std::this_thread::get_id()); });
	ASSERT_NE(std::this_thread::get_id(), future.get());
}
