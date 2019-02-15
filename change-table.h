#ifndef CHANGE_TABLE_H
#define CHANGE_TABLE_H

#include "oidmap.h"

struct commit;
struct ref_filter;

/*
 * This struct holds a list of change refs. The first element is stored inline,
 * to optimize for small lists.
 */
struct change_list {
	/* Ref name for the first change in the list, or null if none.
	 *
	 * This field is private. Use for_each_change_in to read.
	 */
	const char* first_refname;
	/* List of additional change refs. Note that this is empty if the list
	 * contains 0 or 1 elements.
	 *
	 * This field is private. Use for_each_change_in to read.
	 */
	struct string_list additional_refnames;
};

/*
 * Holds information about the head of a single change.
 */
struct change_head {
	/*
	 * The location pointed to by the head of the change. May be a commit or a
	 * metacommit.
	 */
	struct object_id head;
	/*
	 * The content commit for the latest commit in the change. Always points to a
	 * real commit, never a metacommit.
	 */
	struct object_id content;
	/*
	 * Abandoned: indicates that the content commit should be removed from the
	 * history.
	 *
	 * Hidden: indicates that the change is an inactive change from the
	 * hiddenmetas namespace. Such changes will be hidden from the user by
	 * default.
	 *
	 * Deleted: indicates that the change has been removed from the repository.
	 * That is the ref was deleted since the time this struct was created. Such
	 * entries should be ignored.
	 */
	unsigned int abandoned:1,
		hidden:1,
		remote:1,
		deleted:1;
};

/*
 * Holds the list of change refs whose content points to a particular content
 * commit.
 */
struct commit_change_list_entry {
	struct oidmap_entry entry;
	struct change_list changes;
};

/*
 * Holds information about the heads of each change, and permits effecient
 * lookup from a commit to the changes that reference it directly.
 *
 * All fields should be considered private. Use the change_table functions
 * to interact with this struct.
 */
struct change_table {
	/**
	 * Memory pool for the objects allocated by the change table.
	 */
	struct mem_pool *memory_pool;
	/* Map object_id to commit_change_list_entry structs. */
	struct oidmap oid_to_metadata_index;
	/* List of ref names. The util value points to a change_head structure
	 * allocated from memory_pool.
	 */
	struct string_list refname_to_change_head;
};

extern void change_table_init(struct change_table *to_initialize);
extern void change_table_clear(struct change_table *to_clear);

/* Adds the given change head to the change_table struct */
extern void change_table_add(struct change_table *to_modify,
	const char *refname, struct commit *target);

/* Adds the non-hidden local changes to the given change_table struct.
 */
extern void change_table_add_all_visible(struct change_table *to_modify,
	struct repository *repo);

/*
 * Adds all changes matching the given ref filter to the given change_table
 * struct.
 */
extern void change_table_add_matching_filter(struct change_table *to_modify,
	struct repository* repo, struct ref_filter *filter);

typedef int each_change_fn(const char *refname, void *cb_data);

extern int change_table_has_change_referencing(struct change_table *changes,
	const struct object_id *referenced_commit_id);

/* Iterates over all changes that reference the given commit. For metacommits,
 * this is the list of changes that point directly to that metacommit.
 * For normal commits, this is the list of changes that have this commit as
 * their latest content.
 */
extern int for_each_change_referencing(struct change_table *heads,
	const struct object_id *referenced_commit_id, each_change_fn fn, void *cb_data);

/**
 * Returns the change head for the given refname. Returns NULL if no such change
 * exists.
 */
extern struct change_head* get_change_head(struct change_table *heads,
	const char* refname);

#endif
