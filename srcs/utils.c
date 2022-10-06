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

int 	parse_buf(unsigned char *dst, unsigned char *src, size_t dst_len)
{
	unsigned char current;
	bzero(dst, dst_len);
	size_t i = 0;
	for (; src[i] && i < 2 * dst_len; i++)
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
	return ((i + 1) / 2);
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

/**
 * ip checksum (see: https://datatracker.ietf.org/doc/html/rfc1071)
 * @param data address of the data to checksum
 * @param size size in bytes of the data
 * @return the checksum
 */
unsigned short int checksum(void *data, size_t size)
{
	int ret = 0;
	// sum
	for (size_t i = 0; i < size / 2; i++)
		ret += ((unsigned short *)data)[i];
	// adding potential extra byte
	if (size % 2)
		ret += ((unsigned char *)data)[size - 1];
	// folding
	while (ret>>16)
		ret = (ret & 0xffff) + (ret >> 16);
	// returning 1's complement of the sum
	return (~ret);
}

/**
 * datagram for echo icmp message (see: https://datatracker.ietf.org/doc/html/rfc792 https://datatracker.ietf.org/doc/html/rfc4443)
 * @param datagram
 * @return
 */
status getdatagram(t_echo_datagram *datagram, int sequence, t_config config)
{
	if (config.pattern_length)
		for (size_t i = 0; i < sizeof *datagram; i += config.pattern_length)
			ft_memcpy((char *)datagram + i, config.pattern, config.pattern_length);
	else
		bzero(datagram, sizeof *datagram);
	datagram->type = config.ipv6 ? 128 : ICMP_ECHO;
	datagram->code = 0;
	// set to 0 before the checksum computation
	datagram->checksum = 0;
	datagram->echo.identifier = getpid();
	datagram->echo.sequence = sequence;
	struct timeval tmp;
	if (gettimeofday(&tmp, NULL) == -1)
		return (FATAL);
	ft_memcpy(&datagram->echo.date_send, &tmp, sizeof (struct timeval));
	datagram->checksum = checksum((unsigned short int *)datagram, 64);
	return (OK);
}

/**
 * resolve the address of the domain name given as argument
 */
status	get_host(t_config config,struct addrinfo **res)
{
	struct addrinfo hints = {};
	hints.ai_family = config.ipv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_flags = AI_ADDRCONFIG;
//	if (is_broadcast(config.destination) && !config.broadcast)
//		dprintf(2, "warn: -b to enable broadcast\n");
	int ret = getaddrinfo(config.destination, NULL, &hints, res);
	if (ret)
	{
		dprintf(2, "%s: %s: %s\n", config.program_name, config.destination, gai_strerror(ret));
		return (FATAL);
	}
	return (OK);
}
