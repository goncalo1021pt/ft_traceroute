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

#define IP_HEADER_SIZE  20
#define UDP_HEADER_SIZE 8
/* size of the whole probe on the wire, the way real traceroute reports it */
#define PACKET_SIZE       60
#define PAYLOAD_SIZE      (PACKET_SIZE - IP_HEADER_SIZE - UDP_HEADER_SIZE)
#define MAX_QUERIES       10
#define MAX_HOPS          255
#define MAX_PORT          65535
#define EXIT_INIT_FAILURE 2 /* same code real traceroute uses for resolve/socket errors */

typedef enum e_long_options {
	OPT_HELP = 1000
} t_long_options;

typedef struct s_traceroute_stats {
	int packets_sent;
	int packets_received;
} t_traceroute_stats;

/* One probe of a hop: measured while waiting, printed afterwards. */
typedef struct s_probe {
	struct timeval sent;
	bool sent_ok;
	bool replied;
	uint16_t port;
	int icmp_type;
	int icmp_code;
	struct sockaddr_in from;
	double rtt;
} t_probe;

/* What we keep from an ICMP error that answers one of our probes. */
typedef struct s_reply {
	int type;
	int code;
	uint16_t dport;
} t_reply;

typedef struct s_options {
	bool help;
	int max_hops;
	int first_ttl;
	int nqueries;
	int waittime;
	int port;
	int squeries;
	char *interface;
	char *src_addr;
	char *tos;
	char *flowlabel;
} t_options;

typedef struct s_traceroute {
	char *ip_address;
	char *hostname;
	int sockfd;
	int send_sockfd;
	struct sockaddr_in dest_addr;
	uint16_t identifier;
	uint16_t sequence;
	int current_ttl;
	int dest_port;
	uint16_t src_port;
	t_traceroute_stats stats;
} t_traceroute;

// parser.c
void initialize_program(t_options *options, t_traceroute *traceroute);
void free_options(t_options *options);
void print_options(t_options *options);
void print_help(void);
int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options);

// socket.c
int create_icmp_socket(void);
int create_udp_socket(void);
void close_socket(int sockfd);
int resolve_address(t_traceroute *traceroute);
int set_socket_ttl(int sockfd, int ttl);
int set_socket_timeout(int sockfd, int seconds);
int bind_udp_socket(int sockfd, uint16_t *port);

// exec.c
int exec_traceroute(t_traceroute *traceroute, t_options *options);

// icmp.c
bool parse_icmp_reply(const unsigned char *buf, size_t len, const t_traceroute *traceroute,
                      t_reply *reply);
void format_marker(int type, int code, char *buf, size_t size);

// probes.c
int send_probes(t_traceroute *traceroute, const t_options *options);

#endif