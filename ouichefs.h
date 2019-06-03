/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018 Redha Gouicem <redha.gouicem@lip6.fr>
 */

#ifndef _OUICHEFS_H
#define _OUICHEFS_H

#define OUICHEFS_MAGIC  0x48434957

#define OUICHEFS_SB_BLOCK_NR     0

#define OUICHEFS_BLOCK_SIZE       (1 << 12)  /* 4 KiB */
#define OUICHEFS_MAX_FILESIZE     (1 << 22)  /* 4 MiB */
#define OUICHEFS_FILENAME_LEN            28
#define OUICHEFS_MAX_SUBFILES           128

/*
 * CowFS partition layout
 *
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  inode store  |  sb->nr_istore_blocks blocks
 * +---------------+
 * | ifree bitmap  |  sb->nr_ifree_blocks blocks
 * +---------------+
 * | bfree bitmap  |  sb->nr_bfree_blocks blocks
 * +---------------+
 * |    data       |
 * |      blocks   |  rest of the blocks
 * +---------------+
 *
 */

struct ouichefs_inode {
	uint32_t i_mode;	/* File mode */
	uint32_t i_uid;         /* Owner id */
	uint32_t i_gid;		/* Group id */
	uint32_t i_size;	/* Size in bytes */
	uint32_t i_ctime;	/* Inode change time */
	uint32_t i_atime;	/* Access time */
	uint32_t i_mtime;	/* Modification time */
	uint32_t i_blocks;	/* Block count */
	uint32_t i_nlink;	/* Hard links count */
	uint32_t index_block;	/* Block with list of blocks for this file */
};

struct ouichefs_inode_info {
	uint32_t index_block;
	struct inode vfs_inode;
};

#define OUICHEFS_INODES_PER_BLOCK \
	(OUICHEFS_BLOCK_SIZE / sizeof(struct ouichefs_inode))


struct ouichefs_sb_info {
	uint32_t magic;	        /* Magic number */

	uint32_t nr_blocks;      /* Total number of blocks (incl sb & inodes) */
	uint32_t nr_inodes;      /* Total number of inodes */

	uint32_t nr_istore_blocks;/* Number of inode store blocks */
	uint32_t nr_ifree_blocks; /* Number of inode free bitmap blocks */
	uint32_t nr_bfree_blocks; /* Number of block free bitmap blocks */

	uint32_t nr_free_inodes;  /* Number of free inodes */
	uint32_t nr_free_blocks;  /* Number of free blocks */

	uint32_t nr_refcount_blocks;

	unsigned long *ifree_bitmap; /* In-memory free inodes bitmap */
	unsigned long *bfree_bitmap; /* In-memory free blocks bitmap */

	u8* b_refcount; /* In-memory block refcounts */
};

struct ouichefs_file_index_block {
	uint32_t blocks[OUICHEFS_BLOCK_SIZE >> 2];
};

struct ouichefs_dir_block {
	struct ouichefs_file {
		uint32_t inode;
		char filename[OUICHEFS_FILENAME_LEN];
	} files[OUICHEFS_MAX_SUBFILES];
};

/* hash functions */
struct hash_sha256 {
	u32 p[8];
};
extern int hash_init(void);
extern void hash_exit(void);
extern int hash_compute(const char *data, unsigned int datalen, struct hash_sha256* hash);
extern int hash_cmp(struct hash_sha256* h1, struct hash_sha256* h2);
extern void hash_to_string(unsigned char* dest, struct hash_sha256* hash, unsigned hsize);

/* black red tree */
struct rbt_node {
      struct rb_node node;
      struct hash_sha256 hash;
      u32 blockid;

};
extern int hb_insert(struct rb_root* tree, struct rbt_node *data);
extern struct rbt_node* hb_search(struct rb_root* tree, struct hash_sha256* hash);
extern void hb_free(struct rb_root* tree);

/* dedup functions */
void dedup_umount(struct super_block* sb);

/* superblock functions */
int ouichefs_fill_super(struct super_block *sb, void *data, int silent);

/* inode functions */
int ouichefs_init_inode_cache(void);
void ouichefs_destroy_inode_cache(void);
struct inode *ouichefs_iget(struct super_block *sb, unsigned long ino);

/* file functions */
extern const struct file_operations ouichefs_file_ops;
extern const struct file_operations ouichefs_dir_ops;
extern const struct address_space_operations ouichefs_aops;

/* Getters for superbock and inode */
#define OUICHEFS_SB(sb) (sb->s_fs_info)
#define OUICHEFS_INODE(inode) (container_of(inode, struct ouichefs_inode_info, \
					    vfs_inode))

#endif	/* _OUICHEFS_H */
