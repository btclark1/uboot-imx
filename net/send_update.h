/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	Copied from ping.h
 */

#ifndef __SEND_UPDATE_H__
#define __SEND_UPDATE_H__

#include <common.h>
#include <net.h>

/*
 * Initialize send update (beginning of netloop)
 */
//void send_update_start(void);
void update_start(void);

/*
 * Deal with the receipt of a send update packet
 */
void send_update_receive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len);

#endif /* __SEND_UPDATE_H__ */
