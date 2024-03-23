#include <asyncpp/grpc/client_cq.h>
#include <asyncpp/ptr_tag.h>

namespace asyncpp::grpc {
	void client_cq::handle_event(size_t, bool) noexcept {
		m_alarm_set.clear();
		auto task = m_dispatched.pop();
		while (task) {
			if (*task) (*task)();
			task = m_dispatched.pop();
		}
	}

	client_cq::client_cq() {
		m_thread = std::thread([this]() {
#ifdef __linux__
			pthread_setname_np(pthread_self(), "grpc_client_cq");
#endif
			void* tag = nullptr;
			bool ok = false;
			while (this->m_cq.Next(&tag, &ok)) {
				auto [i, t] = ptr_untag<calldata_interface>(tag);
				if (!i) continue;
				i->handle_event(t, ok);
			}
		});
	}

	client_cq::~client_cq() {
		m_cq.Shutdown();
		if (m_thread.joinable()) m_thread.join();
	}

	void client_cq::push(std::function<void()> fn) {
		m_dispatched.emplace(std::move(fn));
		if (!m_alarm_set.test_and_set()) { m_alarm.Set(&m_cq, gpr_time_0(GPR_CLOCK_REALTIME), ptr_tag<0, calldata_interface>(this)); }
	}

	std::shared_ptr<client_cq> client_cq::get_default() {
		static auto instance = std::make_shared<client_cq>();
		return instance;
	}
} // namespace asyncpp::grpc