#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/bitops.h>

#include "ouichefs.h"
#include "bitmap.h"

void dedup_umount(struct super_block* sb) {
	unsigned int bit;
	struct ouichefs_sb_info* sb_info;

	struct inode* file_inode;
	struct ouichefs_inode_info* inode_info;

	struct buffer_head* bh_index;
	struct buffer_head* bh_block;
	struct ouichefs_file_index_block *index;
	int i;

	struct rb_root* tree;
	struct rbt_node* tree_node_new;
	struct rbt_node* tree_node_found;

	tree = kmalloc(sizeof(struct rb_root), GFP_KERNEL);
	if(!tree) {
		pr_warn("[ouichefs] Error on tree creation, dedup aborted !\n");
		return;	
	}
	*tree = RB_ROOT;	
	
	if (!sb || IS_ERR(sb)) {
		pr_info("Superblock not found ! Dedup cancelled\n");
		return;
	}

	sb_info = OUICHEFS_SB(sb);
	if (!sb_info || IS_ERR(sb_info)) {
		pr_info("Superblock Info not found (fs broken ?) ! Dedup cancelled\n");
		return;
	}

	if(sync_filesystem(sb)<0) {
		pr_warn("[ouichefs] Error on sync_fs happened, dedup aborted !\n");
		return;
	}
	
	for_each_clear_bit(bit, sb_info->ifree_bitmap, sb_info->nr_inodes) {

		file_inode = ouichefs_iget(sb, bit);
		if (IS_ERR(file_inode)) {
			pr_info("[ouichefs] Error on inode read for inode %d.\n", bit);
			continue;
		}

		inode_info = (struct ouichefs_inode_info*)OUICHEFS_INODE(file_inode);
		bh_index = sb_bread(sb, inode_info->index_block);
		if (IS_ERR(bh_index)) {
			pr_info("[ouichefs] Error on index_block read for inode %d.\n", bit);
			continue;
		}

		index = (struct ouichefs_file_index_block *)bh_index->b_data;

		for(i=0; i < file_inode->i_blocks ; i++) {
			
			if(index->blocks[i]==0) {
				continue;
			}
			
			bh_block = sb_bread(sb, index->blocks[i]);
			if (IS_ERR(bh_block)) {
				pr_info("[ouichefs] Error reading block %d.\n", index->blocks[i]);
				continue;
			}

			// Compute block's hash
			tree_node_new = kmalloc(sizeof(struct rbt_node), GFP_KERNEL);
			if(!tree_node_new) {
				pr_info("[ouichefs] No memory\n");
				brelse(bh_block);// à revoir
				return;
			}
			
			tree_node_new->blockid = index->blocks[i];

			// Compute the hash of the block
			hash_compute(bh_block->b_data, OUICHEFS_BLOCK_SIZE, &tree_node_new->hash);

			tree_node_found = hb_search(tree, &tree_node_new->hash);
			if(tree_node_found) {
				if(tree_node_found->blockid == tree_node_new->blockid){
					goto relse_blk;
				}
				put_block(sb_info,index->blocks[i]);
				index->blocks[i] = tree_node_found->blockid;
				mark_buffer_dirty(bh_index);

				//sb_info->bref_count[tree_node_found->blockid] = ((u8)(sb_info->bref_count[tree_node_found->blockid]))+1;

				pr_info("[ouichefs] bloc dedup old=%d new=%d\n", tree_node_found->blockid, tree_node_new->blockid);
				kfree(tree_node_new);
			} else {
				hb_insert(tree, tree_node_new);
				pr_info("[ouichefs] bloc %d added into hash tree\n", tree_node_new->blockid);
			}
			
		relse_blk:
			brelse(bh_block);
		}

		brelse(bh_index);
		iput(file_inode);
	}

	hb_free(tree);
}
