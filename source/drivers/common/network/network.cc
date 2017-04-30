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

#include <esc/ipc/nicdevice.h>
#include <esc/proto/net.h>
#include <esc/proto/nic.h>
#include <esc/proto/pci.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/wait.h>
#include <sys/thread.h>
#include <stdlib.h>

struct NICDesc {
	uint16_t vendor;
	uint16_t dev;
	const char *driver;
	const char *model;
	const char *name;
};

static const NICDesc nics[] = {
	{0x8086,	0x0438,	"/sbin/e1000",	"dh8900cc",		"DH8900CC"},
	{0x8086,	0x043a,	"/sbin/e1000",	"dh8900cc-f",	"DH8900CC Fiber"},
	{0x8086,	0x043c,	"/sbin/e1000",	"dh8900cc-b",	"DH8900CC Backplane"},
	{0x8086,	0x0440,	"/sbin/e1000",	"dh8900cc-s",	"DH8900CC SFP"},
	{0x8086,	0x1000,	"/sbin/e1000",	"82542-f",		"82542 (Fiber)"},
	{0x8086,	0x1001,	"/sbin/e1000",	"82543gc-f",	"82543GC (Fiber)"},
	{0x8086,	0x1004,	"/sbin/e1000",	"82543gc",		"82543GC (Copper)"},
	{0x8086,	0x1008,	"/sbin/e1000",	"82544ei",		"82544EI (Copper)"},
	{0x8086,	0x1009,	"/sbin/e1000",	"82544ei-f",	"82544EI (Fiber)"},
	{0x8086,	0x100c,	"/sbin/e1000",	"82544gc",		"82544GC (Copper)"},
	{0x8086,	0x100d,	"/sbin/e1000",	"82544gc-l",	"82544GC (LOM)"},
	{0x8086,	0x100e,	"/sbin/e1000",	"82540em",		"82540EM"},			/* tested and works */
	{0x8086,	0x100f,	"/sbin/e1000",	"82545em",		"82545EM (Copper)"},
	{0x8086,	0x1010,	"/sbin/e1000",	"82546eb",		"82546EB (Copper)"},
	{0x8086,	0x1011,	"/sbin/e1000",	"82545em-f",	"82545EM (Fiber)"},
	{0x8086,	0x1012,	"/sbin/e1000",	"82546eb-f",	"82546EB (Fiber)"},
	{0x8086,	0x1013,	"/sbin/e1000",	"82541ei",		"82541EI"},
	{0x8086,	0x1014,	"/sbin/e1000",	"82541er",		"82541ER"},
	{0x8086,	0x1015,	"/sbin/e1000",	"82540em-l",	"82540EM (LOM)"},
	{0x8086,	0x1016,	"/sbin/e1000",	"82540ep-m",	"82540EP (Mobile)"},
	{0x8086,	0x1017,	"/sbin/e1000",	"82540ep",		"82540EP"},
	{0x8086,	0x1018,	"/sbin/e1000",	"82541ei",		"82541EI"},
	{0x8086,	0x1019,	"/sbin/e1000",	"82547ei",		"82547EI"},
	{0x8086,	0x101a,	"/sbin/e1000",	"82547ei-m",	"82547EI (Mobile)"},
	{0x8086,	0x101d,	"/sbin/e1000",	"82546eb",		"82546EB"},
	{0x8086,	0x101e,	"/sbin/e1000",	"82540ep-m",	"82540EP (Mobile)"},
	{0x8086,	0x1026,	"/sbin/e1000",	"82545gm",		"82545GM"},
	{0x8086,	0x1027,	"/sbin/e1000",	"82545gm-1",	"82545GM"},
	{0x8086,	0x1028,	"/sbin/e1000",	"82545gm-2",	"82545GM"},
	{0x8086,	0x1049,	"/sbin/e1000",	"82566mm",		"82566MM"},
	{0x8086,	0x104a,	"/sbin/e1000",	"82566dm",		"82566DM"},
	{0x8086,	0x104b,	"/sbin/e1000",	"82566dc",		"82566DC"},
	{0x8086,	0x104c,	"/sbin/e1000",	"82562v",		"82562V 10/100"},
	{0x8086,	0x104d,	"/sbin/e1000",	"82566mc",		"82566MC"},
	{0x8086,	0x105e,	"/sbin/e1000",	"82571eb",		"82571EB"},
	{0x8086,	0x105f,	"/sbin/e1000",	"82571eb-1",	"82571EB"},
	{0x8086,	0x1060,	"/sbin/e1000",	"82571eb-2",	"82571EB"},
	{0x8086,	0x1075,	"/sbin/e1000",	"82547gi",		"82547GI"},
	{0x8086,	0x1076,	"/sbin/e1000",	"82541gi",		"82541GI"},
	{0x8086,	0x1077,	"/sbin/e1000",	"82541gi-1",	"82541GI"},
	{0x8086,	0x1078,	"/sbin/e1000",	"82541er",		"82541ER"},
	{0x8086,	0x1079,	"/sbin/e1000",	"82546gb",		"82546GB"},
	{0x8086,	0x107a,	"/sbin/e1000",	"82546gb-1",	"82546GB"},
	{0x8086,	0x107b,	"/sbin/e1000",	"82546gb-2",	"82546GB"},
	{0x8086,	0x107c,	"/sbin/e1000",	"82541pi",		"82541PI"},
	{0x8086,	0x107d,	"/sbin/e1000",	"82572ei",		"82572EI (Copper)"},
	{0x8086,	0x107e,	"/sbin/e1000",	"82572ei-f",	"82572EI (Fiber)"},
	{0x8086,	0x107f,	"/sbin/e1000",	"82572ei",		"82572EI"},
	{0x8086,	0x108a,	"/sbin/e1000",	"82546gb-3",	"82546GB"},
	{0x8086,	0x108b,	"/sbin/e1000",	"82573v",		"82573V (Copper)"},
	{0x8086,	0x108c,	"/sbin/e1000",	"82573e",		"82573E (Copper)"},
	{0x8086,	0x1096,	"/sbin/e1000",	"80003es2lan",	"80003ES2LAN (Copper)"},
	{0x8086,	0x1098,	"/sbin/e1000",	"80003es2lan-s","80003ES2LAN (Serdes)"},
	{0x8086,	0x1099,	"/sbin/e1000",	"82546gb-4",	"82546GB (Copper)"},
	{0x8086,	0x109a,	"/sbin/e1000",	"82573l",		"82573L"},
	{0x8086,	0x10a4,	"/sbin/e1000",	"82571eb",		"82571EB"},
	{0x8086,	0x10a5,	"/sbin/e1000",	"82571eb",		"82571EB (Fiber)"},
	{0x8086,	0x10a7,	"/sbin/e1000",	"82575eb",		"82575EB"},
	{0x8086,	0x10a9,	"/sbin/e1000",	"82575eb",		"82575EB Backplane"},
	{0x8086,	0x10b5,	"/sbin/e1000",	"82546gb",		"82546GB (Copper)"},
	{0x8086,	0x10b9,	"/sbin/e1000",	"82572ei",		"82572EI (Copper)"},
	{0x8086,	0x10ba,	"/sbin/e1000",	"80003es2lan",	"80003ES2LAN (Copper)"},
	{0x8086,	0x10bb,	"/sbin/e1000",	"80003es2lan",	"80003ES2LAN (Serdes)"},
	{0x8086,	0x10bc,	"/sbin/e1000",	"82571eb",		"82571EB (Copper)"},
	{0x8086,	0x10bd,	"/sbin/e1000",	"82566dm-2",	"82566DM-2"},
	{0x8086,	0x10bf,	"/sbin/e1000",	"82567lf",		"82567LF"},
	{0x8086,	0x10c0,	"/sbin/e1000",	"82562v-2",		"82562V-2 10/100"},
	{0x8086,	0x10c2,	"/sbin/e1000",	"82562g-2",		"82562G-2 10/100"},
	{0x8086,	0x10c3,	"/sbin/e1000",	"82562gt-2",	"82562GT-2 10/100"},
	{0x8086,	0x10c4,	"/sbin/e1000",	"82562gt",		"82562GT 10/100"},
	{0x8086,	0x10c5,	"/sbin/e1000",	"82562g",		"82562G 10/100"},
	{0x8086,	0x10c9,	"/sbin/e1000",	"82576",		"82576"},
	{0x8086,	0x10cb,	"/sbin/e1000",	"82567v",		"82567V"},
	{0x8086,	0x10cc,	"/sbin/e1000",	"82567lm-2",	"82567LM-2"},
	{0x8086,	0x10cd,	"/sbin/e1000",	"82567lf-2",	"82567LF-2"},
	{0x8086,	0x10ce,	"/sbin/e1000",	"82567v-2",		"82567V-2"},
	{0x8086,	0x10d3,	"/sbin/e1000",	"82574l",		"82574L"},
	{0x8086,	0x10d5,	"/sbin/e1000",	"82571pt",		"82571PT PT Quad"},
	{0x8086,	0x10d6,	"/sbin/e1000",	"82575gb",		"82575GB"},
	{0x8086,	0x10d9,	"/sbin/e1000",	"82571eb-d",	"82571EB Dual Mezzanine"},
	{0x8086,	0x10da,	"/sbin/e1000",	"82571eb-q",	"82571EB Quad Mezzanine"},
	{0x8086,	0x10de,	"/sbin/e1000",	"82567lm-3",	"82567LM-3"},
	{0x8086,	0x10df,	"/sbin/e1000",	"82567lf-3",	"82567LF-3"},
	{0x8086,	0x10e5,	"/sbin/e1000",	"82567lm-4",	"82567LM-4"},
	{0x8086,	0x10e6,	"/sbin/e1000",	"82576",		"82576"},
	{0x8086,	0x10e7,	"/sbin/e1000",	"82576-2",		"82576"},
	{0x8086,	0x10e8,	"/sbin/e1000",	"82576-3",		"82576"},
	{0x8086,	0x10ea,	"/sbin/e1000",	"82577lm",		"82577LM"},
	{0x8086,	0x10eb,	"/sbin/e1000",	"82577lc",		"82577LC"},
	{0x8086,	0x10ef,	"/sbin/e1000",	"82578dm",		"82578DM"},
	{0x8086,	0x10f0,	"/sbin/e1000",	"82578dc",		"82578DC"},
	{0x8086,	0x10f5,	"/sbin/e1000",	"82567lm",		"82567LM"},
	{0x8086,	0x10f6,	"/sbin/e1000",	"82574l",		"82574L"},
	{0x8086,	0x1501,	"/sbin/e1000",	"82567v-3",		"82567V-3"},
	{0x8086,	0x1502,	"/sbin/e1000",	"82579lm",		"82579LM"},
	{0x8086,	0x1503,	"/sbin/e1000",	"82579v",		"82579V"},
	{0x8086,	0x150a,	"/sbin/e1000",	"82576ns",		"82576NS"},
	{0x8086,	0x150c,	"/sbin/e1000",	"82583v",		"82583V"},
	{0x8086,	0x150d,	"/sbin/e1000",	"82576-4",		"82576 Backplane"},
	{0x8086,	0x150e,	"/sbin/e1000",	"82580",		"82580"},
	{0x8086,	0x150f,	"/sbin/e1000",	"82580-f",		"82580 Fiber"},
	{0x8086,	0x1510,	"/sbin/e1000",	"82580-b",		"82580 Backplane"},
	{0x8086,	0x1511,	"/sbin/e1000",	"82580-s",		"82580 SFP"},
	{0x8086,	0x1516,	"/sbin/e1000",	"82580-2",		"82580"},
	{0x8086,	0x1518,	"/sbin/e1000",	"82576ns",		"82576NS SerDes"},
	{0x8086,	0x1521,	"/sbin/e1000",	"i350",			"I350"},
	{0x8086,	0x1522,	"/sbin/e1000",	"i350-f",		"I350 Fiber"},
	{0x8086,	0x1523,	"/sbin/e1000",	"i350-b",		"I350 Backplane"},
	{0x8086,	0x1524,	"/sbin/e1000",	"i350-2",		"I350"},
	{0x8086,	0x1525,	"/sbin/e1000",	"82567v-4",		"82567V-4"},
	{0x8086,	0x1526,	"/sbin/e1000",	"82576-5",		"82576"},
	{0x8086,	0x1527,	"/sbin/e1000",	"82580-f2",		"82580 Fiber"},
	{0x8086,	0x1533,	"/sbin/e1000",	"i210",			"I210"},
	{0x8086,	0x153b,	"/sbin/e1000",	"i217",			"I217"},
	{0x8086,	0x155a,	"/sbin/e1000",	"i218",			"I218"},			/* tested and works */
	{0x8086,	0x294c,	"/sbin/e1000",	"82566dc-2",	"82566DC-2"},
	{0x8086,	0x2e6e,	"/sbin/e1000",	"cemedia",		"CE Media Processor"},

	{0x10ec,	0x8029,	"/sbin/ne2k",	"ne2k",			"NE2000"},
};

static const int TIMEOUT = 2000; /* ms */

struct NICProc {
	explicit NICProc(int _pid,const std::string &_bdf,const std::string &_driver,const std::string &_link)
		: pid(_pid), bdf(_bdf), driver(_driver), link(_link) {
	}

	int pid;
	std::string bdf;
	std::string driver;
	std::string link;
};

static std::vector<NICProc*> procs;
static volatile bool run = true;

static void start(esc::Net &net,const std::string &bdf,const std::string &driver,const std::string &link) {
	int pid = fork();
	if(pid < 0) {
		printe("fork failed");
		return;
	}
	if(pid == 0) {
		const char *args[] = {driver.c_str(),bdf.c_str(),link.c_str(),NULL};
		execv(args[0],args);
		error("exec failed");
	}

	// register it now even in case it fails below. this allows us to retry it upon termination
	procs.push_back(new NICProc(pid,bdf,driver,link));

	// wait a bit for the NIC driver to register the device
	int res;
	uint duration = 0;
	while(duration < TIMEOUT && (res = open(link.c_str(),O_NOCHAN)) < 0) {
		usleep(20 * 1000);
		duration += 20;
		// we want to show the error from open, not sleep
		errno = res;
	}
	if(res < 0) {
		printe("open");
		return;
	}
	close(res);

	const char *linkname = strchr(link.c_str() + 1,'/') + 1;
	print("Registering link %s",linkname);
	net.linkAdd(linkname,link.c_str());

	// configure lo statically
	if(link == "/dev/lo") {
		net.linkConfig("lo",esc::Net::IPv4Addr(127,0,0,1),
			esc::Net::IPv4Addr(255,0,0,0),esc::Net::Status::UP);
		net.routeAdd("lo",esc::Net::IPv4Addr(127,0,0,1),
			esc::Net::IPv4Addr(0,0,0,0),esc::Net::IPv4Addr(255,0,0,0));
	}
}

static void sighdl(int) {
	run = false;
}

int main() {
	esc::Net net("/dev/tcpip");

	if(signal(SIGTERM,sighdl) == SIG_ERR || signal(SIGINT,sighdl) == SIG_ERR)
		error("Unable to set signal handler");

	// create loopback device
	start(net,"0","/sbin/lo","/dev/lo");

	// eco32 and mmix do not have a PCI bus
#if defined(__x86__)
	{
		// find NICs on PCI bus
		esc::PCI pci("/dev/pci");
		for(int no = 0; ; ++no) {
			esc::PCI::Device nic;
			if(pci.tryByClass(nic,esc::NIC::PCI_CLASS,esc::NIC::PCI_SUBCLASS,no) < 0)
				break;

			for(size_t i = 0; i < ARRAY_SIZE(nics); ++i) {
				if(nic.deviceId == nics[i].dev && nic.vendorId == nics[i].vendor) {
					print("Found PCI-device %d.%d.%d: vendor=%hx, device=%hx model=%s name=%s",
							nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId,nics[i].model,nics[i].name);

					esc::OStringStream link,bdf;
					link << "/dev/enp" << nic.bus << "s" << nic.dev;
					bdf << nic.bus << "." << nic.dev << "." << nic.func;
					start(net,bdf.str(),nics[i].driver,link.str());
				}
			}
		}
	}
#endif

	while(run) {
		sExitState st;
		if(waitchild(&st,-1,0) == 0) {
			for(auto p = procs.begin(); p != procs.end(); ++p) {
				NICProc *np = *p;
				if(np->pid == st.pid) {
					print("NIC driver %d:%s died with exitcode %d (signal %d)",
						np->pid,np->driver.c_str(),st.exitCode,st.signal);

					try {
						net.linkRem(strchr(np->link.c_str() + 1,'/') + 1);
					}
					catch(const std::exception &e) {
						print(e.what());
					}
					start(net,np->bdf,np->driver,np->link);
					procs.erase(p);
					delete np;
					break;
				}
			}
		}
	}

	// kill all drivers and unregister their links
	for(auto p = procs.begin(); p != procs.end(); ++p) {
		NICProc *np = *p;
		if(kill(np->pid,SIGTERM) < 0)
			printe("Killing child %d failed",np->pid);
		if(waitchild(NULL,np->pid,0) < 0)
			printe("Waiting for child %d failed",np->pid);

		try {
			net.linkRem(strchr(np->link.c_str() + 1,'/') + 1);
		}
		catch(const std::exception &e) {
			print(e.what());
		}
	}
	return EXIT_FAILURE;
}
