#include <format>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>
#include <filesystem>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "cache.hpp"
#include "server.hpp"

int port;

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << std::format("Usage: {} <port> <content directory>\n", argv[0]);
		exit(1);
	}

	std::stringstream port_stream(argv[1]);
	port_stream >> port;
	// TODO error handling

	std::filesystem::path srv(argv[2]);
	// TODO error handling

	// TODO networking code

	while (true) {
		// TODO main loop
		break;
	}
}
