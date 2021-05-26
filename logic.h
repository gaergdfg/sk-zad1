#ifndef LOGIC_H
#define LOGIC_H

#include "parser.h"


struct response_data {
	bool head_only;
	int code;
	std::string connection;
	std::string content_length;
	std::string location;
};
using response_data_t = struct response_data;


response_data_t process_http_request(request_data_t &request_data, std::string &correlated_files_path);

#endif /* LOGIC_H */
