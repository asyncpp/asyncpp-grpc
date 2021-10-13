#pragma once
#include <grpcpp/support/status.h>
#include <stdexcept>

namespace asyncpp::grpc {
	class grpc_exception : public std::runtime_error {
		constexpr static const char* const default_strings[] = {
			"OK",		 "CANCELLED",		"UNKNOWN",			 "INVALID_ARGUMENT",   "DEADLINE_EXCEEDED",
			"NOT_FOUND", "ALREADY_EXISTS",	"PERMISSION_DENIED", "RESOURCE_EXHAUSTED", "FAILED_PRECONDITION",
			"ABORTED",	 "OUT_OF_RANGE",	"UNIMPLEMENTED",	 "INTERNAL",		   "UNAVAILABLE",
			"DATA_LOSS", "UNAUTHENTICATED",
		};

		::grpc::Status m_status;

	public:
		grpc_exception(::grpc::Status s) : std::runtime_error(s.error_message()), m_status{s} {}
		grpc_exception(::grpc::StatusCode c)
			: grpc_exception(::grpc::Status(
				  c, static_cast<size_t>(c) > static_cast<size_t>(::grpc::StatusCode::UNAUTHENTICATED) ? "UNKNOWN" : default_strings[static_cast<size_t>(c)])) {
		}
		const ::grpc::Status& get_status() const noexcept { return m_status; }
	};
} // namespace asyncpp::grpc
