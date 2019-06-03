#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include "ouichefs.h"

struct rbt_node* hb_search(struct rb_root* tree, struct hash_sha256* hash)
{
	struct rbt_node* data;
	struct rb_node* node = tree->rb_node;
	while (node) {
		// container_of : ptr, type, member
		data = container_of(node, struct rbt_node, node);

		switch(hash_cmp(hash,&data->hash)) {
			case -1 :
				node = node->rb_left;
				break;
			case 1 :
				node = node->rb_right;
				break;
			default:
				return data;
		}
	}
	return NULL;
}

int hb_insert(struct rb_root* tree, struct rbt_node *data)
{
	struct rb_node** new = &(tree->rb_node);
	struct rb_node* parent = NULL;

	while (*new) {
		struct rbt_node *this = container_of(*new, struct rbt_node, node);
		switch(hash_cmp(&data->hash,&this->hash)) {
			case -1 :
				new = &((*new)->rb_left);
				break;
			case 1 :
				new = &((*new)->rb_right);
				break;
			default:
				return 0;
		}
	}

	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, tree);
	return 1;
}

void hb_free(struct rb_root* tree)
{
	struct rbt_node* pos;
	struct rbt_node* n;
	rbtree_postorder_for_each_entry_safe(pos, n, tree, node) {
		rb_erase(&pos->node, tree);
		kfree(pos);
	}
	kfree(tree);
}
