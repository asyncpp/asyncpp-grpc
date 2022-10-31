#pragma once
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/server_context.h>
#include <type_traits>

namespace asyncpp::grpc {
	enum class rpc_method_type { unary, client_streaming, server_streaming, bidi_streaming };

	template<typename>
	struct method_traits;

	// =========== Server side helpers ===========
	template<rpc_method_type value, typename TRequest, typename TResponse>
	struct server_stream_traits {
		static_assert(value == rpc_method_type::unary || value == rpc_method_type::client_streaming //
						  || value == rpc_method_type::server_streaming || value == rpc_method_type::bidi_streaming,
					  "Invalid rpc_method_type");
		constexpr static inline bool is_server_side = std::true_type::value;
		constexpr static inline bool is_client_streaming = (value == rpc_method_type::client_streaming || value == rpc_method_type::bidi_streaming);
		constexpr static inline bool is_server_streaming = (value == rpc_method_type::server_streaming || value == rpc_method_type::bidi_streaming);
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = value;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type =
			std::conditional_t<value == rpc_method_type::unary, ::grpc::ServerAsyncResponseWriter<TResponse>,
							   std::conditional_t<value == rpc_method_type::client_streaming, ::grpc::ServerAsyncReader<TResponse, TRequest>,
												  std::conditional_t<value == rpc_method_type::server_streaming, ::grpc::ServerAsyncWriter<TResponse>,
																	 ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>>>>;
	};

	// =========== Server side methods ============
	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)> : server_stream_traits<rpc_method_type::unary, TRequest, TResponse> {
		using service_type = TService;
		using call_ptr_type = void (service_type::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*, ::grpc::CompletionQueue*,
													 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReader<TResponse, TRequest>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)>
		: server_stream_traits<rpc_method_type::client_streaming, TRequest, TResponse> {
		using service_type = TService;
		using call_ptr_type = void (service_type::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReader<TResponse, TRequest>*, ::grpc::CompletionQueue*,
													 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncWriter<TResponse>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)>
		: server_stream_traits<rpc_method_type::server_streaming, TRequest, TResponse> {
		using service_type = TService;
		using call_ptr_type = void (service_type::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncWriter<TResponse>*, ::grpc::CompletionQueue*,
													 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)>
		: server_stream_traits<rpc_method_type::bidi_streaming, TRequest, TResponse> {
		using service_type = TService;
		using call_ptr_type = void (service_type::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>*, ::grpc::CompletionQueue*,
													 ::grpc::ServerCompletionQueue*, void*);
	};

	// =========== Client side methods ============

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncResponseReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																									 ::grpc::CompletionQueue*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::unary;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncResponseReader<response_type>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, const request_type&, ::grpc::CompletionQueue*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncResponseReaderInterface<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																											  ::grpc::CompletionQueue*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::unary;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncResponseReader<response_type>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, const request_type&, ::grpc::CompletionQueue*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncWriter<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*, ::grpc::CompletionQueue*,
																							void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::client_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncWriter<TRequest>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, response_type*, ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncWriterInterface<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*,
																									 ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::client_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncWriterInterface<TRequest>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, response_type*, ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&, ::grpc::CompletionQueue*,
																							 void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncReader<TResponse>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, const request_type&, ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderInterface<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																									  ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncReaderInterface<TResponse>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, const request_type&, ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderWriter<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*, ::grpc::CompletionQueue*,
																											 void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::bidi_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncReaderWriter<TRequest, TResponse>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, ::grpc::CompletionQueue*, void*);
	};
	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderWriterInterface<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*,
																													  ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		constexpr static inline rpc_method_type type = rpc_method_type::bidi_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using stream_type = std::unique_ptr<::grpc::ClientAsyncReaderWriterInterface<TRequest, TResponse>>;
		using call_ptr_type = stream_type (service_type::*)(::grpc::ClientContext*, ::grpc::CompletionQueue*, void*);
	};
} // namespace asyncpp::grpc
