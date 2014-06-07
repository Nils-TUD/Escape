/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

/* general */
#define IPC_DEF_SIZE				256

/* requests to file-device */
#define MSG_FILE_OPEN				50
#define MSG_FILE_READ				51
#define MSG_FILE_WRITE				52
#define MSG_FILE_SIZE				53
#define MSG_FILE_CLOSE				54

/* special device messages */
#define MSG_DEV_SHFILE				55
#define MSG_DEV_CANCEL				56
#define MSG_DEV_CREATSIBL			57

/* requests to fs */
#define MSG_FS_OPEN					100
#define MSG_FS_READ					101
#define MSG_FS_WRITE				102
#define MSG_FS_CLOSE				103
#define MSG_FS_STAT					104
#define MSG_FS_SYNCFS				105
#define MSG_FS_LINK					106
#define MSG_FS_UNLINK				107
#define MSG_FS_RENAME				108
#define MSG_FS_MKDIR				109
#define MSG_FS_RMDIR				110
#define MSG_FS_ISTAT				111
#define MSG_FS_CHMOD				112
#define MSG_FS_CHOWN				113

/* Other messages */
#define MSG_SPEAKER_BEEP			200		/* performs a beep */

#define MSG_WIN_CREATE				300		/* creates a window */
#define MSG_WIN_MOVE				301		/* moves a window */
#define MSG_WIN_UPDATE				302		/* requests an update of a window */
#define MSG_WIN_DESTROY				303		/* destroys a window */
#define MSG_WIN_RESIZE				304		/* resizes a window */
#define MSG_WIN_ENABLE				305		/* enables the window-manager */
#define MSG_WIN_DISABLE				306		/* disables the window-manager */
#define MSG_WIN_ADDLISTENER			307		/* announces a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_REMLISTENER			308		/* removes a listener for CREATE_EV or DESTROY_EV */
#define MSG_WIN_SET_ACTIVE			309		/* requests that a window is set to the active one */
#define MSG_WIN_ATTACH				310		/* connect an event-channel to a window */
#define MSG_WIN_SETMODE				311		/* sets the screen mode */
#define MSG_WIN_EVENT				312		/* for all events */

#define MSG_SCR_SETCURSOR			500		/* sets the cursor */
#define MSG_SCR_GETMODE				501		/* gets information about the current video-mode */
#define MSG_SCR_SETMODE				502		/* sets the video-mode */
#define MSG_SCR_GETMODES			503		/* gets all video-modes */
#define MSG_SCR_UPDATE				504		/* updates a part of the screen */

#define MSG_VT_GETFLAG				600		/* gets a flag */
#define MSG_VT_SETFLAG				601		/* sets a flag */
#define MSG_VT_BACKUP				602		/* backups the screen */
#define MSG_VT_RESTORE				603		/* restores the screen */
#define MSG_VT_SHELLPID				604		/* gives the vterm the shell-pid */
#define MSG_VT_ISVTERM				605		/* dummy message on which only vterm answers with no error */
#define MSG_VT_SETMODE				606		/* requests vterm to set the video-mode */

#define MSG_KB_EVENT				700		/* events that the keyboard-driver sends */

#define MSG_MS_EVENT				800		/* events that the mouse-driver sends */

#define MSG_UIM_GETKEYMAP			900		/* gets the current keymap path */
#define MSG_UIM_SETKEYMAP			901		/* sets a keymap, expects the keymap-path as argument */
#define MSG_UIM_EVENT				902		/* the message-id for sending events to the listeners */

#define MSG_PCI_GET_BY_CLASS		1000	/* searches for a PCI device with given class */
#define MSG_PCI_GET_BY_ID			1001	/* searches for a PCI device with given id */
#define MSG_PCI_GET_BY_INDEX		1002	/* searches for a PCI device with given index (0..n) */
#define MSG_PCI_GET_COUNT			1003	/* gets the number of PCI devices */
#define MSG_PCI_READ				1004	/* reads from PCI config space */
#define MSG_PCI_WRITE				1005	/* writes to PCI config space */

#define MSG_INIT_REBOOT				1100	/* requests a reboot */
#define MSG_INIT_SHUTDOWN			1101	/* requests a shutdown */
#define MSG_INIT_IAMALIVE			1102	/* tells init that the shutdown-request has been received
											 * and that you promise to terminate as soon as possible */

#define MSG_NIC_GETMAC				1300	/* get the MAC address of a NIC */
#define MSG_NIC_GETMTU				1301	/* get the MTU of a NIC */

#define MSG_NET_LINK_ADD			1401	/* adds a link */
#define MSG_NET_LINK_REM			1402	/* removes a link */
#define MSG_NET_LINK_CONFIG			1403	/* sets the ip, gateway and subnet-mask of a link */
#define MSG_NET_LINK_MAC			1404	/* gets the MAC address of a link */
#define MSG_NET_ROUTE_ADD			1405	/* adds an entry to the routing table */
#define MSG_NET_ROUTE_REM			1406	/* removes an entry from the routing table */
#define MSG_NET_ROUTE_CONFIG		1407	/* sets the status of a routing table entry */
#define MSG_NET_ROUTE_GET			1408	/* gets the destination for an IP address */
#define MSG_NET_ARP_ADD				1409	/* resolves an IP address and puts it into the ARP table */
#define MSG_NET_ARP_REM				1410	/* removes an IP address from the ARP table */

#define MSG_SOCK_CONNECT			1500	/* connects a socket to the other endpoint */
#define MSG_SOCK_BIND				1501	/* bind a socket to an address */
#define MSG_SOCK_LISTEN				1502	/* puts a socket into listening state */
#define MSG_SOCK_RECVFROM			1503	/* receive data from a socket */
#define MSG_SOCK_SENDTO				1504	/* send data to a socket */

#define MSG_DNS_RESOLVE				1600	/* resolve a name to an address */
#define MSG_DNS_SET_SERVER			1601	/* set the name server */


#define IS_DEVICE_MSG(id)			((id) == MSG_FILE_OPEN || \
									 (id) == MSG_FILE_READ || \
									 (id) == MSG_FILE_WRITE || \
									 (id) == MSG_FILE_CLOSE || \
									 (id) == MSG_DEV_SHFILE || \
									 (id) == MSG_DEV_CREATSIBL || \
									 (id) == MSG_DEV_CANCEL)
