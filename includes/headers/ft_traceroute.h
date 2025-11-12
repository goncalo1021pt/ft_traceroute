#ifndef FT_TRACEROUTE_H
#define FT_TRACEROUTE_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "../libft/libft.h"

#define PACKET_SIZE 64
#define ICMP_HEADER_SIZE 8
#define DATA_SIZE (PACKET_SIZE - ICMP_HEADER_SIZE)

typedef struct s_traceroute_stats
{
	int packets_sent;
	int packets_received;
	struct timeval last_send_time;
} t_traceroute_stats;

typedef struct s_options
{
	bool help;
	int max_hops;
	int first_ttl;
	int nqueries;
	int waittime;
	int port;
} t_options;

typedef struct s_traceroute
{
	char *ip_address;
	char *hostname;
	int sockfd;
	int send_sockfd;
	struct sockaddr_in dest_addr;
	uint16_t identifier;
	uint16_t sequence;
	int current_ttl;
	int dest_port;
	t_traceroute_stats stats;
} t_traceroute;

// parser.c
void initialize_program(t_options *options, t_traceroute *traceroute);
void print_options(t_options *options);
void print_help();
int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options);

// socket.c
int create_icmp_socket(void);
int create_udp_socket(void);
void close_socket(int sockfd);
int resolve_address(t_traceroute *traceroute);
int set_socket_ttl(int sockfd, int ttl);
int set_socket_timeout(int sockfd, int seconds);

// exec.c
int exec_traceroute(t_traceroute *traceroute, t_options *options);

// probes.c
int send_probes(t_traceroute *traceroute, t_options *options);

#endif