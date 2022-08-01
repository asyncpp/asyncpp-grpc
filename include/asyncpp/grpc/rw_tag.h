#pragma once

namespace asyncpp::grpc {

	struct rpc_info_tag {};
	constexpr rpc_info_tag rpc_info() noexcept { return {}; }

    struct send_initial_metadata_tag {};
    constexpr send_initial_metadata_tag send_initial_metadata() noexcept { return {}; }

	template<typename T>
	struct write_tag {
		const T* const m_msg;
	};

	template<typename T>
	struct read_tag {
		T* const m_msg;
	};

	template<typename TMessage>
	read_tag<TMessage> read(TMessage& msg) {
		return read_tag<TMessage>{&msg};
	}

	template<typename TMessage>
	read_tag<TMessage> read(TMessage* msg) {
		return read_tag<TMessage>{msg};
	}

	template<typename TMessage>
	write_tag<TMessage> write(const TMessage& msg) {
		return write_tag<TMessage>{&msg};
	}

	template<typename TMessage>
	write_tag<TMessage> write(const TMessage* msg) {
		return write_tag<TMessage>{msg};
	}
} // namespace asyncpp::grpc