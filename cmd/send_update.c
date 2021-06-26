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

//int api_func(void);
//void test_dump_di(int handle);
//static char *test_stor_typ(int type);

/******************************************************/
int send_update_func( int sub_cmd, int component, const char *str_filename )
{


	char *buffer[3] = {"B","T","C"};
	static char *act;
	static int  env_changed_id;
	int	env_id = 0;

	unsigned char *addr;	
	unsigned char enetaddr[6];
	uint8_t env_enetaddr[6];

	struct udevice *dev;
	
	env_id = env_get_id();
	if ((act == NULL) || (env_changed_id != env_id)) {
		act = env_get("ethact");
		env_changed_id = env_id;
	}
	printf("env_get_id = %d\n", env_id);
	printf("env_get = %s\n", act);
	
	int dm_rtn = dm_init(false);
	int dm_scan = dm_scan_platdata(false);
	int init_rtn = eth_init();	

	printf("dm_init = %d\n", dm_rtn);
	printf("dm_scan_platdata = %d\n", dm_scan);
	printf("eth_init = %d\n",init_rtn);


//	dev = eth_get_dev_by_name("eth0");
	dev = eth_get_dev();

	//eth_set_dev(dev);

	addr = eth_get_ethaddr();

	printf("dev->name = %s\n", dev->name);
	printf("dev->seq = %d\n", dev->seq);
	printf("eth_get_ethaddr = %s\n", addr);

	int mac_rtn = eth_env_get_enetaddr("ethaddr", enetaddr);

	int ret = eth_env_get_enetaddr_by_index("eth", 0, env_enetaddr);
	if (!ret) {
		printf("No MAC set - eth_env_get_enetaddr_by_index returned = %d\n", ret);
	}

	int send_rtn = eth_send(buffer, 3);

	printf("mac_rtn = %d, enetaddr = %s, env_enetaddr = %s, send_rtn = %d\n",
	  mac_rtn, enetaddr, env_enetaddr, send_rtn);

   return 0;

}

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

	send_update_func(sub_cmd, component, str_filename );

	printf("send_update: sub_cmd = %d, component = %d, str_filename = %s\n", 
                     validate, component, str_filename );

	return 0;
}

/******************************************************/
U_BOOT_CMD(send_update, 4, 0, do_send_update,
	   "send update package files to device via ethernet connection",
	   "<validate|no_validate> <component> <filename>\n"
	   "    - validate or no validate of the filename for the specified component\n"
	   "send_update <validate|no_validate>  <component> <filename>");

/********************* END OF FILE *********************************/
