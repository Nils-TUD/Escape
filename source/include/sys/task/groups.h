/**
 * $Id$
 */

#ifndef GROUPS_H_
#define GROUPS_H_

#include <sys/common.h>

typedef struct {
	int refCount;
	size_t count;
	gid_t *groups;
} sProcGroups;

/**
 * Allocates a new group-descriptor with <groups> (<count> groups)
 *
 * @param count the number of groups
 * @param groups the groups-array (will be copied)
 * @return the new group-descriptor or NULL if failed
 */
sProcGroups *groups_alloc(size_t count,const gid_t *groups);

/**
 * Joins the given group, i.e. increases the references
 *
 * @param g the group (may be NULL)
 * @return the group
 */
sProcGroups *groups_join(sProcGroups *g);

/**
 * Checks whether the groups <g> contain <gid>.
 *
 * @param g the groups (may be NULL)
 * @param gid the group-id
 * @return true if so
 */
bool groups_contains(sProcGroups *g,gid_t gid);

/**
 * Leaves the given group, i.e. decreases the references. If refCount is 0, it will be free'd.
 *
 * @param g the group (may be NULL)
 */
void groups_leave(sProcGroups *g);

/**
 * Prints the given group-descriptor
 *
 * @param g the group-descriptor
 */
void groups_print(sProcGroups *g);

#endif /* GROUPS_H_ */
