// A simple clock server for one time use only xD.
// $ curl 127.0.0.1:8080
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define ISVALIDSOCKET(s) ((s) >= 0)

typedef struct addrinfo addrinfo;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_storage sockaddr_storage;

int main() {
        printf("Configuring local address...\n");
        char *port = "8080";
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;       // listen on an IPv4 address
        hints.ai_socktype = SOCK_STREAM; // use the TCP protocol.
        hints.ai_flags = AI_PASSIVE;     // bind the wildcard address; so we can listen
                                         // on any available network interface.

        addrinfo *bind_address;
        // generate an address to be suitable for bind.
        // we could have filled in the addrinfo struct directly.
        // the pros of using the getaddrinfo() instead is that,
        // we can now easily switch between IPv4 to IPv6 just by changing
        // the ai_family flag on the hints object.
        getaddrinfo(NULL, port, &hints, &bind_address);

        printf("Creating socket...\n");
        int socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
        if (!ISVALIDSOCKET(socket_listen)) {
                fprintf(stderr, "could not create a new socket. (%d)\n", errno);
                return 1;
        }

        printf("Binding socket to local address...\n");
        if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
                fprintf(stderr, "could not bind an address to listener socket.(%d)\n", errno);
                return 1;
        }

        freeaddrinfo(bind_address);

        printf("Listening...\n");
        const int MAX_QUEUE_SIZE = 10;
        if (listen(socket_listen, MAX_QUEUE_SIZE) < 0) {
                fprintf(stderr, "could not start listening on port[%s]. (%d)\n", port, errno);
                return 1;
        }
        printf("Server started listening on port %s\n", port);

        printf("Waiting for connection...\n");
        sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);

        // accept() will block the program until a connection has been made to the
        // listening socket. When a new connection is made, accept() will create a new
        // socket for it. The original socket [socket_listen] still continues to
        // listen for new connections. We can use the newly created socket
        // [socket_client] to send and receive data over the newly established
        // connection.
        //
        // accept() also fills in address info of the connected client
        // [sockaddr_storage client_address].
        int socket_client = accept(socket_listen, (sockaddr *)&client_address, &client_len);
        if (!ISVALIDSOCKET(socket_client)) {
                fprintf(stderr, "could not accept a client request. (%d)\n", errno);
                return 1;
        }

        printf("Client is connected...\n");
        char address_buffer[100];
        getnameinfo((sockaddr *)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0,
                    NI_NUMERICHOST); // NI_NUMERICHOST -> want to see the host name as an IP address.
        printf("Client: %s\n", address_buffer);

        printf("Reading request...\n");
        char request[1024];
        // recv() blocks until it received something from the client.
        int bytes_received = recv(socket_client, request, 1024, 0);
        if (bytes_received <= 0) {
                printf("Connection has been terminated by the client.\n");
        } else {
                printf("Received %d bytes.\n", bytes_received);
        }

        printf("Sending response...\n");
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/plain\r\n\r\n"
                               "Local time is: ";
        int bytes_sent = send(socket_client, response, strlen(response), 0);
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

        time_t timer;
        time(&timer);
        char *time_msg = ctime(&timer);
        bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

        printf("Closing client connection...\n");
        close(socket_client);

        printf("Closing listening socket...\n");
        close(socket_listen);

        printf("Done.\n");
}
