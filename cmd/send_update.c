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
	uchar flag;
	unsigned short seq;
};
#define UPDATE_RESPONSE_LEN 10
#define PACKET_SIZE 1024
#define DATA_SIZE (PACKET_SIZE - sizeof(struct update_header))

#define WELL_KNOWN_PORT 5554

/* The UDP port at their end */
static int remote_port;
/* The UDP port at our end */
static int our_port;

enum { ERROR = 0,	QUERY = 1, INIT = 2,	UPDATE = 3, DONE = 4,};


/* Keep track of last packet for resubmission */
static uchar last_packet[PACKET_SIZE];
static unsigned int last_packet_len;

/* Sequence number sent for every packet */
static unsigned short sequence_number = 1;

/* The ip address to update to */
struct in_addr net_update_ip;
char net_update_file_name[128];
int run_as_client = 0;

//void send_update_start(void);

void update_send(struct update_header header, char *update_data,
			  unsigned int update_data_len, uchar retransmit);

void update_start_server(void);

/******************************************/
//void send_update_start(void)
//{
	//int rtn = send_message();
//	update_send("BTCBTC");
//	return;
//}
/******************************************/
void update_send(struct update_header header, char *update_data,
			  unsigned int update_data_len, uchar retransmit)
{
	uchar *packet;
	uchar *packet_base;
	int len = 0;
	char response[UPDATE_RESPONSE_LEN] = {0};
	short tmp;
	const char *error_msg = "An error occurred.";

	struct update_header response_header = header;
	++sequence_number;
	packet = net_tx_packet + net_eth_hdr_size() + IP_UDP_HDR_SIZE;
	packet_base = packet;

		/* Resend last packet */
	if (retransmit) {
		printf("Resending last packet...\n");
		memcpy(packet, last_packet, last_packet_len);
		net_send_udp_packet(net_server_ethaddr, net_update_ip,
				    remote_port, our_port, last_packet_len);
		return;
	}

	/* Write headers */
	memcpy(packet, &response_header, sizeof(response_header));
	packet += sizeof(response_header);


	printf("update_send State = UPDATE, sequence_number = %d\n", sequence_number);
	/* Write response */
	sprintf(response, "%s %s", "From send_update", update_data);
	memcpy(packet, response, strlen(response));
	packet += strlen(response);

	/* Write  file */
	//strcpy((char *)packet, net_update_file_name);
	//packet += strlen(net_update_file_name) + 1;

	/*  from fastboot code.... can use to resend
		* may have Sent some INFO packets, need to update sequence number in
		* header
		*/
	if (header.seq != sequence_number) {
		response_header.seq = htons(sequence_number);
		memcpy(packet_base, &response_header,
					sizeof(response_header));
	}


	len = packet - packet_base;

	printf("In update_send - len = %d\n", len);

	net_send_udp_packet(net_server_ethaddr, net_update_ip, remote_port, our_port, len);

	printf("In update_send - len = %d\n", len);

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

	//remote_port = sport;

	
	printf("in_addr sip.s_addr = %d\n", sip.s_addr);

	printf("update_rec_handler - packet = %s, sport = %d, dport = %d\n", packet, sport, dport);

	if (len < sizeof(struct update_header) || len > PACKET_SIZE)
		return;
	memcpy(&header, packet, sizeof(header));
	header.seq = ntohs(header.seq);
	packet += sizeof(header);
	len -= sizeof(header);

	update_data_len = len;
	if (len > 0)
		memcpy(update_data, packet, len);

	printf("Sending back -> update_data = %s, len = %d\n",update_data, len);

	if (header.seq == sequence_number) {
		update_send(header, update_data,
					update_data_len, 0);
		sequence_number++;
		
		printf("header.seq == sequence_number - setting NETLOOP_SUCCESS\n");
		net_set_state(NETLOOP_SUCCESS);

	} else if (header.seq == sequence_number - 1) {
		/* Retransmit last sent packet */
		update_send(header, update_data,
					update_data_len, 1);
		printf("update_handler .. seq #'s DONT match... retransmit\n");			
	}
}
static void response_timeout_handler(void)
{
	printf("Timeout seting NETLOOP_FAIL\n");

	eth_halt();
	net_set_state(NETLOOP_FAIL);	/* we did not get the reply */
}

/******************************************************/
void update_start(void)
{
	struct update_header header;
	char update_data[10];

	printf("Using %s device\n", eth_get_name());


	our_port = WELL_KNOWN_PORT;
	remote_port = WELL_KNOWN_PORT;

	if(run_as_client)
	{
		header.id = 1;
		header.seq = 1;
		memcpy(update_data, "Test", 4);
		
		printf("Sending command on %pI4\n", &net_ip);

		update_send(header, update_data, sizeof(update_data), 0);

		net_set_timeout_handler(5000UL, response_timeout_handler);
	} 
	else
	{
		printf("Listening for Update command on %pI4\n", &net_ip);
		net_set_udp_handler(update_rec_handler);
	}

}



/********************* END OF FILE *********************************/
