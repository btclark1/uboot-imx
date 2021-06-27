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


int send_update_func( int sub_cmd, int component, const char *str_filename );
static int send_update2(void);
int send_message(void);

static void send_timeout_handler(void);

/*
 * Sends a UDP Packet to the "serverip" containing a string as it's payload.
 * This function is exported and can be used by standalone applications.
 */
int send_message()
{
	if (net_loop(PING) < 0) {
		printf("ping failed; host %s is not alive\n", env_get("ethact"));
		return CMD_RET_FAILURE;
	}

	return(0);
}
/*****************************************************/
// int send_update_func( int sub_cmd, int component, const char *str_filename )
// {


// 	char *buffer[3] = {"B","T","C"};
// 	static char *act;
// 	static int  env_changed_id;
// 	int	env_id = 0;

// 	unsigned char *addr;	
// 	unsigned char enetaddr[6];
// 	uint8_t env_enetaddr[6];

// 	struct udevice *dev;
	
// 	env_id = env_get_id();
// 	printf("env_get_id = %d\n", env_id);
// 	if ((act == NULL) || (env_changed_id != env_id)) {
// 		act = env_get("ethact");
// 		printf("env_get = %s\n", act);
// 		env_changed_id = env_id;
// 	}
	
// 	int dm_rtn = dm_init(false);
// 	printf("dm_init = %d\n", dm_rtn);
	
// 	int dm_scan = dm_scan_platdata(false);
// 	printf("dm_scan_platdata = %d\n", dm_scan);

// 	int init_rtn = eth_init();	
// 	printf("eth_init = %d\n",init_rtn);


// //	dev = eth_get_dev_by_name("eth0");
// 	dev = eth_get_dev();

// 	//eth_set_dev(dev);

// 	addr = eth_get_ethaddr();

// 	printf("dev->name = %s\n", dev->name);
// 	printf("dev->seq = %d\n", dev->seq);
// 	printf("eth_get_ethaddr = %s\n", addr);

// 	int mac_rtn = eth_env_get_enetaddr("ethaddr", enetaddr);

// 	int ret = eth_env_get_enetaddr_by_index("eth", 0, env_enetaddr);
// 	if (!ret) {
// 		printf("No MAC set - eth_env_get_enetaddr_by_index returned = %d\n", ret);
// 	}

// 	int send_rtn = eth_send(buffer, 3);

// 	printf("mac_rtn = %d, enetaddr = %s, env_enetaddr = %s, send_rtn = %d\n",
// 	  mac_rtn, enetaddr, env_enetaddr, send_rtn);

//    return 0;

// }

/******************************************************/
// static int send_update2(void)
// {
// 	uchar *pkt;
// 	ushort *s;
// 	ushort *cp;
// 	struct ethernet_hdr *et;
// 	int len;
// 	ushort chksum;

// 	static const uchar cdp_snap_hdr[8] = {
// 		0xAA, 0xAA, 0x03, 0x00, 0x00, 0x0C, 0x20, 0x00 };

// 	/* Ethernet address of my Ubuntu PC- A8:A1:59:24:20:ED*/
// 	const u8 net_dest_ethaddr[6] = { 0xAB, 0xA1, 0x59, 0x24, 0x20, 0xED };

// 	pkt = net_tx_packet;
// 	et = (struct ethernet_hdr *)pkt;


// 	/* NOTE: trigger sent not on any VLAN */

// 	/* form ethernet header */
// 	memcpy(et->et_dest, net_dest_ethaddr, 6);
// 	memcpy(et->et_src, net_ethaddr, 6);

// 	pkt += ETHER_HDR_SIZE;

// 	/* SNAP header */
// 	memcpy((uchar *)pkt, cdp_snap_hdr, sizeof(cdp_snap_hdr));
// 	pkt += sizeof(cdp_snap_hdr);

// 	/* CDP header */
// 	*pkt++ = 0x02;				/* CDP version 2 */
// 	*pkt++ = 180;				/* TTL */
// 	s = (ushort *)pkt;
// 	cp = s;
// 	/* checksum (0 for later calculation) */
// 	*s++ = htons(0);


// 	/* length of ethernet packet */
// 	len = (uchar *)s - ((uchar *)net_tx_packet + ETHER_HDR_SIZE);
// 	et->et_protlen = htons(len);

// 	len = ETHER_HDR_SIZE + sizeof(cdp_snap_hdr);
// 	//chksum = cdp_compute_csum((uchar *)net_tx_packet + len,
// 	//			  (uchar *)s - (net_tx_packet + len));
// 	chksum = 0;
// 	if (chksum == 0)
// 		chksum = 0xFFFF;
// 	*cp = htons(chksum);

// 	net_send_packet(net_tx_packet, (uchar *)s - net_tx_packet);
// 	return 0;
// }

/******************************************************/


static int do_send_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum send_update_cmd sub_cmd;
	int validate, component = 0;
	const char *str_cmd, *str_filename = NULL;
  	const char *str_component = NULL;


	if (argc < 3)
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
U_BOOT_CMD(send_update, 4, 0, do_send_update,
	   "send update package files to device via ethernet connection",
	   "<validate|no_validate> <component> <filename>\n"
	   "    - validate or no validate of the filename for the specified component\n"
	   "send_update <validate|no_validate>  <component> <filename>");

/********************* END OF FILE *********************************/
