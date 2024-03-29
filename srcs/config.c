#include "ft_ping.h"
#define OPTIONS "-:vhabc:Dp:qt:" // "64dfi:"

status	parse_config(int argc, char **argv, t_config *config) {
	int ret;

	config->program_name = argv[0];
	config->count = 0;
	config->ipv6 = false;
	if (!config->program_name)
		return (FATAL);
	while ((ret = ft_getopt(argc, argv, OPTIONS)) != -1) { // abcDp
		switch (ret) {
			case 1:
				if (config->destination)
					goto error;
				config->destination = ft_optarg;
				break;
			case 'a':
				config->audible = true;
				break;
			case 'f':
				config->interval = 0;
				config->flood = true;
				break;
			case 't':
				config->ttl = ft_atoi(ft_optarg);
				break;
			case 'i':
				config->interval = ft_atoi(ft_optarg);
				break;
			case 'p': {
				for (int i = 0; ft_optarg[i]; i++)
					if ((ft_optarg[i] < '0' || ft_optarg[i] > '9') && (ft_optarg[i] < 'a' || ft_optarg[i] > 'f') &&
						(ft_optarg[i] < 'A' || ft_optarg[i] > 'F')) {
						dprintf(2, "%s: patterns must be specified as hex digits: %s\n", config->program_name,
								ft_optarg + i);
						goto error;
					}
				config->pattern_length = parse_buf(config->pattern, (unsigned char *)ft_optarg, 16);
				break;
			}
			case 'q':
				config->quiet = true;
				break;
			case 'v':
				config->verbose = true;
				break;
			case 'b':
				config->broadcast = true;
				break;
			case 'd':
				config->debug = true;
				break;
			case 'D':
				config->timestamp = true;
				break;
			case '6':
				config->ipv6 = true;
				break;
			case '4':
				config->ipv6 = false;
				break;
			case 'h':
				show_help();
				return (KO);
			case 'c':
				if (!int_validator(ft_optarg, true, 0)) {
					dprintf(2, "%s: invalid argument: '%s'\n", config->program_name, ft_optarg);
					return KO;
				}
				if (!range_validator(ft_optarg, "0", "9223372036854775808")) {
					dprintf(2, "%s: invalid argument: '%s': out of range: 1 <= value <= 9223372036854775808\n",
							config->program_name, ft_optarg);
					return KO;
				}
				config->count = ft_atoi(ft_optarg); // TODO: atol
				break;
			case ':':
				dprintf(2, "%s: missing argument for option -- '%c'\n", config->program_name, ft_optopt);
				goto error;
			default:
			case '?':
				dprintf(2, "%s: invalid option -- '%c'\n", config->program_name, ft_optopt);
				goto error;
		}
	}
	config->is_privileged = !getuid();
	if (config->pattern_length)
	{
		printf("PATTERN: 0x");
		for (int i = 0; i < config->pattern_length; i++)
		{
			unsigned int tmp = (unsigned char)config->pattern[i];
			printf("%.2x", tmp);
		}
		printf("\n");
	}
	if (config->destination)
		return (OK);
error:
	show_help();
	return (KO);

}

void	reset_config(t_config *config)
{
	*config = (t_config){};
}