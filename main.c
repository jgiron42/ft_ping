#include "ft_ping.h"

int main(int argc, char **argv)
{
	t_config	config = {};

	parse_config(argc, argv, &config);
	reset_config(&config);
}