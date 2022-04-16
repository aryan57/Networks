#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#define IP_BUF 32
#define DESTINATION_PORT 32164
#define SOURCE_PORT 6969
#define UDP_PAYLOAD_SIZE 52
#define UDP_PACKET_LEN 1024
#define MAX_TTL 16
#define MAX_BUFF 100
#define MAX_SAME_TTL 3
#define TIMEOUT 1

struct UDP_pseudo_header
{
	uint32_t source_address;	  /* IPv4 address */
	uint32_t destination_address; /* IPv4 address */
	u_int8_t zero;				  /* set to 0, for padding purpose */
	u_int8_t protocol;			  /* 17 for UDP */
	u_int16_t length;			  /* length of UDP header and data */
};

/**
 * copies the first IP address from
 * the hostinfo->h_addr_list and returns 0.
 * Return -1 for errors
 */
int get_ip(char *hostnamebuf, char *ansbuf)
{
	struct hostent *hostinfo = gethostbyname(hostnamebuf);
	if (hostinfo != NULL)
	{
		strcat(ansbuf, inet_ntoa(*((struct in_addr *)*(hostinfo->h_addr_list))));
		return 0;
	}
	return -1;
}

void fill_random_bytes(char *buf, int _size)
{
	for (int i = 0; i < _size; i++)
		buf[i] = 'a' + rand() % 26;
}

void bin(long long n, int numbits)
{
	for (int i=numbits-1; i >=0; i--)
	{
		printf("%lld",(n>>i)&1);
	}
}

unsigned short change_end_16(unsigned short num){
    return ((num&0x00ff)<<8) + ((num&0xff00)>>8);
}

/* Check sum function  */
unsigned short csum(unsigned short *buf, int nwords)
{
	unsigned long sum;
	for (sum = 0; nwords > 0; nwords--,buf++)
	{
		unsigned short num = change_end_16(*buf);
		sum += num;
		sum = (sum >> 16) + (sum & 0xffff);
	}

	sum += (sum >> 16);
	return (unsigned short)(~sum);
}


void fill_ip_header(struct iphdr *ip_header, size_t udp_data_size, const in_addr_t daddr, in_addr_t saddr, char *buffer)
{
	// https://tools.ietf.org/html/rfc791#page-11
	ip_header->version = 4;
	ip_header->ihl = 5; // we are not adding any options or padding
	ip_header->tos = 0; // use normal routines
	ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + udp_data_size);
	ip_header->id = 0; // will be auto filled
	ip_header->frag_off = 0;
	ip_header->protocol = IPPROTO_UDP;
	ip_header->saddr = saddr;
	ip_header->daddr = daddr;
	ip_header->check = 0; // initialised to 0, will be filled after checksum calculation
}

void fill_udp_header(struct udphdr *udp_header, uint16_t destport, uint16_t srcprt, uint16_t len)
{
	// https://datatracker.ietf.org/doc/html/rfc768
	udp_header->dest = destport;  // destination port
	udp_header->source = srcprt;  // source port
	udp_header->len = htons(len); // packet length (including udp payload and udp header) in bytes
}

int main(int argc, char **argv)
{

	if (argc != 2)
	{
		printf("Please write in the format \"mytraceroute <destination domain name>\"\n");
		exit(EXIT_FAILURE);
	}
	/**
	 * 1. The program first finds out the IP address
	 * corresponding to the given domain name
	 * (use gethostbyname())
	 */
	char IP[IP_BUF];
	IP[0] = 0;
	if (get_ip(argv[1], IP) < 0)
	{
		printf("Error in getting ip from the host \"%s\"\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in client_addr;
	client_addr.sin_addr.s_addr = inet_addr(IP);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(DESTINATION_PORT);

	/**
	 * 2. Create the two raw sockets with
	 * appropriate parameters and bind them
	 * to local IP
	 */
	int S1, S2;
	if ((S1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
	{
		printf("Error in raw socket creation.\n");
		exit(EXIT_FAILURE);
	}
	if ((S2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		printf("Error in raw socket creation.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in source_addr;
	source_addr.sin_addr.s_addr = INADDR_ANY;
	source_addr.sin_family = AF_INET;
	source_addr.sin_port = htons(SOURCE_PORT);

	if (bind(S1, (const struct sockaddr *)&source_addr, sizeof(source_addr)) < 0)
	{
		printf("Error in bind.\n");
		exit(EXIT_FAILURE);
	}
	if (bind(S2, (const struct sockaddr *)&source_addr, sizeof(source_addr)) < 0)
	{
		printf("Error in bind.\n");
		exit(EXIT_FAILURE);
	}

	printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n", argv[1], IP, MAX_TTL, UDP_PAYLOAD_SIZE);

	/**
	 * 3. Set the socket option on S1 to include IPHDR_INCL.
	 * ( This tells the IP layer software that you have already
	 * included the IP header yourself,
	 * so it will not insert it again )
	 */
	int one = 1;
	if (setsockopt(S1, IPPROTO_IP, IP_HDRINCL, (void *)&one, sizeof(one)) < 0)
	{
		printf("Error setting IP_HDRINCL\n");
		exit(EXIT_FAILURE);
	}

	/**
	 * 4. Sends a UDP packet for traceroute using S1.
	 */

	char buffer[UDP_PACKET_LEN];
	bzero(buffer, UDP_PACKET_LEN);
	char udp_data[UDP_PAYLOAD_SIZE];
	struct iphdr *ip_header = (struct iphdr *)buffer;
	struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct iphdr));
	fill_ip_header(ip_header, sizeof(udp_data), client_addr.sin_addr.s_addr, source_addr.sin_addr.s_addr, buffer);
	fill_udp_header(udp_header, client_addr.sin_port, source_addr.sin_port, sizeof(struct udphdr) + sizeof(udp_data));

	struct UDP_pseudo_header pseudo_header;
	pseudo_header.source_address = source_addr.sin_addr.s_addr;
	pseudo_header.destination_address = client_addr.sin_addr.s_addr;
	pseudo_header.zero = 0;
	pseudo_header.protocol = IPPROTO_UDP;
	pseudo_header.length = udp_header->len;

	int TTL = 1;
	int same_ttl = 0;
	int can_send = 1;
	clock_t start_clock, end_clock;
	int remain_time = TIMEOUT;
	fd_set read_set;

	while (TTL <= MAX_TTL)
	{
		if (can_send)
		{
			fill_random_bytes(udp_data, UDP_PAYLOAD_SIZE);
			strcpy(buffer + sizeof(struct iphdr) + sizeof(struct udphdr), udp_data);

			ip_header->ttl = TTL;  // time to live

			ip_header->check=0;
			ip_header->check = csum((unsigned short *)buffer, ip_header->ihl * 2);
			ip_header->check=htons(ip_header->check);

			udp_header->check = 0; // checksum initialised to 0, will be filled after checksum calculation

			/**
			 * UDP checksum calculation according to RFC 768
			 *
			 * checksum is the ones complment of the ones complement
			 * sum of the pseudo header + UDP header + UDP data
			 *
			 * https://imgur.com/a/mA4b51t
			 */
			char checksum_buffer[sizeof(pseudo_header) + sizeof(udp_header) + sizeof(udp_data)];
			memcpy(checksum_buffer, (void *)&pseudo_header, sizeof(pseudo_header));
			memcpy(checksum_buffer + sizeof(pseudo_header), (void *)&udp_header, sizeof(udp_header));
			memcpy(checksum_buffer + sizeof(pseudo_header) + sizeof(udp_header), (void *)&udp_data, sizeof(udp_data));
			/**
			 * 1 word = 2 bytes
			 * size of checksum_buffer must be in multiple of 16 bits or a word
			 */
			if (sizeof(checksum_buffer) % 2)
			{
				printf("size of checksum_buffer must be a multiple of 16 bits\n");
				exit(EXIT_FAILURE);
			}
			udp_header->check = csum((unsigned short *)checksum_buffer, sizeof(checksum_buffer)/2);
			udp_header->check = 0;

			same_ttl++;
			if (sendto(S1, buffer, ntohs(ip_header->tot_len), 0, (const struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
			{
				printf("Error in sendto()\n");
				printf("%s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			start_clock = clock();
		}

		FD_ZERO(&read_set);
		FD_SET(S2, &read_set);
		/**
		 * 5. Make a select call to wait for a ICMP message to be
		 * received using the raw socket S2 or a timeout value of
		 * 1 sec.
		*/
		int res;
		struct timeval t = {remain_time, 0};
		if ((res = select(S2 + 1, &read_set, NULL, NULL, &t)) < 0)
		{
			printf("Error in select()\n");
			exit(EXIT_FAILURE);
		}

		/**
		 * 7. If the select call times out, repeat from step 4 again
		*/
		if (res == 0)
		{
			// timeout
			if (same_ttl == MAX_SAME_TTL)
			{
				printf("%d * * * *\n", TTL);
				TTL++;
				same_ttl = 0;
			}
			can_send = 1;
			remain_time = TIMEOUT;
			continue;
		}

		/**
		 * 6. If the select() call comes out with receive
		 * of an ICMP message in S2:
		*/
		if (FD_ISSET(S2, &read_set))
		{
			char msg[MAX_BUFF];
			bzero(msg, MAX_BUFF);
			int recv_len;
			socklen_t raddr_len = sizeof(source_addr);
			if ((recv_len = recvfrom(S2, msg, MAX_BUFF, 0, (struct sockaddr *)&source_addr, &raddr_len)) < 0)
			{
				printf("Error in recvfrom()\n");
				exit(EXIT_FAILURE);
			}
			end_clock = clock();

			if (recv_len == 0)
			{
				can_send = 1;
				remain_time = TIMEOUT;
				continue;
			}

			struct iphdr ip_header_recv = *((struct iphdr *)msg);
			struct icmphdr icmp_header_recv = *((struct icmphdr *)(msg + sizeof(struct iphdr)));

			if (ip_header_recv.protocol == IPPROTO_ICMP) // ICMP Header
			{
				if (icmp_header_recv.type == ICMP_DEST_UNREACH)
				{
					// We reached the destination address.
					// To be safe, check that the source IP address
					// of the ICMP Destination Unreachable Message
					// matches with your target server IP address.

					if (ip_header->daddr == ip_header_recv.saddr)
					{
						printf("%d\t%s\t%.3f\n", TTL, inet_ntoa(*((struct in_addr *)&ip_header_recv.saddr)), (float)(end_clock - start_clock) / CLOCKS_PER_SEC * 1000);
					}
					close(S1);
					close(S2);
					return 0;
				}
				else if (icmp_header_recv.type == ICMP_TIME_EXCEEDED)
				{
					printf("%d\t%s\t%.3f\n", TTL, inet_ntoa(*((struct in_addr *)&ip_header_recv.saddr)), (float)(end_clock - start_clock) / CLOCKS_PER_SEC * 1000);
					same_ttl = 0;
					TTL++;
					can_send = 1;
					remain_time = TIMEOUT;
					continue;
				}
				else
				{
					// printf("some other error\n");
					same_ttl++;
					can_send = 0;
				}
			}
		}
	}

	close(S1);
	close(S2);
	return 0;
}