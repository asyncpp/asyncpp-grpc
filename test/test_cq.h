#pragma once
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/ptr_tag.h>
#include <grpcpp/completion_queue.h>
#include <thread>

class test_cq : public ::grpc::CompletionQueue {
	std::thread m_th;

public:
	test_cq() {
		m_th = std::thread([this]() {
			void* tag = nullptr;
			bool ok = false;
			while (this->Next(&tag, &ok)) {
				auto [i, t] = asyncpp::ptr_untag<asyncpp::grpc::calldata_interface>(tag);
				if (!i) continue;
				i->handle_event(t, ok);
			}
		});
	}
	~test_cq() {
		Shutdown();
		if (m_th.joinable()) m_th.join();
	}
};
