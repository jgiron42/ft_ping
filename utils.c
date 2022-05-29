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