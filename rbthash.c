#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/rbtree.h>
#include "ouichefs.h"

static struct rb_root rbtree = RB_ROOT;

struct rbt_node* hb_search(struct hash_sha256* hash)
{
	struct rbt_node* data;
	struct rb_node* node = rbtree.rb_node;
	while (node) {
		// container_of : ptr, type, member
		data = container_of(node, struct rbt_node, node);

		switch(hash_sha256_cmp(hash,&data->hash)) {
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

int hb_insert(struct rbt_node *data)
{
	struct rb_node** new = &(rbtree.rb_node);
	struct rb_node* parent = NULL;

	while (*new) {
		struct rbt_node *this = container_of(*new, struct rbt_node, node);
		switch(hash_sha256_cmp(&data->hash,&this->hash)) {
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
	rb_insert_color(&data->node, &rbtree);
	return 1;
}
