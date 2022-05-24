#include "ft_ping.h"

t_status	parse_config(int argc, char **argv, t_config *config)
{
	int	ret;

	while ((ret = ft_getopt(argc, argv, "-vh")) != -1)
	{
		switch (ret)
		{
			case 1:
				if (!config->destination)
					config->destination = ft_strdup(ft_optarg);
				else
					config->help = true;
				break;
			case 'v':
				config->verbose = true;
				break;
			case 'h':
				config->help = true;
				break;
			case '?':
			case ':':
				return (FATAL);
		}
	}
	return (OK);
}

void	reset_config(t_config *config)
{
	free(config->destination);
	*config = (t_config){};
}