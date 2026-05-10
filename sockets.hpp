#ifndef SOCKET_LIB_HPP
#define SOCKET_LIB_HPP

#include <cstddef>
#include <netinet/in.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>

template <typename Base_t, typename Inherited_t>
auto* cast_to_c_abstract_base(Inherited_t* inherited)
{
	if constexpr(std::is_const_v<Inherited_t>)
		return reinterpret_cast<const Base_t*>(inherited);
	else
		return reinterpret_cast<Base_t*>(inherited);
}

template <typename Inherited_t, typename Base_t>
auto* cast_to_c_child(Base_t* base)
{
	if constexpr(std::is_const_v<Base_t>)
		return reinterpret_cast<const Inherited_t*>(base);
	else
		return reinterpret_cast<Inherited_t*>(base);
}

class Socket
{
	public:

	explicit Socket(int handle)
	:	file_descriptor_(handle)
	{}

	~Socket()
	{
		if(file_descriptor_)
			close(*file_descriptor_);
	}

	operator int() const
	{
		return *file_descriptor_;
	}

	Socket(const Socket&)=delete;
	Socket& operator=(const Socket&)=delete;

	Socket(Socket&&)=default;
	Socket& operator=(Socket&&)=default;

	private:

	std::optional<int> file_descriptor_;
};

class Connection
{
	public:

	Connection(Socket socket, const sockaddr_in& address, int config = 0)
		: socket_(std::move(socket))
		, address_(address)
		, config_(config)
	{}

	void await_content(std::span<std::byte> buffer)
	{
		recv(socket_, buffer.data(), buffer.size(), config_);
	}

	void send_content(std::span<const std::byte> response)
	{
		send(socket_, response.data(), response.size(), config_);
	}

	private:

	Socket socket_;
	sockaddr_in address_;
	int config_;
};

class Listener
{
	public:

	Listener(int port_to_listen_over, int queue_length, int config = 0)
	:	socket_(socket(AF_INET, SOCK_STREAM, config))
	{
		const sockaddr_in address{ .sin_family=AF_INET, .sin_port=htons(port_to_listen_over), .sin_addr=INADDR_ANY };
		if(bind(socket_, cast_to_c_abstract_base<sockaddr>(&address), sizeof(address))!=0)
			throw std::runtime_error{"failed to bind socket to port"};

		listen(socket_, queue_length);
	}

	Connection await_connection()
	{
		sockaddr_storage their_address;
		socklen_t bytes_written = sizeof(their_address);
		Socket socket_for_connection{accept(socket_, cast_to_c_abstract_base<sockaddr>(&their_address), &bytes_written)};
		return Connection(std::move(socket_for_connection), *cast_to_c_child<sockaddr_in>(&their_address));
	}

	private:

	Socket socket_;
};

#endif // SOCKET_LIB_HPP_INCLUDED
