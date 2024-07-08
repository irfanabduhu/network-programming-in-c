#include "init.h"
#include <stdio.h>
#include <string.h>

typedef struct addrinfo addrinfo;
typedef struct timeval timeval;

int main(int argc, char *argv[])
{
	// hostname can be an IP address or a domain name like google.com
	// port can be a number or a protocol like http.
	if (argc < 3) {
		fprintf(stderr, "usage: tcp_client hostname port\n");
		return 1;
	}

	// We don't specify the IP family here and let getaddrinfo
	// automatically configure it based on the remote server.
	printf("Configuring remote address...\n");
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM; // We want a TCP connection.

	addrinfo *peer_address;
	if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
		fprintf(stderr, "could not get address info for server: %d\n",
			errno);
		return 1;
	}

	// Let's just print the remote address to see it's working.
	// We are doing it for debugging purposes.
	// Not necessary to actually establish a connection to a TCP host.
	printf("Remote address is: ");
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
		    address_buffer, sizeof(address_buffer), service_buffer,
		    sizeof(service_buffer), NI_NUMERICHOST);
	printf("%s %s\n", address_buffer, service_buffer);

	// Now create a socket to connect to the remote address.
	printf("Creating socket...\n");
	int socket_peer = socket(peer_address->ai_family,
				 peer_address->ai_socktype,
				 peer_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_peer)) {
		fprintf(stderr, "could not create socket: %d\n", errno);
		return 1;
	}

	// connect() associates a socket with a remote address
	// and initiates the TCP connection.
	printf("Connecting...\n");
	if (connect(socket_peer, peer_address->ai_addr,
		    peer_address->ai_addrlen)) {
		fprintf(stderr, "could not connect to the remote server: %d\n",
			errno);
		return 1;
	}

	// clean up addrinfo.
	freeaddrinfo(peer_address);

	printf("Connected.\n");
	printf("To send data, insert text followed by enter.\n");

	while (1) {
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(socket_peer, &reads);
		FD_SET(fileno(stdin), &reads); // listen for terminal input

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		if (select(socket_peer + 1, &reads, 0, 0, &timeout) < 0) {
			fprintf(stderr, "select() failed: %d\n", errno);
			return 1;
		}

		// check for TCP response.
		if (FD_ISSET(socket_peer, &reads)) {
			char read[4096];
			int bytes_received =
				recv(socket_peer, read, sizeof(read), 0);
			if (bytes_received < 1) {
				printf("Connection closed by peer.\n");
				break;
			}
			// the data from recv() is not null terminated.
			// For this reas, we use the %.*s printf() format specifier,
			// which prints a string of a specified lenght.
			printf("Received (%d bytes): %.*s", bytes_received,
			       bytes_received, read);
		}

		// check for terminal input.
		if (FD_ISSET(fileno(stdin), &reads)) {
			char read[4096];
			if (!fgets(read, sizeof(read), stdin))
				break;

			printf("Sending: %s", read);
			int bytes_sent =
				send(socket_peer, read, strlen(read), 0);
			printf("Sent %d bytes.\n", bytes_sent);
		}
	}

	printf("Closing socket...\n");
	close(socket_peer);

	printf("Done.\n");
}
