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
	uchar flags;
	unsigned short seq;
};

/* Sequence number sent for every packet */
static unsigned short sequence_number = 1;

/* The ip address to update to */
struct in_addr net_update_ip;

int send_update_func( int sub_cmd, int component, const char *str_filename );
static int send_update2(void);
int send_message(void);

static void send_timeout_handler(void);

void send_update_start(void);

void udp_send(const char *msg);



void send_update_start(void)
{
	int rtn = send_message();

	udp_send("BTCBTC");

	return;

}

void send_update_receive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len)
{
	return;
}

/*
 * Sends a UDP Packet to the "serverip" containing a string as it's payload.
 * This function is exported and can be used by standalone applications.
 */
int send_message()
{
	//  WORKS!!
	//if (net_loop(PING) < 0) {
	//	printf("ping failed; host %s is not alive\n", env_get("ethact"));
	//	return CMD_RET_FAILURE;
	//}

	uchar *pkt;
	ushort *s;
	ushort *cp;
	struct ethernet_hdr *et;
	int len;
	ushort chksum;

	static const uchar cdp_snap_hdr[8] = {
		0xAA, 0xAA, 0x03, 0x00, 0x00, 0x0C, 0x20, 0x00 };

	/* Ethernet address of my Ubuntu PC- A8:A1:59:24:20:ED*/
	const u8 net_dest_ethaddr[6] = { 0xAB, 0xA1, 0x59, 0x24, 0x20, 0xED };

	pkt = net_tx_packet;
	et = (struct ethernet_hdr *)pkt;


	/* NOTE: trigger sent not on any VLAN */

	/* form ethernet header */
	memcpy(et->et_dest, net_dest_ethaddr, 6);
	memcpy(et->et_src, net_ethaddr, 6);

	pkt += ETHER_HDR_SIZE;

	/* SNAP header */
	memcpy((uchar *)pkt, cdp_snap_hdr, sizeof(cdp_snap_hdr));
	pkt += sizeof(cdp_snap_hdr);

	/* CDP header */
	*pkt++ = 0x02;				/* CDP version 2 */
	*pkt++ = 180;				/* TTL */
	s = (ushort *)pkt;
	cp = s;
	/* checksum (0 for later calculation) */
	*s++ = htons(0);


	/* length of ethernet packet */
	len = (uchar *)s - ((uchar *)net_tx_packet + ETHER_HDR_SIZE);
	et->et_protlen = htons(len);

	len = ETHER_HDR_SIZE + sizeof(cdp_snap_hdr);
	//chksum = cdp_compute_csum((uchar *)net_tx_packet + len,
	//			  (uchar *)s - (net_tx_packet + len));
	chksum = 0;
	if (chksum == 0)
		chksum = 0xFFFF;
	*cp = htons(chksum);

	net_send_packet(net_tx_packet, (uchar *)s - net_tx_packet);

	printf("net_ethaddr = %s\n", net_ethaddr);

	return 0;

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
		.flags = 0,
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

	len = packet - packet_base;

	net_send_udp_packet(net_server_ethaddr, net_update_ip, 15, 15, len);
}

/******************************************************/

static int send_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum send_update_cmd sub_cmd;
	int validate, component = 0;
	const char *str_cmd, *str_filename = NULL;
  	const char *str_component = NULL;


	if (argc < 4)
   {
 show_usage:
		return CMD_RET_USAGE;
   }
   /* get validate or no_validate */
	str_cmd = argv[1];
	/* parse the behavior */
	switch (*str_cmd) {
	case 'v':
		sub_cmd = SU_VALIDATE;
      validate = 1;
		break;
	case 'n':
		sub_cmd = SU_NO_VALIDATE;
      validate = 0;
		break;
	default:
		goto show_usage;
	}

   /* get component to update */
	str_component = argv[2];
	if (!str_component)
		goto show_usage;
   component = simple_strtoul(str_component, NULL, 10);

   /* get file name */
	str_filename = argv[3];
	if (!str_filename)
		goto show_usage;

	//send_update_func(sub_cmd, component, str_filename );
	//printf("send_update: sub_cmd = %d, component = %d, str_filename = %s\n", 
   //                  validate, component, str_filename );
	
	//int rtn = send_update2();
	//printf("send_update2 = %d\n", rtn);

	int rtn = send_message();
	printf("send_message = %d\n", rtn);


	return 0;
}

/******************************************************/
//U_BOOT_CMD(send_update, 4, 0, do_send_update,
//	   "send update package files to device via ethernet connection",
//	   "<validate|no_validate> <component> <filename>\n"
//	   "    - validate or no validate of the filename for the specified component\n"
//	   "send_update <validate|no_validate>  <component> <filename>");

/********************* END OF FILE *********************************/
