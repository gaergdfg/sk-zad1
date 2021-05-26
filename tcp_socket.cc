#include <ext/stdio_filebuf.h>
#include <filesystem>
#include <unistd.h>

#include "err.h"
#include "logic.h"
#include "parser.h"
#include "tcp_socket.h"

namespace fs = std::filesystem;


#define QUEUE_LEN 5
#define BUFFER_SIZE 1 << 13


/*
 * Wysyla wiadomosc na podstawie [res_data]. Jesli w zapytaniu metoda bylo GET,
 * przesyla rowniez plik [requested_file].
 * Zwraca: true, gdy serwer potrzymuje polaczenie
 *         false, wpp
 */
bool send_response(int client_sock, response_data_t &res_data, std::string &requested_file) {
	std::string response = "HTTP/1.1 " + std::to_string(res_data.code) + " ";

	switch (res_data.code) {
		case 200:
			response += "OK";
			break;
		case 302:
			response += "Found";
			break;
		case 400:
			response += "Bad Request";
			break;
		case 404:
			response += "Not Found";
			break;
		case 500:
			response += "Internal Server Error";
			break;
		case 501:
			response += "Not Implemented";
			break;
		default:
			break;
	}

	response += "\r\n";

	response += "Connection: ";
	response += res_data.code == 400 || res_data.code == 501 ? "close" : res_data.connection;
	response += "\r\n";

	if (res_data.code == 200 || res_data.code == 302) {
		response += "Content-Type: application/octet-stream\r\n";
	}

	response += "Content-Length: " + res_data.content_length + "\r\n";

	if (res_data.code == 302) {
		response += "Location: " + res_data.location + "\r\n";
	}

	response += "\r\n";

	std::ifstream file;
	if (res_data.code == 200) {
		std::string path;
		try {
			path = fs::current_path();
		} catch (fs::filesystem_error &ec) {
			res_data.code = 500;
			res_data.head_only = true;
			res_data.content_length = "0";
			return send_response(client_sock, res_data, requested_file);
		}
		path += requested_file;
		file.open(path.c_str(), std::ifstream::binary);
		if (!file.is_open()) {
			res_data.code = 404;
			res_data.head_only = true;
			res_data.content_length = "0";
			return send_response(client_sock, res_data, requested_file);
		}
	}

	while (response.length()) {
		ssize_t sent = write(client_sock, response.c_str(), response.length());
		if (sent == -1) {
			if (res_data.head_only) {
				file.close();
			}
			return false;
		}

		response.erase(0, sent);
	}

	if (res_data.head_only) {
		return res_data.connection != "close";
	}

	char *buffer = (char *)calloc(BUFFER_SIZE, 1);
	if (!buffer) {
		syserr("calloc");
	}
	int sent_total, read;
	while (true) {
		try {
			file.read(buffer, BUFFER_SIZE);
		} catch (std::iostream::failure &ec) {
			res_data.code = 500;
			res_data.head_only = true;
			res_data.content_length = "0";
			res_data.connection = "close";
			return send_response(client_sock, res_data, requested_file);
		}
		sent_total = 0, read = file.gcount();
		if (!read) {
			break;
		}

		while (sent_total < read) {
			int sent = send(client_sock, buffer + sent_total, read - sent_total, MSG_NOSIGNAL);
			if (sent == -1) {
				return false;
			}

			sent_total += sent;
		}
	}

	free(buffer);
	file.close();

	return res_data.connection != "close";
}

TCP_Socket::TCP_Socket(std::string &_correlated_files_path) {
	correlated_files_path = _correlated_files_path;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		syserr("socket");
}

void TCP_Socket::tcp_bind(int port) {
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
		syserr("bind");
}

void TCP_Socket::tcp_listen() {
	if (listen(sock, QUEUE_LEN) < 0)
		syserr("listen");
}

void TCP_Socket::tcp_accept() {
	client_address_len = sizeof(client_address);
	if ((client_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len)) < 0)
		syserr("accept");
}

void TCP_Socket::proccess_requests() {
	Parser parser;
	request_data_t req_data;
	response_data_t res_data;

	do {
		if (!parser.read_http_request(client_sock)) {
			return;
		}

		req_data = parser.parse_http_request();
		res_data = process_http_request(req_data, correlated_files_path);
	} while (send_response(client_sock, res_data, req_data.request_target));
}

void TCP_Socket::tcp_close() {
	if (close(client_sock) < 0)
		syserr("close");
}
