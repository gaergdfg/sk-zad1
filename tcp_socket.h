#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <netinet/in.h>
#include <string>


class TCP_Socket {
public:
	TCP_Socket(std::string &correlated_files_path);

	void tcp_bind(int port);

	void tcp_listen();

	void tcp_accept();

	void proccess_requests();

	void tcp_close();

private:
	int sock, client_sock;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t client_address_len;
	std::string correlated_files_path;
};

#endif /* TCP_SOCKET_H */