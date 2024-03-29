#include "ft_ping.h"

int main(int argc, char **argv)
{
	t_config	config = {.count = 0, .ipv6 = false, .interval = 1};
	int			ret = 0;
//	signal(SIGPIPE, SIG_IGN);
	if (parse_config(argc, argv, &config) != OK)
		ret = 2;
	if (!ret)
		ret = ft_ping(config);
	reset_config(&config);
	return (ret);
}