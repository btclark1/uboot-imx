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

//#include <watchdog.h>

struct __packed update_header {
	uchar id;
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
static const unsigned short packet_size = PACKET_SIZE;
static const unsigned short udp_version = 1;


/* The ip address to update to */
struct in_addr net_update_ip;
char net_update_file_name[128];

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
		memcpy(packet, last_packet, last_packet_len);
		net_send_udp_packet(net_server_ethaddr, net_update_ip,
				    remote_port, our_port, last_packet_len);
		return;
	}

	/* Write headers */
	memcpy(packet, &response_header, sizeof(response_header));
	packet += sizeof(response_header);

	switch (header.id) {
	case QUERY:
		printf("update_send State = QUERY, sequence_number = %d\n", sequence_number);
		tmp = htons(sequence_number);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		break;
	case INIT:
		printf("update_send State = INIT, sequence_number = %d\n", sequence_number);
		tmp = htons(udp_version);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		tmp = htons(packet_size);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		break;
	case ERROR:
		printf("update_send State = ERROR, sequence_number = %d\n", sequence_number);
		memcpy(packet, error_msg, strlen(error_msg));
		packet += strlen(error_msg);
		break;
	case UPDATE:
		printf("update_send State = UPDATE, sequence_number = %d\n", sequence_number);
		/* Write response */
		sprintf(response, "%s %s", "From send_update", update_data);
		memcpy(packet, response, strlen(response));
		packet += strlen(response);

		/* Write  file */
		strcpy((char *)packet, net_update_file_name);
		packet += strlen(net_update_file_name) + 1;
		/*
		 * Sent some INFO packets, need to update sequence number in
		 * header
		 */
		if (header.seq != sequence_number) {
			response_header.seq = htons(sequence_number);
			memcpy(packet_base, &response_header,
			       sizeof(response_header));
		}
		break;
	case DONE:
		printf("update_send State = DONE, sequence_number = %d\n", sequence_number);
		/* Write response */
		sprintf(response, "%s - DONE = %d", "From send_update", DONE);
		memcpy(packet, response, strlen(response));
		packet += strlen(response);
		break;

	default:
		pr_err("ID %d not implemented.\n", header.id);
		return;
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

	/* check its from our port and the expected IP */
	if (dport != our_port)
	{
		printf("Not our port ... \n");
		return;
	}

	remote_port = sport;


	printf("update_rec_handler - packet = %s, sport = %d, dport = %d\n", packet, sport, dport);

	if (len < sizeof(struct update_header) || len > PACKET_SIZE)
		return;
	memcpy(&header, packet, sizeof(header));
	header.seq = ntohs(header.seq);
	packet += sizeof(header);
	len -= sizeof(header);

	switch (header.id) {
	case QUERY:
		printf("update_handler State = QUERY\n");
		update_send(header, update_data, 0, 0);
		break;
	case INIT:
	case UPDATE:
		printf("update_handler State = INIT/UPDATE\n");
		update_data_len = len;
		if (len > 0)
			memcpy(update_data, packet, len);


		if (header.seq == sequence_number) {
			update_send(header, update_data,
				      update_data_len, 0);
			sequence_number++;
			printf("update_handler State = INIT/UPDATE... seq #'s match\n");			
		} else if (header.seq == sequence_number - 1) {
			/* Retransmit last sent packet */
			update_send(header, update_data,
				      update_data_len, 1);
			printf("update_handler State = INIT/UPDATE... seq #'s DONT match... retransmit\n");			
		}
		break;
	default:
		pr_err("ID %d not implemented.\n", header.id);
		header.id = ERROR;
		update_send(header, update_data, 0, 0);
		break;
	}
}

/******************************************************/
void update_start_server(void)
{
	struct update_header header;
	char update_data[10];

	printf("Using %s device\n", eth_get_name());
	printf("Listening for Update command on %pI4\n", &net_ip);

	our_port = WELL_KNOWN_PORT;

	net_set_udp_handler(update_rec_handler);

	/* zero out server ether in case the server ip has changed */
	memset(net_server_ethaddr, 0, 6);

	header.id = QUERY;
	header.seq = 1;
	memcpy(update_data, "Test", 4);

	update_send(header, update_data, sizeof(update_data), 0);
}



/********************* END OF FILE *********************************/
