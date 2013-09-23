/**
 * Echoes data right back at client.
 */

#include <event2/event.h>
#include "../src/server.h"
#include <stdio.h>

void *on_connect(net_connection_t *conn)
{
	printf("Client connected...\n");
	return NULL;
}

void on_disconnect(net_connection_t *conn, void **ctx)
{
	printf("Client disconnected...\n");
}

void on_read(net_connection_t *conn, char *buf, int size)
{
	net_send(conn, buf, size, 0);
}

int main(int argc, char **argv)
{
	#ifdef WIN32
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		perror("WSAStartup()");
		return NULL;
	}  
	#endif

	event_base *event_base = event_base_new();
	net_server_t *server = net_setup_server(event_base, NULL, 3375);

	if(!server) {
		fprintf(stderr, "Oops\n");
		while(true) { }
	}

	net_set_connect_handler(server, on_connect);
	net_set_disconnect_handler(server, on_disconnect);
	net_set_read_handler(server, on_read);

	printf("Server started...\n");
	event_base_loop(event_base, 0);

	net_teardown_server(&server);

	return 0;
}
