#pragma once

namespace asyncpp::grpc {

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