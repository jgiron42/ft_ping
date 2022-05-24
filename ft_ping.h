#ifndef FT_PING_FT_PING_H
#define FT_PING_FT_PING_H
#include <stdio.h>
#include <linux/icmp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "libft/libft.h"

typedef struct	s_config
{
	char 	*destination;
	bool	verbose : 1;
	bool	help	: 1;
}				t_config;

typedef enum	e_status
{
	OK,
	KO,
	FATAL,
}				t_status;

t_status	parse_config(int argc, char **argv, t_config *config);
void		reset_config(t_config *config);

#endif //FT_PING_FT_PING_H
