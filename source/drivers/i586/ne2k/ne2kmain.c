/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/driver/pci.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <stdlib.h>
#include <stdio.h>

#define NIC_CLASS		0x02
#define NIC_SUBCLASS	0x00

#define NE2K_VENDOR_ID	0x10ec
#define NE2K_DEVICE_ID	0x8029

/* page 0 */
#define REG_CMD			0x00		/* command register */
#define REG_PSTART		0x01		/* page start register */
#define REG_PSTOP		0x02		/* page stop register */
#define REG_BNRY		0x03		/* boundary pointer */
#define REG_ISR			0x07		/* interrupt status regsiter */
#define REG_RBCR0		0x0A		/* remote byte count register 0 */
#define REG_RBCR1		0x0B		/* remote byte count register 1 */
#define REG_RCR			0x0C		/* receive configuration register */
#define REG_TCR			0x0D		/* transmit configuration register */
#define REG_DCR      	0x0E		/* data configuration register */
#define REG_IMR			0x0F		/* interrupt mask register */

/* page 1 */
#define REG_CURR		0x07		/* current page */

#define CMD_STP			(1 << 0)	/* software reset */
#define CMD_STA			(1 << 1)	/* activate NIC */
#define CMD_TXP			(1 << 2)	/* transmit packet */
#define CMD_RD0			(1 << 3)	/* remote dma;	rd2=0,rd1=0,rd0=1: remote read */
#define CMD_RD1			(1 << 4)	/* 				rd2=0,rd1=1,rd0=0: remote write */
#define CMD_RD2			(1 << 5)	/* 				rd2=0,rd1=1,rd0=1: send packet */
									/*				rd2=1,rd1=X,rd0=X: abort/complete remote DMA */
#define CMD_PS0			(1 << 6)	/* page select: ps1=0,ps0=0: register page 0 */
#define CMD_PS1			(1 << 7)	/*				ps1=0,ps1=1: register page 1 */
									/*				ps1=1,ps1=0: register page 2 */

#define DCR_WTS			(1 << 0)	/* word transfer select; 1 = word, 0 = byte */
#define DCR_BOS			(1 << 1)	/* byte order select; 0 = MSB in AD15-AD8, 1 = MSB in AD7-AD0 */
#define DCR_LAS			(1 << 2)	/* long address select; 0 = dual 16-bit DMA, 1 = single 32-bit */
#define DCR_LS			(1 << 3)	/* loopback select; 0 = loopback mode, 1 = normal operation */

#define RCR_SEP			(1 << 0)	/* save errored packets */
#define RCR_AR			(1 << 1)	/* accept runt packets (packets with less than 64 bytes) */
#define RCR_AB			(1 << 2)	/* accept broadcast (all 1's in the dest-address) */
#define RCR_AM			(1 << 3)	/* accept multicast */
#define RCR_PRO			(1 << 4)	/* promiscuous physical; phys-addr must match with PAR0-PAR5 */
#define RCR_MON			(1 << 5)	/* monitor mode; 0 = buffer to mem, 1 = don't buffer to mem */

#define TCR_CRC			(1 << 0)	/* inhibit CRC; 0 = CRC appended by transmitter, 1 = inhibited */
#define TCR_LB0			(1 << 1)	/* encoded loopback control */
#define TCR_LB1			(1 << 2)
#define TCR_ATD			(1 << 3)	/* auto transmit disable */
#define TCR_OFST		(1 << 4)	/* collision offset enable */

#define ISR_PRX			(1 << 0)	/* packet received */
#define ISR_PTX			(1 << 1)	/* packet transmitted */
#define ISR_RXE			(1 << 2)	/* receive error */
#define ISR_TXE			(1 << 3)	/* transmit error */
#define ISR_OVW			(1 << 4)	/* overwrite warning */
#define ISR_CNT			(1 << 5)	/* counter overflow */
#define ISR_RDC			(1 << 6)	/* remote DMA complete */
#define ISR_RST			(1 << 7)	/* reset status */

#define PAGE_TX			0x40
#define PAGE_RX			0x50
#define PAGE_STOP		0x80

typedef struct {
	uint16_t basePort;
} sNe2k;

static void writeReg(uint16_t reg,uint8_t value);
static void init(sPCIDevice *nic);

static sNe2k ne2k;

int main(void) {
	sPCIDevice nic;

	/* get NIC from pci */
	if(pci_getByClass(NIC_CLASS,NIC_SUBCLASS,&nic) < 0)
		error("Unable to find NIC (d:%d)",NIC_CLASS,NIC_SUBCLASS);

	if(nic.deviceId != NE2K_DEVICE_ID || nic.vendorId != NE2K_VENDOR_ID) {
		error("NIC is no NE2K (found %d.%d.%d: vendor=%hx, device=%hx)",
				nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);
	}

	printf("[NE2K] Found PCI-device %d.%d.%d: vendor=%hx, device=%hx\n",
			nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);
	fflush(stdout);

	init(&nic);
	return EXIT_SUCCESS;
}

static void writeReg(uint16_t reg,uint8_t value) {
	outByte(ne2k.basePort + reg,value);
}

static void init(sPCIDevice *nic) {
	size_t i;
	for(i = 0; i < 6; i++) {
		if(nic->bars[i].addr && nic->bars[i].type == 1) {
			if(requestIOPorts(nic->bars[i].addr,nic->bars[i].size) < 0) {
				error("Unable to request io-ports %d..%d",
						nic->bars[i].addr,nic->bars[i].addr + nic->bars[i].size - 1);
			}
			ne2k.basePort = nic->bars[i].addr;
		}
	}

	/* Program Command Register for Page 0 */
	writeReg(REG_CMD,CMD_STP | CMD_RD2);
	/* Initialize Data Configuration Register (DCR) */
	writeReg(REG_DCR,DCR_WTS | DCR_LS);
	/* Clear Remote Byte Count Registers (RBCR0 RBCR1) */
	writeReg(REG_RBCR0,0);
	writeReg(REG_RBCR1,0);
	/* Initialize Receive Configuration Register (RCR) */
	writeReg(REG_RCR,RCR_MON);
	/* Place the NIC in LOOPBACK mode 1 or 2 (Transmit Configuration Register e 02H or 04H) */
	writeReg(REG_TCR,TCR_LB0);
	/* Initialize Receive Buffer Ring Boundary Pointer, Page Start and Page Stop */
	writeReg(REG_PSTART,PAGE_RX);
	writeReg(REG_BNRY,PAGE_RX);
	writeReg(REG_PSTOP,PAGE_STOP);
	/* Clear Interrupt Status Register (ISR) by writing 0FFh to */
	writeReg(REG_ISR,0xFF);
	/* Initialize Interrupt Mask Register (IMR) */
	writeReg(REG_IMR,0);
	/* Program Command Register for page 1 (Command Register e 61H)
	 * i)	Initialize Physical Address Registers (PAR0-PAR5)
	 * ii)	Initialize Multicast Address Registers (MAR0-MAR7)
	 * iii)	Initialize CURRent pointer */
	/* select page 1 */
	writeReg(REG_CMD,CMD_STP | CMD_RD2 | CMD_PS0);
	writeReg(REG_CURR,PAGE_RX + 1);
	/* Put NIC in START mode (Command Register e 22H). The local receive DMA is still not active
	 * since the NIC is in LOOPBACK */
	writeReg(REG_CMD,CMD_STA | CMD_RD2);
	/* Initialize the Transmit Configuration for the intended value. The NIC is now ready for
	 * transmission and reception */

	/* accept runt and multicast */
	writeReg(REG_RCR,RCR_AR | RCR_AM);
	writeReg(REG_TCR,0);

	/* enable interrupts */
	writeReg(REG_ISR,0xFF);
	writeReg(REG_IMR,0x3F);
}
