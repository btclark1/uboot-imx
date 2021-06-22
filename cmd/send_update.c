/*
Send update packages from one device to another

B. Clark
 */


#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <linux/err.h>

#include <net.h>

#include <net/tftp.h>

#include "glue.h"

enum send_update_cmd {
	SU_VALIDATE,
	SU_NO_VALIDATE,
};


int send_update_func( int sub_cmd, int component, const char *str_filename );
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



   return 0;

}

int api_func()
{

	int rv = 0, h, i, j, devs_no;
	struct api_signature *sig = NULL;
	ulong start, now;
	struct device_info *di;
	lbasize_t rlen;
	struct display_info disinfo;
}


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

U_BOOT_CMD(send_update, 4, 0, do_send_update,
	   "send update package files to device via ethernet connection",
	   "<validate|no_validate> <component> <filename>\n"
	   "    - validate or no validate of the filename for the specified component\n"
	   "send_update <validate|no_validate>  <component> <filename>");
