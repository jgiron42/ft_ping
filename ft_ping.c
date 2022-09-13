#include "ft_ping.h"

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
t_status getdatagram(t_echo_datagram *datagram, int sequence, t_config config)
{
	if (config.pattern_length)
		for (size_t i = 0; i < sizeof *datagram; i += config.pattern_length)
			memcpy((char *)datagram + i, config.pattern, config.pattern_length);
	else
		bzero(datagram, sizeof *datagram);
	datagram->type = config.ipv6 ? 128 : ICMP_ECHO;
	datagram->code = 0;
	// set to 0 before the checksum computation
	datagram->checksum = 0;
	datagram->echo.identifier = getpid();
	datagram->echo.sequence = htons(sequence);
	datagram->echo.date_send = ft_utime();
	datagram->checksum = checksum((unsigned short int *)datagram, sizeof(t_echo_datagram));
	return (OK);
}

t_status	get_host(t_config config,struct addrinfo **res)
{
	struct addrinfo hints = {};
	hints.ai_family = config.ipv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_flags = AI_ADDRCONFIG;
	if (is_broadcast(config.destination) && !config.broadcast)
		dprintf(2, "warn: -b to enable broadcast\n");
	int ret = getaddrinfo(config.destination, NULL, &hints, res);
	if (ret)
	{
	//		dprintf(2, "%s: getaddrinfo: %s\n", config.program_name, gai_strerror(ret));
		dprintf(2, "%s: getaddrinfo: %s\n", config.program_name, "no detail because gai_strerror is not authorized HAHAHAHA!!!");
		return (FATAL);
	}
	return (OK);
}

t_status	send_ping(t_config config, session *ses,t_packet *ping)
{
	static	int n = 0;
	t_echo_datagram datagram;

	*ping = (t_packet){};
	ping->seq = ++n;
	getdatagram(&datagram, ping->seq, config);
	if ((ping->size = sendto(ses->sock, &datagram, sizeof(datagram), 0, ses->dst->ai_addr, ses->dst->ai_addrlen)) == -1)
	{
		my_perror(config, "sendto");
		return (FATAL);
	}
	ping->date = ft_utime();
	clear_duplicate(ses, ping->seq);
	if (ping->date == -1)
	{
		my_perror(config,"gettimeofday");
		return (FATAL);
	}
	ses->stats.send++;
	return (OK);
}

enum {
	WAITING = 1,
	READY = 2,
	ABORTED = 4,
	PRINT_CURRENT = 8,
} status;

t_status	receive_pong(t_config conf, session ses, t_packet *pong)
{
	struct {
		struct ip iphdr;
		t_echo_datagram data;
	} ipframe = {};
	t_echo_datagram *reply = &ipframe.data;
	t_echo_datagram *nested = (t_echo_datagram *)&(reply->ttl_exceeded.original);
	char	ctrl_data[CMSG_SPACE(sizeof(int))];
	struct	iovec iov;
	if (ses.type == SOCK_RAW)
		iov = (struct iovec){.iov_base = &ipframe, .iov_len = sizeof ipframe};
	else
		iov = (struct iovec){.iov_base = &ipframe.data, .iov_len = sizeof ipframe.data};
	struct	msghdr msg = {
			.msg_name = &pong->addr,
			.msg_namelen = sizeof pong->addr,
			.msg_iov = &iov, .msg_iovlen = 1,
			.msg_control = ctrl_data,
			.msg_controllen = sizeof ctrl_data
	};

	ft_memset(ctrl_data, 0, sizeof ctrl_data);
	*pong = (t_packet){};
	pong->size = recvmsg(ses.sock, &msg, MSG_DONTWAIT);
	if (pong->size == -1 && errno == EAGAIN)
		return (KO);
	if (pong->size == -1)
	{
		my_perror(conf, "recvmsg");
		return (FATAL);
	}
	pong->ttl = ipframe.iphdr.ip_ttl;
	if (!inet_ntop(conf.ipv6 ? AF_INET6 : AF_INET, &ipframe.iphdr.ip_src, pong->ipstr, sizeof pong->ipstr))
	{
		my_perror(conf, "inet_ntop");
		return (FATAL);
	}

	// ICMP layer
	pong->icmp_code = reply->code;
	pong->icmp_type = reply->type;

	// check if the packet is ours:
	switch (reply->type)
	{
		case ICMP_ECHOREPLY:
			if (reply->echo.identifier != (unsigned short)getpid()) // not ours
				return KO;
			break;
		case ICMP_PARAMETERPROB:
			pong->info = reply->parameter_problem.pointer;
		case ICMP_DEST_UNREACH:
		case ICMP_SOURCE_QUENCH:
		case ICMP_TIME_EXCEEDED:
			if (nested->echo.identifier != (unsigned short)getpid()) // not ours
				return KO;
			break;
		default:
			return KO;
	}

	// check checksum:
	unsigned short int tmp = reply->checksum;
	reply->checksum = 0;
	if (tmp != checksum(reply, sizeof *reply))
		dprintf(2, "invalid checksum\n");

	// icmp message layer;
	switch(reply->type)
	{
		case ICMP_ECHOREPLY:
			pong->seq = ntohs(reply->echo.sequence);
			pong->date_request = reply->echo.date_send;
			pong->date = ft_utime();
			if (pong->date == -1)
			{
				my_perror(conf, "gettimeofday");
				return (FATAL);
			}
			break;
		case ICMP_DEST_UNREACH:
		case ICMP_SOURCE_QUENCH:
		case ICMP_TIME_EXCEEDED:
			pong->seq = ntohs(nested->echo.sequence);
			break;
	}
	return (OK);
}

void handler(int sig)
{
	if (sig == SIGALRM)
		status |= READY;
	else if (sig == SIGQUIT)
		status |= PRINT_CURRENT;
	else
		status |= ABORTED;
}

t_status	init_socket(t_config *config, session *ses)
{
	if (get_host(*config, &ses->dst) != OK)
		return (FATAL);
	ses->family = config->ipv6 ? AF_INET6 : AF_INET;
	ses->type = SOCK_RAW;
	ses->proto = config->ipv6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	if ((ses->sock = socket(ses->family, ses->type, ses->proto)) == -1)
	{
		my_perror(*config, "socket");
		return (FATAL);
	}
	int yes = 1;
	if ((config->broadcast && setsockopt(ses->sock,SOL_SOCKET, SO_BROADCAST,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_SOCKET, SO_DEBUG,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_IP, IP_RECVTTL,"\1", sizeof yes) == -1)
		|| (config->ttl && setsockopt(ses->sock,SOL_IP, IP_TTL, &config->ttl, sizeof config->ttl) == -1))
	{
		my_perror(*config, "setsockopt");
		return (FATAL);
	}

	return (OK);
}


t_status	loop(t_config config, session *ses)
{
	t_packet ping = {};
	t_packet pong = {};

	while (!(status & ABORTED) && (config.count == 0 || ses->stats.received < config.count))
	{
		if (status & PRINT_CURRENT)
		{
			print_short_stat(ses->stats);
			status -= PRINT_CURRENT;
		}
		if (status & READY && (!config.count || ses->stats.send < config.count))
		{
			if (send_ping(config, ses, &ping) == FATAL)
				return (FATAL);
			if (ping.seq == 1)
				print_ping(config, *ses, ping);
			if (config.interval) {
				alarm(config.interval);
				status -= READY;
			}
			else if (getuid())
			{
				dprintf(2, "%s: cannot flood; minimal interval allowed for user is 200ms\n", config.program_name);
				return (KO);
			}
		}
		long long time;
		switch (receive_pong(config, *ses, &pong))
		{
			case OK:
				if (!config.quiet && !config.flood && print_pong(config, pong) != OK)
					return (FATAL);
				time = pong.date - pong.date_request;
				if (pong.icmp_type == ICMP_ECHOREPLY) {
					if (!check_duplicate(ses, pong.seq)) {
						ses->stats.received++;
						ses->stats.average += time;
						ses->stats.sdev += time * time;
						if (time > ses->stats.max)
							ses->stats.max = time;
						if (time < ses->stats.min)
							ses->stats.min = time;
						set_duplicate(ses, pong.seq);
					} else
						ses->stats.duplicates++;
				}
				else if (pong.icmp_type == ICMP_TIME_EXCEEDED || pong.icmp_type == ICMP_DEST_UNREACH || pong.icmp_type == ICMP_SOURCE_QUENCH)
					ses->stats.errors++;
				break;
			case KO: break;
			case FATAL: return FATAL;
		}
	}
	return (OK);
}

t_status	ft_ping(t_config config)
{
	session			ses = {};
	ses.stats = (t_stat){.min = LLONG_MAX};
	signal(SIGALRM, &handler);
	signal(SIGINT, &handler);
	signal(SIGQUIT, &handler);

	if (init_socket(&config, &ses) != OK)
		return (FATAL);
	status = READY;
	ses.stats.time = ft_utime();
	loop(config, &ses);
	print_stat(config, ses.stats);
	return (OK);
}
