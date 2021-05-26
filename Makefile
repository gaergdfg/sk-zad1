CC = g++
CFLAGS = -O2 -std=c++17 -lstdc++fs -Wextra -Wall
EXEC_NAME = serwer

server: server.o parser.o tcp_socket.o logic.o err.o
	$(CC) server.o parser.o tcp_socket.o logic.o err.o -o $(EXEC_NAME) $(CFLAGS)

server.o: server.cc tcp_socket.h err.h
	$(CC) -c server.cc $(CFLAGS)

tcp_socket.o: tcp_socket.cc parser.h logic.h err.h
	$(CC) -c tcp_socket.cc $(CFLAGS)

parser.o: parser.cc parser.h err.h
	$(CC) -c parser.cc $(CFLAGS)

logic.o: logic.cc logic.h parser.h err.h
	$(CC) -c logic.cc $(CFLAGS)

err.o: err.h

clean:
	rm *.o $(EXEC_NAME)
