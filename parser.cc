#include <regex>
#include <string>
#include <unistd.h>
#include <cstdio>

#include "err.h"
#include "parser.h"


#define BUFFER_SIZE 1 << 13
#define MAX_MSG_LEN 1 << 21


const std::string msg_end = "\r\n\r\n";

const std::regex msg_end_regex(R"(\r\n\r\n)");
const std::regex http_request_regex(R"(([a-zA-Z]+) (\/[^ ]*) HTTP\/1\.1\r\n(?:[^:\s]+: *(?:.*[^ ]) *\r\n)*\r\n)");
const std::regex request_line_regex(R"(([^:\s]+): *(.*[^ ]) *\r\n)");


std::string to_lower(const std::string &value) {
	std::string res = value;

	for (size_t i = 0; i < value.length(); i++) {
		if (value[i] >= 'A' && value[i] <= 'Z') {
			res[i] = value[i] - 'A' + 'a';
		}
	}

	return res;
}

Parser::Parser() {
	buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
	if (!buffer) {
		syserr("calloc");
	}
}

Parser::~Parser() {
	free(buffer);
}

/*
 * Wczytuje dane otrzymane z gniazda, umieszczajac je w [buffered_sock_content],
 * po czym, gdy napotkamy podwojne powtorzenie CRLF, wycina najkrotszy prefiks
 * konczacy sie tymi znakami z [buffered_sock_content] i umieszcza go w [curr_request].
 * Zwraca: false, gdy klient zakonczyl polaczenie
 *         true, wpp
 */
bool Parser::read_http_request(int client_sock) {
	if (!std::regex_search(buffered_sock_content, msg_end_regex)) {
		std::string buffer_converter;
		int msg_len;

		do {
			msg_len = read(client_sock, buffer, BUFFER_SIZE);
			if (!msg_len) {
				return false;
			}
			if (buffered_sock_content.length() + msg_len > MAX_MSG_LEN) {
				return true;
			}

			buffer_converter = std::string(buffer);
			buffered_sock_content += buffer_converter;
			memset(buffer, 0, msg_len);
		} while (!std::regex_search(buffered_sock_content, msg_end_regex));
	}

	size_t msg_end_pos = buffered_sock_content.find(msg_end);
	curr_request = buffered_sock_content.substr(0, msg_end_pos + msg_end.length());
	buffered_sock_content.erase(0, msg_end_pos + msg_end.length());

	return true;
}

request_data_t Parser::parse_http_request() {
	request_data_t data = { false, "", "", "", "" };

	if (curr_request == "") {
		data.error = true;
		return data;
	}

	std::smatch match;

	if (!std::regex_match(curr_request, match, http_request_regex)) {
		data.error = true;
		return data;
	}

	data.method = match[1];
	data.request_target = match[2];

	bool connection_flag = false, content_length_flag = false;

	while (std::regex_search(curr_request, match,request_line_regex)) {
		std::string field_name = to_lower(match[1]);
		std::string field_value = match[2];

		if (field_name == "connection") {
			if (connection_flag) {
				data.error = true;
				return data;
			} else {
				connection_flag = true;
				data.connection = field_value;
			}
		} else if (field_name == "content-length") {
			if (content_length_flag) {
				data.error = true;
				return data;
			} else {
				content_length_flag = true;
				data.content_length = field_value;
			}
		}

		curr_request = match.suffix().str();
	}

	curr_request = "";

	return data;
}
