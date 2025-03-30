#include <format>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <filesystem>

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "utils.hpp"
#include "cache.hpp"
#include "server.hpp"

#define MAX_LISTEN_BACKLOG 4096

in_port_t port;

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << std::format("Usage: {} <port> <content directory>\n", argv[0]);
		exit(1);
	}

	std::stringstream port_stream(argv[1]);
	port_stream >> port;
  if (port_stream.bad() || port_stream.fail()) {
    std::cerr << std::format("Port parsing failed.\n");
    exit(1);
  }

	fs::path srv(argv[2]);
	fs::directory_entry srv_ent(srv);
	if (!srv_ent.is_directory()) {
		std::cerr << "Data directory path is not a directory.\n";
		exit(1);
	}

	int epollfd = epoll_create1(0);
	if (epollfd < 0) {
		std::cerr << std::format("epoll fd construction failed: {}\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr{ .sin_family = AF_INET,
		.sin_port = port, .sin_addr = {.s_addr = INADDR_ANY}};
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << std::format("Socket construction failed: {}\n", strerror(errno));
		exit(1);
	}
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		std::cerr << std::format("Socket bind failed: {}\n", strerror(errno));
		close(sockfd);
		exit(1);
	}
	if (listen(sockfd, MAX_LISTEN_BACKLOG) < 0) {
		std::cerr << std::format("Socket listen failed: {}\n", strerror(errno));
		close(sockfd);
		exit(1);
	}

	while (true) {
		// TODO main loop
		break;
	}
}
