#ifndef PARSER_H
#define PARSER_H

#include <string>


struct request_data {
	bool error;
	std::string method;
	std::string request_target;
	std::string connection;
	std::string content_length;
};
using request_data_t = struct request_data;


class Parser {
public:
	Parser();
	~Parser();

	/*
	* Wczytuje dane otrzymane z gniazda, umieszczajac je w [buffered_sock_content],
	* po czym, gdy napotkamy podwojne powtorzenie CRLF, wycina najkrotszy prefiks
	* konczacy sie tymi znakami z [buffered_sock_content] i umieszcza go w [curr_request].
	* Zwraca: false, gdy klient zakonczyl polaczenie
	*         true, wpp
	*/
	bool read_http_request(int client_sock);

	request_data_t parse_http_request();

private:
	char *buffer;
	std::string curr_request;
	std::string buffered_sock_content;
};

#endif /* PARSER_H */
