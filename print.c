#include "ft_ping.h"

t_status		print_timestamp()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1)
	{
		perror("gettimeofday");
		return (KO);
	}
	printf("[%ld.%06ld] ", tv.tv_sec, tv.tv_usec);
	return (OK);
}

int	get_precision(long long time)
{
	return 3 - (time /1000 > 0) - (time /1000 > 10) - (time /1000 > 100);
}

t_status	print_pong(t_config config, t_packet pong, t_packet ping)
{
	if (config.timestamp && print_timestamp() != OK)
		return (FATAL);
	printf("%zu bytes from %s: icmp_seq=%d ttl=%d time=%.*f ms%s\n", pong.size, pong.ipstr,
		   pong.seq, pong.ttl, get_precision((pong.date - ping.date)), (float) (pong.date - ping.date) / 1000
			, config.audible ? "\a" : "");
	return (OK);
}

t_status	print_ping(t_config conf, session ses, t_packet ping)
{
	char ipbuf[50];
	(void)ses; (void)ping; (void)conf; (void)ipbuf;
//	if (!inet_ntop(ses.dst->ai_family, &ping.addr.sin_addr, ipbuf, 50))
//	{
//		perror("inet_ntop");
//		return (FATAL);
//	}
//	printf("PING %s(%s) %zd data bytes\n", conf.destination, ipbuf, ping.size);
	return (OK);
}

t_status	print_stat(t_config conf, t_stat stats)
{
	if (stats.received)
	{
		stats.average /= stats.received;
		stats.sdev = ft_sqrt((stats.sdev / stats.received) - (stats.average * stats.average));
	}
	stats.time = ft_utime() - stats.time;
	printf("\n--- %s ping statistics ---\n", conf.destination);
	printf("%d packets transmitted, %d received, ", stats.send, stats.received);
	if (stats.duplicates)
		printf("+%d duplicates, ", stats.duplicates);
	printf("%d%% packet loss, time %lldms\n", stats.send ? 100 * (stats.send - stats.received) / stats.send : 0 , stats.time / 1000);
	if (stats.received > 0)
		printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", (float)stats.min / 1000, (float)stats.average / 1000,
			   (float)stats.max / 1000, (float)stats.sdev / 1000);
	return (OK);
}