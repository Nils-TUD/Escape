/**
 * $Id$
 */

#ifndef GROUP_H_
#define GROUP_H_

#include <esc/common.h>
#include <stdio.h>

#define GROUPS_PATH			"/etc/groups"
#define MAX_GROUPNAME_LEN	32

/* fixed because boot-modules can't use the fs and thus, can't read /etc/groups */
#define GROUP_STORAGE		10

typedef struct sGroup {
	gid_t gid;
	char name[MAX_GROUPNAME_LEN];
	size_t userCount;
	uid_t *users;
	struct sGroup *next;
} sGroup;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parses the group-information from given file into a linked list of groups.
 * Please free the memory with group_free() if you're done.
 *
 * @param file the file to read
 * @param count will be set to the number of groups in the returned list
 * @return the groups or NULL if failed
 */
sGroup *group_parseFromFile(const char *file,size_t *count);

/**
 * Parses the given group-information into a linked list of groups with their members.
 * Please free the memory with group_free() if you're done.
 *
 * @param groups the group-information (null-terminated)
 * @param count will be set to the number of groups in the returned list
 * @return the groups or NULL if failed
 */
sGroup *group_parse(const char *groups,size_t *count);

/**
 * Searches for the group with given name in the list denoted by <g> and returns the group-id
 *
 * @param g the group-list
 * @param name the group-name
 * @return the group-id or -1 if not found
 */
gid_t group_getByName(const sGroup *g,const char *name);

/**
 * Collects all groups for the given user
 *
 * @param g the group-list
 * @param uid the user-id
 * @param openSlots specifies how many slots should be left open in the groups-array
 * @param count will be set to the number of collected groups
 * @return the group-id-array or NULL if failed
 */
gid_t *group_collectGroupsFor(const sGroup *g,uid_t uid,size_t openSlots,size_t *count);

/**
 * Writes the given groups to the given file
 *
 * @param g the groups
 * @param f the file
 * @return 0 on success
 */
int group_write(const sGroup *g,FILE *f);

/**
 * Free's the given groups
 *
 * @param g the groups
 */
void group_free(sGroup *g);

/**
 * Prints the given groups
 *
 * @param g the groups
 */
void group_print(const sGroup *g);

#ifdef __cplusplus
}
#endif

#endif /* GROUP_H_ */
