/*
 * Implements a simple single-threaded TCP/IP server.
 */

#include "server_internal.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#ifndef WIN32
#include <unistd.h>
#endif

// Include networking
#ifdef WIN32
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
#define close(x) closesocket(x)
#define read(s, buf, len) recv(s, buf, len, NULL)
#endif

#define BUFFER_SIZE 1023

/////////////////////
//  Event handlers //
/////////////////////


/**
 * Handle client event
 */
void net_on_client_event(struct bufferevent *bev, short events, void *server_v)
{
  assert(bev && server_v);
  net_server_t *server = (net_server_t *) server_v;

  if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
    fprintf(stderr, "BufferEvent raised on error.\n");
    evutil_socket_t fd = bufferevent_getfd(bev);
    net_disconnect(server->connection_data[fd]);
  }
}


/**
 * Handles a read event.
 */
void net_on_read(evutil_socket_t fd, short events, void *server_v)
{
  assert(server_v);
	net_server_t *server = (net_server_t *) server_v;

	// Add one to allow for (possible) zero-termination.
	char buffer[BUFFER_SIZE + 1];
	size_t nread = read(fd, buffer, BUFFER_SIZE);

	if(nread == -1) {
		perror("read()");
		net_disconnect(server->connection_data[fd]);
		return;
	}

	// Read did not succeed or connection was terminated
	if(nread == 0) {
		net_disconnect(server->connection_data[fd]);
		return;
	}

	// Add zero-terminator in case we decide the data is to be printed.
	buffer[nread] = '\0';

	if(server->read_handler) {
		net_connection_t *conn = server->connection_data[fd];
		server->read_handler(conn, buffer, nread);
	}

	return;
}


/**
 * Called when a new connection is pending. Accepts the connection and starts
 * listening to incoming data.
 */
void net_on_new_connection(evutil_socket_t fd, short events, void *server_v)
{
	net_server_t *server = (net_server_t *) server_v;

	if(!server)
		return;

	int client = accept_client(server->sock);

  /* Create read event */
  event *read_event = event_new(server->ev_base, client, EV_READ | EV_ET | EV_PERSIST, net_on_read, (void *) server);
  if(read_event == NULL) {
    perror("event_new()");
    close(client);
    return;
  }

  int result = event_add(read_event, NULL);
  if(result == -1) {
    perror("event_add()");
    event_free(read_event);
    close(client);
    return;
  }

  /* Create write buffer */
  bufferevent *buffer_event = bufferevent_socket_new(server->ev_base, client, 0);
  if(buffer_event == NULL) {
    fprintf(stderr, "bufferevent_socket_new() failed.\n");
    return;
  }

  net_connection_t *conn = new net_connection_t();
	conn->fd = client;
	conn->server = server;
  conn->read_event = read_event;
  conn->buffer_event = buffer_event;
	conn->local = NULL;

	if(server->connect_handler)
		conn->local = server->connect_handler(conn);

	server->connection_data[client] = conn;

  bufferevent_setcb(conn->buffer_event, NULL, NULL, net_on_client_event, (void *) server);
  bufferevent_enable(conn->buffer_event, EV_WRITE);
}


////////////////////////
//  Server functions  //
////////////////////////


/**
 * Initializes the TCP server.
 *
 * @param ev_base
 * @param context
 * @param port
 *
 * @return Instance of network server, or NULL on failure.
 */
net_server_t *net_setup_server(event_base *ev_base, void *context, int port)
{
	 net_server_t *server = NULL;

	// Initialize windows sockets
	#ifdef WIN32
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		perror("WSAStartup()");
		return NULL;
	}
	#endif

	try {
		server = new net_server_t();
	} catch(std::bad_alloc e) {
		return NULL;
	}

	server->context = context;
	server->ev_base = ev_base;

	// Create server socket
	server->sock = setup_server_socket(port);

	if(server->sock == -1) {
		delete server;
		return NULL;
	}

	// Add server socket to libevent
	event *event = event_new(server->ev_base, server->sock, EV_READ | EV_ET | EV_PERSIST, net_on_new_connection, (void *) server);
	int result = event_add(event, NULL);

	if(result == -1) {
		perror("event_add()");
		event_free(event);
		delete server;
		return NULL;
	}

	// Set all handler to NULL
	server->connect_handler = NULL;
	server->disconnect_handler = NULL;
	server->read_handler = NULL;

	return server;
}


/**
 * Shuts down the server.
 *
 * @param s  Server to shutdown.
 *
 * @return 0 on success, -1 on failure.
 */
int net_teardown_server(net_server_t **s)
{
	if(!s) 
		return -1;

	net_server_t *server = *s;

	if(!server)
		return -1;

	while(!server->connection_data.empty()) {
		std::map<int, net_connection_t *>::iterator it = server->connection_data.begin();
		printf("Closing (%d)\n", it->first);
		net_disconnect(it->second);
	}

	server->connection_data.clear();

	// Close socket and libevent
	event_base_free(server->ev_base);
	server->ev_base = NULL;
	close(server->sock);

	// Free memory
	delete server;
	*s = NULL;

	return 0;
}


void net_set_connect_handler(net_server_t *server, connect_handler_t handler)
{
	if(!server)
		return;
	server->connect_handler = handler;
}


void net_set_disconnect_handler(net_server_t *server, disconnect_handler_t handler)
{
	if(!server)
		return;
	server->disconnect_handler = handler;
}


void net_set_read_handler(net_server_t *server, read_handler_t handler) 
{
	if(!server)
		return;
	server->read_handler = handler;
}


////////////////////////////
//  Connection functions  //
////////////////////////////


/**
 * Sends data to the connection specified
 */
int net_send(net_connection_t *conn, char *buf, size_t size)
{
  if(conn == NULL)
    return -1;
  bufferevent_write(conn->buffer_event, buf, size);
	return 0;
}


/**
 * Terminate a connection.
 *
 * @param conn  Connection to terminate.
 *
 * @return Always 0.
 */
int net_disconnect(net_connection_t *conn)
{
	if(!conn)
		return 0;

	net_server_t *server = conn->server;

	if(server->disconnect_handler)
		server->disconnect_handler(conn, &(conn->local));
	server->connection_data.erase(conn->fd);

  if(conn->read_event) {
    event_del(conn->read_event);
    event_free(conn->read_event);
    conn->read_event = NULL;
  }

  if(conn->buffer_event) {
    bufferevent_free(conn->buffer_event);
    conn->buffer_event = NULL;
  }

	close(conn->fd);
	delete conn;

	return 0;
}


/**
 * Returns global (server-wide) context for a given connection.
 */
void *net_get_global_data(net_connection_t *conn)
{
	return conn->server->context;
}


/**
 * Returns local (connection-only) context.
 */
void *net_get_local_data(net_connection_t *conn)
{
	return conn->local;
}


/////////////////////////
//  Utility functions  //
/////////////////////////


/**
 * Creates a new server socket.
 *
 * @param port  Port the server will listen on
 * @return  File descriptor
 */
int setup_server_socket(int port)
{
	int sock;
	int optval;
	sockaddr_in addr;

	// Open IPv4 TCP socket
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == -1) {
		perror("socket()");
		return -1;
	}

	#ifndef WIN32
	// Disable blocking
	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	#endif

	// Disable nagling and allow reuse of address
	optval = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(optval));
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval));

	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sock, (sockaddr *) &addr, sizeof(sockaddr_in)) == -1) {
		perror("bind()");
		close(sock);
		return -1;
	}

	if(listen(sock, 0) == -1) {
		perror("listen()");
		close(sock);
		return -1;
	}

	return sock;
}


/**
 * Accepts a pending client.
 *
 * @param sock  File descriptor of server socket.
 *
 * @return File descriptor of accepted client, or -1 on failure.
 */
int accept_client(int sock)
{
	int client;
	int optval;
	sockaddr_in addr;
	socklen_t addr_size;

	addr_size = sizeof(sockaddr_in);

	// Accept connection
	client = accept(sock, (struct sockaddr *) &addr, &addr_size);

	if(client == -1) {
		perror("accept()");
		return -1;
	}

	// Disable blocking
	#ifndef WIN32
	int flags = fcntl(client, F_GETFL, 0);
	fcntl(client, F_SETFL, flags | O_NONBLOCK);
	#endif

	// Disable nagling
	optval = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(optval));

	return client;
}

