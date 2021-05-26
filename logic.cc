#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

#include "err.h"
#include "logic.h"
#include "parser.h"

namespace fs = std::filesystem;


const std::regex path_regex(R"(\/(?:[a-zA-Z0-9-_.\/]+))");
const std::regex resource_regex(R"(([a-zA-Z0-9-_\/.]+)\t(\d+(?:\.\d+){3})\t(\d+))");


/*
 * Zwraca: true, gdy [path] ma poprawna strukture sciezki i nie zawiera nielegalnych znakow
 *         false, wpp
 */
bool validate_path(std::string path) {
	std::smatch match;
	if (!std::regex_match(path, match, path_regex)) {
		return false;
	}

	std::string delimiter = "/";
	std::string token;
	size_t pos = 0;
	int level = 0;

	path.erase(0, 1);
	while ((pos = path.find(delimiter)) != std::string::npos) {
		token = path.substr(0, pos);

		level += token == ".." ? -1 : 1;
		if (level < 0 || token == "") {
			return false;
		}

		path.erase(0, pos + delimiter.length());
	}

	return true;
}

/*
 * Laduje rozmiar pliku [file_path] reprezentowany jako napis do [dest].
 * Zwraca: true, gdy operacja sie powiodla
 *         false, wpp
 */
bool load_file_size(std::string &file_path, std::string &dest) {
	std::uintmax_t num;

	try {
		num = fs::file_size(file_path);
	} catch (fs::filesystem_error &ec) {
		return false;
	}
	dest = std::to_string(num);

	return true;
}

response_data_t process_http_request(request_data_t &req_data, std::string &correlated_files_path) {
	response_data_t data = {
		true,
		500,
		req_data.connection == "" ? "keep-alive" : req_data.connection,
		"0",
		""
	};

	if (req_data.error || data.content_length != "0") {
		data.code = 400;
		return data;
	}

	if (!validate_path(req_data.request_target)) {
		data.code = 404;
		return data;
	}

	if (req_data.method != "GET" && req_data.method != "HEAD") {
		data.code = 501;
		return data;
	}

	if (req_data.connection == "close") {
		data.code = 200;
		return data;
	}

	data.head_only = req_data.method == "HEAD";
	bool file_exists_locally = false;
	try {
		file_exists_locally = fs::exists((std::string)fs::current_path() + req_data.request_target);
	} catch (fs::filesystem_error &ec) {
		data.code = 404;
		data.head_only = true;
		return data;
	}
	if (file_exists_locally) {
		data.code = 200;
		std::string arg = ((std::string)fs::current_path()) + req_data.request_target;
		if (!load_file_size(arg, data.content_length)) {
			data.code = 500;
			data.head_only = true;
			return data;
		}

		return data;
	} else {
		std::ifstream correlated_files;
		correlated_files.open(correlated_files_path);
		if (!correlated_files.is_open()) {
			data.code = 500;
			data.head_only = true;
			return data;
		}

		std::string input;
		std::smatch match;
		while (getline(correlated_files, input)) {
			if (!std::regex_search(input, match, resource_regex)) {
				correlated_files.close();
				data.code = 500;
				data.head_only = true;
				return data;
			}

			if (req_data.request_target == match[1]) {
				correlated_files.close();
				data.code = 302;
				data.location = "http://" + (std::string)match[2] + ":" + (std::string)match[3] + req_data.request_target;
				return data;
			}
		}

		correlated_files.close();
		data.code = 404;
		data.head_only = true;
		return data;
	}

	return data;
}
