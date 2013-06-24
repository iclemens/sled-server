
#include "../src/server.h"
#include <stdio.h>

void *connect_handler(net_connection_t *conn)
{
	printf("Client connected...\n");
	return NULL;
}

void disconnect_handler(net_connection_t *conn, void **ctx)
{
	printf("Client disconnected...\n");
}

void read_handler(net_connection_t *conn, char *buf, int size)
{
	buf[size] = 0;
	printf("Received: %s\n", buf);
}

int main(int argc, char **argv)
{
	net_server_t *server = net_setup_server(NULL, 3375);

	net_set_connect_handler(server, connect_handler);
	net_set_disconnect_handler(server, disconnect_handler);
	net_set_read_handler(server, read_handler);

	printf("Server started...\n");

	while(true) {
	}

	net_teardown_server(&server);

	return 0;
}
