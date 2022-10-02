#include "ft_ping.h"

/**
 * return the UTC time in ms by calling gettimeofday
 * @return time or -1 if an error occured (see: man 2 gettimeofday for errors description)
 */
long long int ft_utime(void)
{
	struct timeval time;

	if (gettimeofday(&time, NULL) == -1)
		return (-1);
	return (time.tv_sec * 1000000 + time.tv_usec);
}


float	ft_sqrt(float nb)
{
	float n = 1;
	float old = -1;
	if (nb <= 0)
		return (0);
	while (old != n)
	{
		old = n;
		n = (n + nb / n) / 2;
	}
	return (n);
}

int 	parse_buf(char *dst, char *src, size_t dst_len)
{
	char current;
	bzero(dst, dst_len);
	size_t i = 0;
	for (; src[i] && i <= 2 * dst_len; i++)
	{
		if (ft_isdigit(src[i]))
			current = src[i] - '0';
		else if (src[i] >= 'a' && src[i] <= 'f')
			current = src[i] - 'a' + 10;
		else if (src[i] >= 'A' && src[i] <= 'F')
			current = src[i] - 'A' + 10;
		else
			return -1;
		if (i % 2 == 0)
			dst[i / 2] = current;
		else
			dst[(i - 1) / 2] |= dst[i / 2] << 4 | current;
	}
	return (i / 2 + 1);
}

void	clear_duplicate(session *ses, int seq)
{
	seq %= (DUPLICATE_SIZE) * 8;
	ses->duplicate_table[seq / 8] &= ~(1 << seq % 8);
}

void	set_duplicate(session *ses, int seq)
{
	seq %= (DUPLICATE_SIZE) * 8;
	ses->duplicate_table[seq / 8] |= 1 << seq % 8;
}

char	check_duplicate(session ses, int seq)
{
	seq %= (DUPLICATE_SIZE) * 8;
	return ses.duplicate_table[seq / 8] & (1 << seq % 8);
}

void	my_perror(t_config config, char *prefix)
{
	dprintf(2, "%s: %s: %m\n", config.program_name, prefix);
}