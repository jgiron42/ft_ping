#ifndef FT_PING_FT_PING_H
#define FT_PING_FT_PING_H
#include <stdio.h>
#include <linux/icmp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include "libft/libft.h"
#include <sys/time.h>
#include <locale.h>
#include <netinet/ip.h>
#include <math.h>
#define NAME ft_ping


typedef struct	s_config
{
	bool	ipv6;
	char	*program_name;
	char 	*destination;
	bool	is_privileged: 1;
	bool	verbose 	: 1;
	bool	help		: 1;
	bool	quiet		: 1;
	bool	audible		: 1;
	bool	debug		: 1;
	bool	timestamp	: 1;
	bool	broadcast	: 1;
	bool	flood		: 1;
	int		interval;
	int		pattern_length;
	char	pattern[16];
	char	*error;
	int		ttl;
	int		count;
}				t_config;

typedef struct t_packet
{
	bool				is_dup;
	int					icmp_type;
	int					icmp_code;
	int					seq;
	long long int		date;
	long long int		date_request;
	ssize_t 			size;
	int					ttl;
	int					info;
	char				ipstr[50];
	struct sockaddr_in	addr;
	bool				replied;
}				t_packet;
typedef struct	s_ping
{
	int					seq;
	int					sock;
	struct	addrinfo	*dst;
	long long int		ping_date;
	long long int		pong_date;
	ssize_t				ping_size;
	ssize_t				pong_size;
	int					pong_ttl;
	char				ipstr[50];
}				t_ping;

typedef struct				s_stat
{
	int	send;
	int	received;
	int	duplicates;
	int	errors;
	int	nlost;
	long long time;
	long long average;
	long long sdev;
	long long min;
	long long max;
}							t_stat;

#define DUPLICATE_SIZE 0x100000

typedef	struct session
{
	int					sock;
	int					family;
	int					proto;
	int					type;
	struct	addrinfo	*dst;
	t_stat				stats;
	char 				duplicate_table[DUPLICATE_SIZE];
}				session;


typedef	struct				s_icmp_datagram
{
	unsigned char		type;
	unsigned char		code;
	unsigned short int	checksum;
	union {
		struct {
			unsigned short int identifier;
			unsigned short int sequence;
			long long date_send;
		} echo;
		struct {
			int unused;
			struct iphdr ip;
			unsigned char original[64];
		} ttl_exceeded; // 11
		struct {
			unsigned char pointer;
			unsigned char unused[3];
			struct iphdr ip;
			unsigned char original[64];
		} parameter_problem;
		char data[48];
	};
} __attribute__((packed))	t_echo_datagram;

status	parse_config(int argc, char **argv, t_config *config);
void		reset_config(t_config *config);
void		show_help();
status	ft_ping(t_config config);
void	my_perror(t_config config, char *prefix);

status		print_timestamp();
int	get_precision(long long time);
status	print_pong(t_config config, t_packet pong);
status	print_ping(t_config conf, session ses, t_packet ping);
status	print_short_stat(t_stat stats);
status	print_stat(t_config conf, t_stat stats);
float	ft_sqrt(float nb);
long long int ft_utime(void);

char	check_duplicate(session ses, int seq);
void	set_duplicate(session *ses, int seq);
void	clear_duplicate(session *ses, int seq);

int 	parse_buf(char *dst, char *src, size_t dst_len);
bool int_validator(char *str, bool is_signed, char terminator);
bool		range_validator(char *str, char *lower_bound, char *upper_bound);
bool		ipv4_validator(char *str);
bool	is_broadcast(char *ip);

#endif //FT_PING_FT_PING_H
