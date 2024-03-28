#pragma once
#include <asyncpp/dispatcher.h>
#include <asyncpp/grpc/calldata_interface.h>
#include <asyncpp/threadsafe_queue.h>
#include <grpcpp/alarm.h>
#include <grpcpp/completion_queue.h>

#include <memory>
#include <thread>

namespace asyncpp::grpc {
	class client_cq : public dispatcher, private grpc::calldata_interface {
		std::thread m_thread;
		threadsafe_queue<std::function<void()>> m_dispatched;
		std::atomic_flag m_alarm_set;
		::grpc::Alarm m_alarm;
		::grpc::CompletionQueue m_cq;

		void handle_event(size_t evt, bool ok) noexcept override;

	public:
		client_cq();
		client_cq(const client_cq&) = delete;
		client_cq(client_cq&&) = delete;
		client_cq& operator=(const client_cq&) = delete;
		client_cq& operator=(client_cq&&) = delete;
		~client_cq();

		void push(std::function<void()> fn) override;

		::grpc::CompletionQueue& cq() noexcept { return m_cq; }
		const ::grpc::CompletionQueue& cq() const noexcept { return m_cq; }

		static std::shared_ptr<client_cq> get_default();
	};
} // namespace asyncpp::grpc
