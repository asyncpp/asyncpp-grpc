#pragma once
#include <cstddef>

namespace asyncpp::grpc {
	class calldata_interface {
	public:
		virtual ~calldata_interface() noexcept {}
		virtual void handle_event(size_t evt, bool ok) noexcept = 0;
	};
} // namespace asyncpp::grpc
