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

/* API demo includes */
#include <env.h>
#include <linux/types.h>
#include <api_public.h>
#include "glue.h"

/* may need for raw eth */
//#include <net.h>
//#include <net/tftp.h>

/* API demo defines */
#define BUF_SZ		2048
#define WAIT_SECS	5
static char buf[BUF_SZ];
#define errf(fmt, args...) do { printf("ERROR @ %s(): ", __func__); printf(fmt, ##args); } while (0)

enum send_update_cmd {
	SU_VALIDATE,
	SU_NO_VALIDATE,
};


int send_update_func( int sub_cmd, int component, const char *str_filename );
int api_func(void);
void test_dump_di(int handle);
static char *test_stor_typ(int type);

/******************************************************/
int send_update_func( int sub_cmd, int component, const char *str_filename )
{


	char *buffer[3] = {"B","T","C"};
	static char *act;
	static int  env_changed_id;
	int	env_id;

	env_id = env_get_id();
	if ((act == NULL) || (env_changed_id != env_id)) {
		act = env_get("ethact");
		env_changed_id = env_id;
	}
	
	int rtn = eth_init();	

	printf("env_id = %d, act = %s, rtn = %d\n", env_id, act, rtn);

	eth_send(buffer, 3);

	printf(" Call to api_func() returned - %d\n", api_func());

   return 0;

}

/******************************************************/
int api_func()
{
	int rv = 0, h, i, j, devs_no;
	struct device_info *di;
	
	/* enumerate devices */
	printf("\n*** Enumerate devices ***\n");
	devs_no = ub_dev_enum();

	printf("Number of devices found: %d\n", devs_no);
	if (devs_no == 0)
		return -1;

	printf("\n*** Show devices ***\n");
	for (i = 0; i < devs_no; i++) {
		test_dump_di(i);
		printf("\n");
	}

	/* test networking */
	printf("Trying network devices...\n");
	for (i = 0; i < devs_no; i++) {
		di = ub_dev_get(i);

		if (di->type == DEV_TYP_NET)
			break;

	}
	if (i == devs_no)
		printf("No network devices available\n");
	else {
		if ((rv = ub_dev_open(i)) != 0)
			errf("open device %d error %d\n", i, rv);
		else if ((rv = ub_dev_send(i, &buf, 2048)) != 0)
			errf("could not send to device %d, error %d\n", i, rv);

		ub_dev_close(i);
	}

	if (ub_dev_close(h) != 0)
		errf("could not close device %d\n", h);

	printf("\n*** Env vars ***\n");

	printf("ethact = %s\n", ub_env_get("ethact"));
	printf("old fileaddr = %s\n", ub_env_get("fileaddr"));
	ub_env_set("fileaddr", "deadbeef");
	printf("new fileaddr = %s\n", ub_env_get("fileaddr"));

	const char *env = NULL;

	while ((env = ub_env_enum(env)) != NULL)
		printf("%s = %s\n", env, ub_env_get(env));

	

	/* reset */
	printf("\n*** Resetting board ***\n");
	ub_reset();
	printf("\nHmm, reset returned...?!\n");

	return rv;
}

/******************************************************/
void test_dump_di(int handle)
{
	int i;
	struct device_info *di = ub_dev_get(handle);

	printf("device info (%d):\n", handle);
	printf("  cookie\t= 0x%08x\n", (uint32_t)di->cookie);
	printf("  type\t\t= 0x%08x\n", di->type);

	if (di->type == DEV_TYP_NET) {
		printf("  hwaddr\t= ");
		for (i = 0; i < 6; i++)
			printf("%02x ", di->di_net.hwaddr[i]);

		printf("\n");

	} else if (di->type & DEV_TYP_STOR) {
		printf("  type\t\t= %s\n", test_stor_typ(di->type));
		printf("  blk size\t\t= %d\n", (unsigned int)di->di_stor.block_size);
		printf("  blk count\t\t= %d\n", (unsigned int)di->di_stor.block_count);
	}
}

/******************************************************/
static char *test_stor_typ(int type)
{
	if (type & DT_STOR_IDE)
		return "IDE";

	if (type & DT_STOR_MMC)
		return "MMC";

	if (type & DT_STOR_SATA)
		return "SATA";

	if (type & DT_STOR_SCSI)
		return "SCSI";

	if (type & DT_STOR_USB)
		return "USB";

	return "Unknown";
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
