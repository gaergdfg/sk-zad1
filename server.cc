#include <filesystem>
#include <string>
#include <unistd.h>

#include "err.h"
#include "tcp_socket.h"

namespace fs = std::filesystem;


#define PORT_NUM 8080


int main(int argc, char *argv[]) {
	if (argc < 3 || 4 < argc) {
		fatal("Sposob uzycia: serwer <nazwa-katalogu-z-plikami> <plik-z-serwerami-skorelowanymi> [<numer-portu-serwera>]");
	}

	std::string correlated_files_path;
	try {
		correlated_files_path = fs::canonical(argv[2]);
	} catch (fs::filesystem_error &ec) {
		syserr("canonical");
	}
	if (access(correlated_files_path.c_str(), R_OK)) {
		syserr("Blad przy odczytaniu pliku z serwerami skorelowanymi");
	}

	try {
		fs::current_path(argv[1]);
	} catch (fs::filesystem_error &ec) {
		syserr("Blad przy otworzeniu katalogu z plikami");
	}

	TCP_Socket tcp_socket(correlated_files_path);
	tcp_socket.tcp_bind(argc == 4 ? atoi(argv[3]) : PORT_NUM);
	tcp_socket.tcp_listen();

	while (true) {
		tcp_socket.tcp_accept();
		tcp_socket.proccess_requests();
		tcp_socket.tcp_close();
	}

	return 0;
}
