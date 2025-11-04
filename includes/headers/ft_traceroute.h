#ifndef FT_PING_H
#define FT_PING_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include "../libft/libft.h"

#define PACKET_SIZE 64
#define ICMP_HEADER_SIZE 8
#define DATA_SIZE (PACKET_SIZE - ICMP_HEADER_SIZE)

// ICMP (Internet Control Message Protocol) 
typedef struct s_icmp_header
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
} t_icmp_header;

typedef struct s_traceroute_packet
{
	t_icmp_header header;
	char data[DATA_SIZE];
} t_traceroute_packet;

typedef struct s_traceroute_stats
{
	int packets_sent;
	int packets_received;
	int packets_dropped;
	double min_rtt;
	double max_rtt;
	double total_rtt;
	struct timeval start_time;
	struct timeval end_time;
	struct timeval last_send_time;
} t_traceroute_stats;

typedef struct s_options
{
	bool help;
} t_options;

typedef struct s_traceroute
{
	char *ip_address;
	char *hostname;
	int sockfd;
	struct sockaddr_in dest_addr;
	uint16_t identifier;
	uint16_t sequence;
	t_traceroute_stats stats;
} t_traceroute;

extern volatile sig_atomic_t g_ping_running;
extern volatile sig_atomic_t g_alarm_received;
extern volatile sig_atomic_t g_statistics_requested;

// parser.c
void initialize_program(t_options *options, t_traceroute *traceroute);
void print_options(t_options *options);
void print_help();
int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options);

// exec.c
int exec_traceroute(t_traceroute *traceroute, t_options *options);

#endif