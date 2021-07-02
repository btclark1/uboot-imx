/*
Send update packages from one device to another

B. Clark
 */

/* CMD command includes */
#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>    /* getenv, atoi */

#include <linux/err.h>
#include <dm.h>
#include <dm/root.h>

/* may need for raw eth */
#include <net.h>
#include <net/tftp.h>

struct __packed update_header {
	uchar id;
	uchar flags;
	unsigned short seq;
};
#define UPDATE_MESSAGE_LEN 1024
#define PACKET_SIZE 1024
#define DATA_SIZE (PACKET_SIZE - sizeof(struct update_header))

#define WELL_KNOWN_PORT 5554

/* The UDP port at their end */
static int remote_port;
/* The UDP port at our end */
static int our_port;

/* The ip address to update to */
struct in_addr net_update_ip;
char net_update_file_name[128];
int run_as_client = 0;

void update_send(struct update_header header, char *update_data,
			  unsigned int update_data_len);
void update_start(void);
static void update_rec_handler(uchar *packet, unsigned int dport,
			     struct in_addr sip, unsigned int sport,
			     unsigned int len);

static int update_load(char *filename, ulong msec_max, int cnt_max, ulong addr);

/****************************************************************/
void update_send(struct update_header header, char *update_data,
			  unsigned int update_data_len)
{
	uchar *packet;
	uchar *packet_base;
	int len = 0;
	char message[UPDATE_MESSAGE_LEN] = {0};


	struct update_header message_header = header;
	packet = net_tx_packet + net_eth_hdr_size() + IP_UDP_HDR_SIZE;
	packet_base = packet;

	message_header.seq++;
	unsigned int seq_cnt = message_header.seq;
	/* Write headers */
	message_header.seq = htons(message_header.seq);
	memcpy(packet, &message_header, sizeof(message_header));
	packet += sizeof(message_header);
	/* Write  file */
	//strcpy((char *)packet, net_update_file_name);
	//packet += strlen(net_update_file_name) + 1;

	/* Write message */
	//sprintf(message, "%s %s", "From send_update ", update_data);
	//memcpy(packet, message, strlen(message));
	
	memcpy(packet, update_data, update_data_len);
	packet += strlen(message);

	len = packet - packet_base;
		
	printf("In update_send - len = %d\n", len);

	net_send_udp_packet(net_server_ethaddr, net_update_ip, remote_port, our_port, len);

	if(seq_cnt >= 10)
		net_set_state(NETLOOP_SUCCESS);
	
	printf("End of update_send...seq_cnt = %d \n", seq_cnt);
}
/**********************************************************************/
/**
 * update_rec_handler() - Incoming UDP packet handler.
 *
 * @packet: Pointer to incoming UDP packet
 * @dport: Destination UDP port
 * @sip: Source IP address
 * @sport: Source UDP port
 * @len: Packet length
 */
static void update_rec_handler(uchar *packet, unsigned int dport,
			     struct in_addr sip, unsigned int sport,
			     unsigned int len)
{
	struct update_header header;
	char update_data[DATA_SIZE] = {0};
	unsigned int update_data_len = 0;

	if( run_as_client)
		printf("Received as Client ... \n");
	else
		printf("Received as Server ... \n");
	
	/* check its from our port and the expected IP */
	if (dport != our_port)
	{
		printf("Not our port ... \n");
		return;
	}
	//NOT sure we want to do this
	//remote_port = sport;

	printf("n_addr sip.s_addr =  %d.%d.%d.%d\n", 
				(ntohl(sip.s_addr)>>12), (ntohl(sip.s_addr)>>8),
				(ntohl(sip.s_addr)>>4), ntohl(sip.s_addr));
	printf("update_rec_handler - sport = %d, dport = %d\n", sport, dport);

	if (len < sizeof(struct update_header) || len > PACKET_SIZE)
		return;
	memcpy(&header, packet, sizeof(header));
	header.seq = ntohs(header.seq);	
	packet += sizeof(header);
	len -= sizeof(header);

	update_data_len = len;
	if ((len > 0) && (len < DATA_SIZE))
		memcpy(update_data, packet, len);

	printf("Sending back -> header.id = %d , header.flags = %d, header.seq = %d\n",
			header.id, header.flags, header.seq);
	printf("Sending back -> update_data = %s, len = %d \n",update_data, len);

	
	update_send(header, update_data,	update_data_len);

	// delay some so we can see whats going on.
	mdelay(1000);

	if( header.seq >= 10)
	{
		net_set_state(NETLOOP_SUCCESS);
	}

	printf("End of update_rec_handler...header.seq = %d \n", header.seq);

}

#include <cpu_func.h>

static int update_load(char *filename, ulong msec_max, int cnt_max, ulong addr)
{
	int size, rv;
	ulong saved_timeout_msecs;
	int saved_timeout_count;
	char *saved_netretry, *saved_bootfile;

	rv = 0;
	/* save used globals and env variable */
	saved_timeout_msecs = tftp_timeout_ms;
	saved_timeout_count = tftp_timeout_count_max;
	saved_netretry = strdup(env_get("netretry"));
	saved_bootfile = strdup(net_boot_file_name);

	/* set timeouts for auto-update */
	tftp_timeout_ms = msec_max;
	tftp_timeout_count_max = cnt_max;

	/* we don't want to retry the connection if errors occur */
	env_set("netretry", "no");

	/* download the update file */
	image_load_addr = addr;
	copy_filename(net_boot_file_name, filename, sizeof(net_boot_file_name));
	size = net_loop(TFTPGET);

	if (size < 0)
		rv = 1;
	else if (size > 0)
		flush_cache(addr, size);

	/* restore changed globals and env variable */
	tftp_timeout_ms = saved_timeout_msecs;
	tftp_timeout_count_max = saved_timeout_count;

	env_set("netretry", saved_netretry);
	if (saved_netretry != NULL)
		free(saved_netretry);

	if (saved_bootfile != NULL) {
		copy_filename(net_boot_file_name, saved_bootfile,
			      sizeof(net_boot_file_name));
		free(saved_bootfile);
	}

	return rv;
}


static void response_timeout_handler(void)
{
	printf("Timeout setting NETLOOP_FAIL\n");
	eth_halt();
	net_set_state(NETLOOP_FAIL);	/* we did not get the reply */
}

/******************************************************/
static void update_wait_arp_handler(uchar *pkt, unsigned dest,
				 struct in_addr sip, unsigned src,
				 unsigned len)
{
	printf("Timeout on arp,  update_wait_arp_handler\n");
	net_set_state(NETLOOP_FAIL); /* got arp reply - quit net loop */
}

/******************************************************/
void update_start(void)
{
	struct update_header header;
	char * update_data;

	printf("Using %s device\n", eth_get_name());


	our_port = WELL_KNOWN_PORT;
	remote_port = WELL_KNOWN_PORT;

	//net_set_arp_handler(update_wait_arp_handler);

	if(run_as_client)
	{
		header.id = 1;
		header.seq = 0;
		header.flags = 0xff;
		memcpy(update_data, "Test", 4);
		
		printf("Sending command on %pI4\n", &net_ip);

		//net_set_timeout_handler(5000UL, response_timeout_handler);
		
		memset(update_data, 0x5A, UPDATE_MESSAGE_LEN);

		update_send(header, update_data, sizeof(update_data));

		net_set_udp_handler(update_rec_handler);
		printf("In update_start - Client - After net_set_udp_handler\n");

	} 
	else
	{
		printf("Listening for Update command on %pI4\n", &net_ip);
		net_set_udp_handler(update_rec_handler);
		printf("In update_start - Server - After net_set_udp_handler\n");
	}

}



/********************* END OF FILE *********************************/
