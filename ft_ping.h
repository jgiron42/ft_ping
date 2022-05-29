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
	bool	verbose 	: 1;
	bool	help		: 1;
	bool	quiet		: 1;
	bool	audible		: 1;
	bool	debug		: 1;
	bool	timestamp	: 1;
	bool	broadcast	: 1;
	bool	flood		: 1;
	char	*error;
	int		ttl;
	int		count;
}				t_config;

typedef struct t_packet
{
	int					seq;
	long long int		date;
	ssize_t 			size;
	int					ttl;
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
	int	nlost;
	long long time;
	long long average;
	long long sdev;
	long long min;
	long long max;
}							t_stat;

typedef	struct session
{
	int					sock;
	int					family;
	int					proto;
	int					type;
	struct	addrinfo	*dst;
	t_stat				stats;
}				session;

typedef enum	e_status
{
	OK,
	KO,
	FATAL,
}				t_status;

typedef	struct				s_echo_datagram
{
	unsigned char		type;
	unsigned char		code;
	unsigned short int	checksum;
	unsigned short int	identifier;
	unsigned short int	sequence;
	char		data[48];
} __attribute__((packed))	t_echo_datagram;

t_status	parse_config(int argc, char **argv, t_config *config);
void		reset_config(t_config *config);
void		show_help();
t_status	ft_ping(t_config config);

t_status		print_timestamp();
int	get_precision(long long time);
t_status	print_pong(t_config config, t_packet pong, t_packet ping);
t_status	print_ping(t_config conf, session ses, t_packet ping);
t_status	print_stat(t_config conf, t_stat stats);
float	ft_sqrt(float nb);
long long int ft_utime(void);


bool int_validator(char *str, bool is_signed, char terminator);
bool		range_validator(char *str, char *lower_bound, char *upper_bound);
bool		ipv4_validator(char *str);
bool	is_broadcast(char *ip);

#endif //FT_PING_FT_PING_H
