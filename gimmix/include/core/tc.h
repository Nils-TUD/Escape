/**
 * $Id: tc.h 235 2011-06-16 08:37:02Z nasmussen $
 */

#ifndef TC_H_
#define TC_H_

#include "common.h"

// the available translation-caches
#define TC_INSTR		0
#define TC_DATA			1

// represents an entry in the TC
typedef struct {
	// 64  61                                 13+s-13       13          3   0
	// +-+--+---------------------------------------+--------+----------+---+
	// |0|i |                   v                   |   0    |     n    | 0 |
	// +-+--+---------------------------------------+--------+----------+---+
	octa key;

	// 38                                 3+s-13        3   0
	// +---------------------------------------+--------+---+
	// |                    a                  |   0    | p |
	// +---------------------------------------+--------+---+
	octa translation;
} sTCEntry;

/**
 * Initializes the ITC and DTC
 */
void tc_init(void);

/**
 * Resets the ITC and DTC, i.e. it removes all entries
 */
void tc_reset(void);

/**
 * Shuts the ITC and DTC down, i.e. free's memory
 */
void tc_shutdown(void);

/**
 * Returns the TC-entry at given index
 *
 * @param tc the tc: TC_INSTR or TC_DATA
 * @param index the index of the entry
 * @return the entry or NULL
 */
sTCEntry *tc_getByIndex(int tc,size_t index);

/**
 * Returns the TC-entry for given key.
 *
 * @param tc the tc: TC_INSTR or TC_DATA
 * @param key the key (not the address!)
 * @return the entry or NULL
 * @see tc_addrToKey
 */
sTCEntry *tc_getByKey(int tc,octa key);

/**
 * Removes all entries from given TC
 *
 * @param tc the tc: TC_INSTR or TC_DATA
 */
void tc_removeAll(int tc);

/**
 * Removes the given entry, i.e. it marks it invalid
 *
 * @param e the entry
 */
void tc_remove(sTCEntry *e);

/**
 * Puts the given key-translation-pair into the given tc
 *
 * @param tc the tc: TC_INSTR or TC_DATA
 * @param key the key (not the address!)
 * @param translation the translation
 * @return the created entry
 * @see tc_addrToKey
 * @see tc_pteToTrans
 */
sTCEntry *tc_set(int tc,octa key,octa translation);

/**
 * Translates the given PTE to a translation of an TC-entry
 *
 * @param pte the page-table-entry
 * @param s the page-size-shift from rV
 * @return the translation
 */
octa tc_pteToTrans(octa pte,int s);

/**
 * Translates the given address to a key of an TC-entry
 *
 * @param addr the virtual address
 * @param s the page-size-shift from rV
 * @param n the address-space-number from rV
 * @return the key
 */
octa tc_addrToKey(octa addr,int s,int n);

#endif /* TC_H_ */
