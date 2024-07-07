// List available IP addresses.
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ifaddrs ifaddrs;

int main() {
	// addresses holds the list of available network interface addresses in the system.
	ifaddrs *addresses;

	if (getifaddrs(&addresses) == -1) {
		printf("failed to get network interface addresses.\n");
		return -1;
	}

	for (ifaddrs *addr = addresses; addr != 0; addr = addr->ifa_next) {
		int family = addr->ifa_addr->sa_family;
		int is_ip = family == AF_INET || family == AF_INET6;

		// skip non-IP network interfaces.
		if (!is_ip) continue;
		
		char *adapter_name = addr->ifa_name;
		char *addr_type = family == AF_INET ? "IPv4" : "IPv6";

		printf("%s\t", adapter_name);
		printf("%s\t", addr_type);

		char address[100];
		const int family_size = 
			family == AF_INET ? 
				sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6);

		getnameinfo(
			addr->ifa_addr, family_size,
			address, sizeof(address),
			0, 0,
			NI_NUMERICHOST
		);

		printf("\t%s\n", address);
	}

	freeifaddrs(addresses);
	return 0;
}
