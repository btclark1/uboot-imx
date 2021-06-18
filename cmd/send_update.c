/*
Send update packages from one device to another

B. Clark
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <linux/err.h>

enum send_update_cmd {
	SU_VALIDATE,
	SU_NO_VALIDATE,
};


int send_update( int sub_cmd, int component, char *str_filename )
{
   return status;

}

static int do_send_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum send_update_cmd sub_cmd;
	int validate, component;
	const char *str_cmd, *str_filename = NULL;
	int ret;


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

	send_update(sub_cmd, component, str_filename );

	printf("send_update: sub_cmd = %d, component = %d, str_filename = %s", 
                     sub_cmd, component, str_filename );


err:
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(send_update, 4, 0, do_send_update,
	   "send update package files to device via ethernet connection",
	   "<validate|no_validate> <component> <filename>\n"
	   "    - validate or no validate of the filename for the specified component\n"
	   "send_update <validate|no_validate>  <component> <filename>");
