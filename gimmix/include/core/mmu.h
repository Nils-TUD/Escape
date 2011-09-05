/**
 * $Id: mmu.h 172 2011-04-15 09:41:38Z nasmussen $
 */

#ifndef MMU_H_
#define MMU_H_

#include "common.h"

// flags for accessing memory
#define MEM_SIDE_EFFECTS		(1 << 0)
#define MEM_UNCACHED			(1 << 1)
#define MEM_UNINITIALIZED		(1 << 2)
#define MEM_IGNORE_PROT_EX		(1 << 3)

// access-flags
#define MEM_READ				(1 << 2)
#define MEM_WRITE				(1 << 1)
#define MEM_EXEC				(1 << 0)

// convenience-struct that holds all information from rV
typedef struct {
	int b[5];	// segment sizes
	int s;		// page size = 2^s
	int n;		// process number
	octa r;		// root location
	int f;		// function (hw/sw)
} sVirtTransReg;

/**
 * Reads one instruction from given address. That means, it tries to access this address for
 * execution. It might use the ITC and IC.
 *
 * @param addr the virtual address
 * @param flags control the behaviour:
 * 	MEM_SIDE_EFFECTS: if disabled, the state of the machine doesn't change and events are not fired
 *  MEM_UNCACHED: if enabled, the data is not put into the cache if not already present
 *  MEM_IGNORE_PROT_EX: if enabled, protection-faults are ignored
 * @return the read tetra
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_PROT_EXEC if rV is invalid (and protection-faults are not ignored)
 * @throws TRAP_REPEAT|TRAP_PROT_EXEC if a PTP/PTE is invalid or the page is not executable
 *  (and protection-faults are not ignored)
 * @throws TRAP_SOFT_TRANS|TRAP_PROT_EXEC if <addr> is not in ITC and software-translation is set
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not readable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
tetra mmu_readInstr(octa addr,int flags);

/**
 * Reads one octa/tetra/wyde/byte from given address. That means, it tries to access this address
 * for reading. It might use the DTC and DC.
 *
 * @param addr the virtual address
 * @param flags control the behaviour:
 * 	MEM_SIDE_EFFECTS: if disabled, the state of the machine doesn't change and events are not fired
 *  MEM_UNCACHED: if enabled, the data is not put into the cache if not already present
 *  MEM_IGNORE_PROT_EX: if enabled, protection-faults are ignored
 * @return the read octa/tetra/wyde/byte
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_PROT_READ if rV is invalid (and protection-faults are not ignored)
 * @throws TRAP_REPEAT|TRAP_PROT_READ if a PTP/PTE is invalid or the page is not readable
 *  (and protection-faults are not ignored)
 * @throws TRAP_SOFT_TRANS|TRAP_PROT_READ if <addr> is not in DTC and software-translation is set
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not readable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
octa mmu_readOcta(octa addr,int flags);
tetra mmu_readTetra(octa addr,int flags);
wyde mmu_readWyde(octa addr,int flags);
byte mmu_readByte(octa addr,int flags);

/**
 * Writes one octa/tetra/wyde/byte to given address. The tetra-, wyde- and byte-versions will access
 * this address for reading AND writing. The octa-version for writing only. Note that the protection-
 * faults that are thrown contain the bits that are missing. So, if we have to read first and have
 * no read-permission, it will throw an TRAP_PROT_READ. If we have no write-permission either, it
 * will throw both.
 * It might use the DTC and DC.
 *
 * @param addr the virtual address
 * @param value the value to write
 * @param flags control the behaviour:
 * 	MEM_SIDE_EFFECTS: if disabled, the state of the machine doesn't change and events are not fired
 *  MEM_UNCACHED: if enabled, the data is not put into the cache if not already present
 *  MEM_UNINITIALIZED: if enabled, the data will not be fetched from the bus if not already present,
 *    but it will simply be zero'd
 *  MEM_IGNORE_PROT_EX: if enabled, protection-faults are ignored
 * @return the read octa/tetra/wyde/byte
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_PROT_READ|TRAP_PROT_WRITE if rV is invalid (and protection-faults are not ignored)
 * @throws TRAP_REPEAT|TRAP_PROT_READ|TRAP_PROT_WRITE if a PTP/PTE is invalid or the page is not
 *  readable (and protection-faults are not ignored)
 * @throws TRAP_SOFT_TRANS|TRAP_PROT_READ|TRAP_PROT_WRITE if <addr> is not in DTC and software-
 *   translation is set
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not writable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
void mmu_writeOcta(octa addr,octa value,int flags);
void mmu_writeTetra(octa addr,tetra value,int flags);
void mmu_writeWyde(octa addr,wyde value,int flags);
void mmu_writeByte(octa addr,byte value,int flags);

/**
 * If radical is disabled, it flushes the affected cache-block in the DC to memory.
 * If radical is enabled, it will additionally remove this cache-block from the DC.
 * Protection-faults are ignored here.
 *
 * @param addr the virtual address
 * @param radical whether the cache-block should also be removed
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_SOFT_TRANS|TRAP_PROT_WRITE if <addr> is not in DTC and software-translation is set
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not writable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
void mmu_syncdOcta(octa addr,bool radical);

/**
 * If radical is disabled, it synchronizes the DC with the IC. More precisely, it will remove the
 * affected cache-block from the IC and flush the affected cache-block in the DC.
 * If radical is enabled, it will remove the affected cache-block in IC and DC.
 * Protection-faults are ignored here.
 *
 * @param addr the virtual address
 * @param radical whether the cache-block should also be removed in DC
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_SOFT_TRANS|TRAP_PROT_EXEC if <addr> is not in ITC and software-translation is set
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not writable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
void mmu_syncidOcta(octa addr,bool radical);

/**
 * Unpacks the given rV and returns the parts
 *
 * @param rv the value of rV
 * @return the parts
 */
sVirtTransReg mmu_unpackrV(octa rv);

/**
 * Translates the given address for given mode.
 *
 * @param addr the virtual address
 * @param mode the mode with which to access the data (MEM_READ | MEM_WRITE | MEM_EXEC). this will
 * 	be used as exception-bits!
 * @param exceptions the required permissions (MEM_READ | MEM_WRITE | MEM_EXEC). this will be used
 *  to test whether an protection-fault should be thrown
 * @param sideEffects whether to cause side-effects (change state and fire events)
 * @return the physical address
 * @throws TRAP_PRIVILEGED_ACCESS if <addr> is privileged but we're not in privileged mode
 * @throws TRAP_SOFT_TRANS|<mode> if <addr> is not in DTC/ITC and software-translation is set
 * @throws <mode> if rV is invalid
 * @throws TRAP_REPEAT|<mode> if a PTP/PTE is invalid
 * @throws TRAP_REPEAT|<mode> if the page is not accessible for <exceptions>
 */
octa mmu_translate(octa addr,int mode,int exceptions,bool sideEffects);

#endif /* MMU_H_ */
