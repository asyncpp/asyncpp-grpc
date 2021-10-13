#pragma once
#include <exception>
#include <string>
#include <typeinfo>

namespace grpc {
	class Status;
}

namespace asyncpp::grpc::util {
	std::string demangle_typename(std::string to_demangle);
	const std::type_info& exception_type(std::exception_ptr e);
	const std::type_info& current_exception_type();
	std::string exception_message(std::exception_ptr e);
	std::string current_exception_message();

	::grpc::Status exception_to_status(std::exception_ptr e);
} // namespace asyncpp::grpc::util
