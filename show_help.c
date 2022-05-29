#include "ft_ping.h"
#define HELP_USAGE "ft_ping [options] <destination>"

static const char *options[][2] = {
		{"-v", "verbose output"},
		{"-h", "print help and exit"},
		{NULL, NULL}
};

void	show_help()
{
	dprintf(2, "\nUsage:\n  %s\n\n", HELP_USAGE);
	dprintf(2, "Options:\n");
	for (int i = 0; options[i][0]; i++)
		dprintf(2, "  %-20s%s\n", options[i][0], options[i][1]);
}