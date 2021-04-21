#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include <im_server/im_server.hpp>



int main () {
	im_server_t _server;
	_server.start (8080, "/ws");
	return 0;
}
