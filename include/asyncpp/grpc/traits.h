#pragma once

namespace asyncpp::grpc {

	template<typename>
	struct method_traits;

	// =========== Server side methods ============
	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::true_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncResponseWriter<TResponse>*, ::grpc::CompletionQueue*,
												 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReader<TResponse, TRequest>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::true_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = void (TService::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReader<TResponse, TRequest>*, ::grpc::CompletionQueue*,
												 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncWriter<TResponse>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::true_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncWriter<TResponse>*, ::grpc::CompletionQueue*,
												 ::grpc::ServerCompletionQueue*, void*);
	};

	template<typename TService, typename TRequest, typename TResponse>
	struct method_traits<void (TService::*)(::grpc::ServerContext*, ::grpc::ServerAsyncReaderWriter<TResponse, TRequest>*, ::grpc::CompletionQueue*,
											::grpc::ServerCompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::true_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = void (TService::*)(::grpc::ServerContext*, TRequest*, ::grpc::ServerAsyncWriter<TResponse>*, ::grpc::CompletionQueue*,
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
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncResponseReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																										  ::grpc::CompletionQueue*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncResponseReaderInterface<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																											  ::grpc::CompletionQueue*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncResponseReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																										  ::grpc::CompletionQueue*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncWriter<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*, ::grpc::CompletionQueue*,
																							void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncWriter<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*, ::grpc::CompletionQueue*,
																								 void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncWriterInterface<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*,
																									 ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::false_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncWriterInterface<TRequest>> (TService::*)(::grpc::ClientContext*, TResponse*,
																										  ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&, ::grpc::CompletionQueue*,
																							 void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncReader<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																								  ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderInterface<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																									  ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::false_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncReaderInterface<TResponse>> (TService::*)(::grpc::ClientContext*, const TRequest&,
																										   ::grpc::CompletionQueue*, void*);
	};

	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderWriter<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*, ::grpc::CompletionQueue*,
																											 void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncReaderWriter<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*,
																												  ::grpc::CompletionQueue*, void*);
	};
	template<typename TService, typename TResponse, typename TRequest>
	struct method_traits<std::unique_ptr<::grpc::ClientAsyncReaderWriterInterface<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*,
																													  ::grpc::CompletionQueue*, void*)> {
		constexpr static inline bool is_server_side = std::false_type::value;
		constexpr static inline bool is_client_streaming = std::true_type::value;
		constexpr static inline bool is_server_streaming = std::true_type::value;
		constexpr static inline bool is_streaming = is_client_streaming || is_server_streaming;
		using service_type = TService;
		using request_type = TRequest;
		using response_type = TResponse;
		using call_ptr_type = std::unique_ptr<::grpc::ClientAsyncReaderWriterInterface<TRequest, TResponse>> (TService::*)(::grpc::ClientContext*,
																														   ::grpc::CompletionQueue*, void*);
	};
} // namespace asyncpp::grpc
