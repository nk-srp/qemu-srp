/*
 *
 */

#ifndef _SC_LINK_SOCKET_H
#define _SC_LINK_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


struct bus_access {
	char command;
	int32_t address;
	int32_t data;
} my_bus_access;

static int mysocket;

void cpu_socket_init();

//static inline int cpu_socket_read(uint32_t addr, int num_socket) {
//	if (num_socket == 0)
//		cpu_socket_init();
//
//	my_bus_access.command = 0x01;
//	my_bus_access.address = addr;
//
//	if (send(mysocket, &my_bus_access, sizeof(my_bus_access), 0)
//			!= sizeof(my_bus_access)) {
//		perror("Mismatch in number of sent bytes\n");
//		exit(1);
//	}
//
//	int received = recv(mysocket, &my_bus_access, sizeof(my_bus_access), 0);
//
//	printf("Read op done, received: %d, data: %d\n", received,
//			my_bus_access.data);
//
//	return my_bus_access.data;
//}
//
//static inline void cpu_socket_write(int32_t addr, int32_t data, int num_socket) {
//	if (num_socket == 0)
//		cpu_socket_init();
//
//	my_bus_access.command = 0x02;
//	my_bus_access.address = addr;
//	my_bus_access.data = data;
//
//	int bytes = send(mysocket, &my_bus_access, sizeof(my_bus_access), 0);
//	if (bytes != sizeof(my_bus_access)) {
//		perror("Mismatch in number of sent bytes\n");
//		exit(1);
//	}
//
//}

void cpu_socket_init() {
	struct sockaddr_in hostaddr;

	printf("start\n");
	if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Failed to creadte socket.\n");
		exit(1);
	}
	printf("socket open\n");

	memset(&hostaddr, 0, sizeof(hostaddr));
	hostaddr.sin_family = AF_INET;
	hostaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	hostaddr.sin_port = htons(2012);

	if (connect(mysocket, (const struct sockaddr*) &hostaddr, sizeof(hostaddr))
			< 0) {
		perror("connet");
		exit(1);
	}

}


#endif
