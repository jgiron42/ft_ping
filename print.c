#include "ft_ping.h"

status		print_timestamp(t_config conf)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1)
	{
		my_perror(conf, "gettimeofday");
		return (KO);
	}
	printf("[%ld.%06ld] ", tv.tv_sec, tv.tv_usec);
	return (OK);
}

int	get_precision(long long time)
{
	return 3 - (time /1000 > 0) - (time /1000 > 10) - (time /1000 > 100);
}

status	print_icmp_message(t_packet pong)
{
	switch (pong.icmp_type) {
		case ICMP_ECHOREPLY:
			printf("Echo Reply");
			break;
		case ICMP_DEST_UNREACH:
			switch (pong.icmp_code) {
				case ICMP_NET_UNREACH:
					printf("Destination Net Unreachable");
					break;
				case ICMP_HOST_UNREACH:
					printf("Destination Host Unreachable");
					break;
				case ICMP_PROT_UNREACH:
					printf("Destination Protocol Unreachable");
					break;
				case ICMP_PORT_UNREACH:
					printf("Destination Port Unreachable");
					break;
				case ICMP_FRAG_NEEDED:
					printf("Frag needed and DF set");
					break;
				case ICMP_SR_FAILED:
					printf("Source Route Failed");
					break;
				default:
					printf("Dest Unreachable, Bad Code: %d", pong.icmp_code);
					break;
			}
			break;
		case ICMP_SOURCE_QUENCH:
			printf("Source Quench");
			break;
		case ICMP_REDIRECT:
			switch (pong.icmp_code) {
				case ICMP_REDIR_NET:
					printf("Redirect Network");
					break;
				case ICMP_REDIR_HOST:
					printf("Redirect Host");
					break;
				case ICMP_REDIR_NETTOS:
					printf("Redirect Type of Service and Network");
					break;
				case ICMP_REDIR_HOSTTOS:
					printf("Redirect Type of Service and Host");
					break;
				default:
					printf("Redirect, Bad Code: %d", pong.icmp_code);
					break;
			}
			break;
		case ICMP_ECHO:
			printf("Echo Request");
			break;
		case ICMP_TIME_EXCEEDED:
			switch(pong.icmp_code) {
				case ICMP_EXC_TTL:
					printf("Time to live exceeded");
					break;
				case ICMP_EXC_FRAGTIME:
					printf("Frag reassembly time exceeded");
					break;
				default:
					printf("Time exceeded, Bad Code: %d", pong.icmp_code);
					break;
			}
			break;
		case ICMP_PARAMETERPROB:
			printf("Parameter problem: pointer = %u", pong.info);
			break;
		case ICMP_TIMESTAMP:
			printf("Timestamp");
			break;
		case ICMP_TIMESTAMPREPLY:
			printf("Timestamp Reply");
			break;
		case ICMP_INFO_REQUEST:
			printf("Information Request");
			break;
		case ICMP_INFO_REPLY:
			printf("Information Reply");
			break;
		default:
			printf("Bad ICMP type: %d", pong.icmp_type);
	}
	return OK;
}

status	print_pong(t_config config, t_packet pong)
{
	if (config.timestamp && print_timestamp(config) != OK)
		return (FATAL);
	if (pong.icmp_type == ICMP_ECHOREPLY) {
		printf("%zu bytes from %s: icmp_seq=%d ", pong.size, pong.ipstr,pong.seq);
		printf("ttl=%d time=%.*f ms", pong.ttl, get_precision((pong.date - pong.date_request)),
			   (float) (pong.date - pong.date_request) / 1000);
		if (pong.is_dup)
			printf(" (DUP!)");
	}
	else
	{
		printf("From %s: icmp_seq=%d ", pong.ipstr,pong.seq);
		print_icmp_message(pong);
	}
	if (config.audible)
		printf("\a");
	printf("\n");
	return (OK);
}

status	print_ping(t_config conf, session ses, t_packet ping)
{
	char ipbuf[50];
	(void)ses; (void)ping; (void)conf; (void)ipbuf;
	if (!inet_ntop(ses.dst->ai_family,
				   &ping.addr.sin_addr,
				   ipbuf, 50))
	{
		my_perror(conf, "inet_ntop");
		return (FATAL);
	}
	printf("PING %s(%s) %zd data bytes\n", conf.destination, ipbuf, ping.size);
	return (OK);
}

status	print_short_stat(t_stat stats)
{

	stats.time = ft_utime() - stats.time;
	dprintf(2, "\b\b%d/%d packets, %d%% loss", stats.send, stats.received, stats.send ? 100 * (stats.send - stats.received) / stats.send : 0);
	if (stats.received > 0) {
		long long int average = stats.average / stats.received;
		long long int sdev = ft_sqrt((stats.sdev / stats.received) - (average * average));
		printf(", min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms", (float) stats.min / 1000, (float) average / 1000,
			   (float) stats.max / 1000, (float) sdev / 1000);
		printf("\n");
	}
	return (OK);
}

status	print_stat(t_config conf, t_stat stats)
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
	if (stats.errors)
		printf("+%d errors, ", stats.errors);
	printf("%d%% packet loss, time %lldms\n", stats.send ? 100 * (stats.send - stats.received) / stats.send : 0 , stats.time / 1000);
	if (stats.received > 0)
		printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms", (float)stats.min / 1000, (float)stats.average / 1000,
			   (float)stats.max / 1000, (float)stats.sdev / 1000);
	printf("\n");
	return (OK);
}