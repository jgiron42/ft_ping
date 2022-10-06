#include "ft_ping.h"

p_status program_status;

status	init_socket(t_config *config, session *ses)
{
	ses->family = config->ipv6 ? AF_INET6 : AF_INET;
	ses->type = SOCK_RAW;
	ses->proto = config->ipv6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	if ((ses->sock = socket(ses->family, ses->type, ses->proto)) == -1)
	{
		my_perror(*config, "socket");
		return (FATAL);
	}
	if (get_host(*config, &ses->dst) != OK)
		return (FATAL);
	if ((config->broadcast && setsockopt(ses->sock,SOL_SOCKET, SO_BROADCAST,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_SOCKET, SO_DEBUG,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_IP, IP_RECVTTL,"\1", 8) == -1)
		|| (config->ttl && setsockopt(ses->sock,SOL_IP, IP_TTL, &config->ttl, sizeof config->ttl) == -1))
	{
		my_perror(*config, "setsockopt");
		return (FATAL);
	}
	return (OK);
}

status	send_ping(t_config config, session *ses,t_packet *ping)
{
	static	int n = 0;
	t_echo_datagram datagram;

	*ping = (t_packet){};
	ping->seq = ++n;
	getdatagram(&datagram, ping->seq, config);
	if ((ping->size = sendto(ses->sock, &datagram, 64, 0, ses->dst->ai_addr, ses->dst->ai_addrlen)) == -1)
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

status	receive_pong(t_config conf, session ses, t_packet *pong)
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
//			pong->seq = ntohs(reply->echo.sequence);
			pong->seq = reply->echo.sequence;
			pong->date_request = reply->echo.date_send.tv_sec * 1000000 + reply->echo.date_send.tv_usec;
			pong->date = ft_utime();
			if (pong->date == -1)
			{
				my_perror(conf, "gettimeofday");
				return (FATAL);
			}
			if (check_duplicate(ses, pong->seq))
				pong->is_dup = true;
			break;
		case ICMP_DEST_UNREACH:
		case ICMP_SOURCE_QUENCH:
		case ICMP_TIME_EXCEEDED:
			pong->seq = ntohs(nested->echo.sequence);
			pong->seq = nested->echo.sequence;
			break;
	}
	return (OK);
}

void	handler(int sig)
{
	if (sig == SIGALRM)
		program_status |= READY;
	else if (sig == SIGQUIT)
		program_status |= PRINT_CURRENT;
	else
		program_status |= ABORTED;
}

status	loop(t_config config, session *ses)
{
	t_packet ping = {};
	t_packet pong = {};
	int			pipes = 0;

	while (!(program_status & ABORTED) && (config.count == 0 || ses->stats.received < config.count))
	{
		if (program_status & PRINT_CURRENT)
		{
			print_short_stat(ses->stats);
			program_status -= PRINT_CURRENT;
		}
		if (program_status & READY && (!config.count || ses->stats.send < config.count))
		{
			if (send_ping(config, ses, &ping) == FATAL)
				return (FATAL);
			pipes++;
			if (ping.seq == 1)
				print_ping(config, *ses, ping);
			if (config.interval) {
				alarm(config.interval);
				program_status -= READY;
			}
			else if (getuid()) {
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
				if (pipes > ses->stats.max_pipe)
					ses->stats.max_pipe = pipes;
				pipes = 0;
				if (pong.icmp_type == ICMP_ECHOREPLY) {
					ses->stats.average += time;
					ses->stats.sdev += time * time;
					if (time > ses->stats.max)
						ses->stats.max = time;
					if (time < ses->stats.min)
						ses->stats.min = time;
					if (!check_duplicate(*ses, pong.seq)) {
						ses->stats.received++;
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

int	ft_ping(t_config config)
{
	session			ses = {};
	ses.stats = (t_stat){.min = LLONG_MAX};
	signal(SIGALRM, &handler);
	signal(SIGINT, &handler);
	signal(SIGQUIT, &handler);

	if (init_socket(&config, &ses) != OK)
	{
		if (ses.dst)
			freeaddrinfo(ses.dst);
		return (2);
	}
	program_status = READY;
	ses.stats.time = ft_utime();
//	if (is_broadcast(config.destination))
//		dprintf(2, "WARNING: pinging broadcast address\n");
	loop(config, &ses);
	print_stat(config, ses.stats);
	if (ses.dst)
		freeaddrinfo(ses.dst);
	return (ses.stats.received == 0);
}


