#include "ft_ping.h"

/**
 * ip checksum (see: https://datatracker.ietf.org/doc/html/rfc1071)
 * @param data address of the data to checksum
 * @param size size in bytes of the data
 * @return the checksum
 */
unsigned short int checksum(void *data, size_t size)
{
	long int ret = 0;
	// sum
	for (size_t i = 0; i < size / 2; i++)
		ret += ((short int *)data)[i];
	// adding potential extra byte
	if (size % 2)
		ret += ((char *)data)[size - 1];
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
//	ft_memset(datagram->data, 48, sizeof (datagram->data));
	ft_memset(datagram->data, 0, sizeof (datagram->data));
	for (size_t i = 0; i < sizeof (datagram->data); i++)
		datagram->data[i] = (unsigned char)i;
	datagram->type = config.ipv6 ? 128 : ICMP_ECHO;
	datagram->code = 0;
	// set to 0 before the checksum computation
	datagram->checksum = 0;
	datagram->identifier = 0;
	datagram->sequence = sequence;
	datagram->checksum = checksum((unsigned short int *)datagram, sizeof(t_echo_datagram));
	return (OK);
}

t_status	get_host(t_config config,struct addrinfo **res)
{
	struct addrinfo hints = {};
	hints.ai_family=config.ipv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_flags=AI_ADDRCONFIG;
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
		perror("sendto");
		return (FATAL);
	}
	ping->date = ft_utime();
	if (ping->date == -1)
	{
		perror("gettimeofday");
		return (FATAL);
	}
	ses->stats.send++;
	return (OK);
}

enum {
	WAITING,
	READY,
	ABORTED
} status;

t_status	receive_pong(t_config conf, session ses, t_packet *pong)
{
	struct {
		struct ip iphdr;
		t_echo_datagram data;
	} ipframe = {};
	t_echo_datagram *reply = &ipframe.data;
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
//	for (struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
//		printf("%d %d\n", cmsg->cmsg_level, cmsg->cmsg_type);
//		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
//			pong->ttl = (int) *CMSG_DATA(cmsg);
//			printf("%d\n", pong->ttl);
//			break;
//		}
//	}
//	printf("%p\n", CMSG_FIRSTHDR(&msg));
	if (pong->size == -1 && errno == EAGAIN)
		return (KO);
	if (pong->size == -1)
	{
		perror("recvmsg");
		return (FATAL);
	}

//	printf("%p\n", CMSG_FIRSTHDR(&msg));
	for (struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_TTL) {
			pong->ttl = *(int *)CMSG_DATA(cmsg);
			printf("%d\n", pong->ttl);
			break;
		}
	}
	pong->seq = reply->sequence;
	pong->date = ft_utime();
	if (pong->date == -1)
	{
		perror("gettimeofday");
		return (FATAL);
	}
	unsigned short int tmp = reply->checksum;
	reply->checksum = 0;
	if (tmp != checksum(reply, sizeof *reply))
	{
		dprintf(2, "invalid checksum\n");
		return (KO);
	}
	if (!inet_ntop(conf.ipv6 ? AF_INET6 : AF_INET, &ipframe.iphdr.ip_src, pong->ipstr, sizeof pong->ipstr))
	{
		perror("inet_ntop");
		return (FATAL);
	}
	return (OK);
}

void handler(int sig)
{
	if (sig == SIGALRM)
		status = READY;
	else
		status = ABORTED;
}

t_status	init_socket(t_config *config, session *ses)
{
	if (get_host(*config, &ses->dst) != OK)
		return (FATAL);
	ses->type = SOCK_DGRAM;
	ses->family = config->ipv6 ? AF_INET6 : AF_INET;
	ses->proto = config->ipv6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	if ((ses->sock = socket(ses->family, ses->type, ses->proto)) == -1)
	{
		perror("socket");
		return (FATAL);
	}
	int yes = 1;
	if ((config->broadcast && setsockopt(ses->sock,SOL_SOCKET, SO_BROADCAST,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_SOCKET, SO_DEBUG,"\1", 8) == -1)
		|| (config->debug && setsockopt(ses->sock,SOL_IP, IP_RECVTTL,&yes, sizeof yes) == -1)
		|| (config->ttl && setsockopt(ses->sock,SOL_IP, IP_TTL,&config->ttl, sizeof config->ttl) == -1))
	{
		perror("setsockopt");
		return (FATAL);
	}

	return (OK);
}


t_status	loop(t_config config, session *ses)
{
	t_packet ping = {};
	t_packet pong = {};

	while (status != ABORTED && (config.count == 0 || ses->stats.send < config.count))
	{
		if (status == READY)
		{
			if (send_ping(config, ses, &ping) == FATAL)
				return (FATAL);
			alarm(1);
			status = WAITING;
			if (ping.seq == 1)
				print_ping(config, *ses, ping);
		}
		long long time;
		switch (receive_pong(config, *ses, &pong))
		{
			case OK:
				if (!config.quiet && !config.flood && print_pong(config, pong, ping) != OK)
					return (FATAL);
				time = pong.date - ping.date;
				if (ping.seq == pong.seq && !ping.replied)
				{
					ses->stats.received++;
					ses->stats.average += time;
					ses->stats.sdev += time * time;
					if (time > ses->stats.max)
						ses->stats.max = time;
					if (time < ses->stats.min)
						ses->stats.min = time;
					ping.replied = true;
				}
				else
					ses->stats.duplicates++;
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

	if (init_socket(&config, &ses) != OK)
		return (FATAL);
	status = READY;
	ses.stats.time = ft_utime();
	loop(config, &ses);
	print_stat(config, ses.stats);
	return (OK);
}
