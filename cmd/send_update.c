/*
Send update packages from one device to another

B. Clark
 */

/* CMD command includes */
#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <linux/err.h>
#include <dm.h>
#include <dm/root.h>

/* may need for raw eth */
#include <net.h>
#include <net/tftp.h>

#include <watchdog.h>

/* API demo defines */
//#define BUF_SZ		2048
//#define WAIT_SECS	5
//static char buf[BUF_SZ];
//#define errf(fmt, args...) do { printf("ERROR @ %s(): ", __func__); printf(fmt, ##args); } while (0)

enum send_update_cmd {
	SU_VALIDATE,
	SU_NO_VALIDATE,
};

#define UPDATE_RESPONSE_LEN 10

struct __packed update_header {
	uchar id;
	unsigned short seq;
};

/* Sequence number sent for every packet */
static unsigned short sequence_number = 1;

/* The ip address to update to */
struct in_addr net_update_ip;
char net_update_file_name[128];

void send_update_start(void);

void udp_send(const char *msg);
void send_update_receive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len);

/******************************************/
void send_update_start(void)
{
	//int rtn = send_message();

	udp_send("BTCBTC");

	return;

}

/******************************************/
void send_update_receive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len)
{
	return;
}

/******************************************/
void udp_send(const char *msg)
{
	uchar *packet;
	uchar *packet_base;
	int len = 0;
	char response[UPDATE_RESPONSE_LEN] = {0};

	struct update_header response_header = {
		.id = 1,
		.seq = htons(sequence_number)
	};
	++sequence_number;
	packet = net_tx_packet + net_eth_hdr_size() + IP_UDP_HDR_SIZE;
	packet_base = packet;

	/* Write headers */
	memcpy(packet, &response_header, sizeof(response_header));
	packet += sizeof(response_header);

	/* Write response */
	sprintf(response, "%s %s", "From send_update", msg);
	memcpy(packet, response, strlen(response));
	packet += strlen(response);

	/* Write  file */
	strcpy((char *)packet, net_update_file_name);
	packet += strlen(net_update_file_name) + 1;

	len = packet - packet_base;

	net_send_udp_packet(net_server_ethaddr, net_update_ip, 15, 15, len);
}

/******************************************************/


/********************* END OF FILE *********************************/
