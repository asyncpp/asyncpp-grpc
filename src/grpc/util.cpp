#include <asyncpp/grpc/grpc_exception.h>
#include <asyncpp/grpc/util.h>
#include <cxxabi.h>

namespace asyncpp::grpc::util {

	std::string demangle_typename(std::string to_demangle) {
		int status = 0;
		char* buff = __cxxabiv1::__cxa_demangle(to_demangle.c_str(), nullptr, nullptr, &status);
		if (!buff) return to_demangle;
		std::string demangled = buff;
		std::free(buff);
		return demangled;
	}

	struct unknown_exception_type {};

	const std::type_info& exception_type(std::exception_ptr e) {
		if (!e) return typeid(unknown_exception_type);
		#ifdef __GLIBCXX__
			return *e.__cxa_exception_type();
		#else
		try {
			std::rethrow_exception(e);
		} catch (...) { return current_exception_type(); }
		#endif
	}

	const std::type_info& current_exception_type() {
		auto ptr = abi::__cxa_current_exception_type();
		if (!ptr) return typeid(unknown_exception_type);
		return *ptr;
	}

	std::string exception_message(std::exception_ptr e) {
		if (!e) return "unknown";
		try {
			std::rethrow_exception(e);
		} catch (const std::exception& e) { return e.what(); } catch (...) {
			return demangle_typename(current_exception_type().name());
		}
	}

	std::string current_exception_message() { return exception_message(std::current_exception()); }

	::grpc::Status exception_to_status(std::exception_ptr e) {
		try {
			std::rethrow_exception(e);
		} catch (const ::grpc::Status& s) { //
			return s;
		} catch (const grpc_exception& s) { //
			return s.get_status();
		} catch (const std::exception& s) {
#ifdef NDEBUG
			return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal error");
#else
			return ::grpc::Status(::grpc::StatusCode::INTERNAL, s.what());
#endif
		} catch (...) { return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Unknown exception"); }
	}
} // namespace asyncpp::grpc::util
