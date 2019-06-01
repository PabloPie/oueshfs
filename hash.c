#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/scatterlist.h>

#include "ouichefs.h"


struct sdesc {
    struct shash_desc shash;
    char ctx[];
};

static char* hash_algo_name = "sha256";
static unsigned hash_algo_size;
static struct sdesc* hash_sdesc = NULL;
static struct crypto_shash *hash_algo = NULL;

int hash_init(void)
{
	hash_algo = crypto_alloc_shash(hash_algo_name, 0, 0);
	if (IS_ERR(hash_algo)) {
		return PTR_ERR(hash_algo);
	}
	hash_algo_size = crypto_shash_descsize(hash_algo);

	hash_sdesc = kmalloc(sizeof(struct sdesc) + hash_algo_size, GFP_KERNEL);
	if (!hash_sdesc)
		return -ENOMEM;
	hash_sdesc->shash.tfm = hash_algo;
	hash_sdesc->shash.flags = 0x0;
	
	/*struct hash_sha256 test;
	char str[65];
	hash_compute_sha256("pablo", 5, &test);
	pr_info("%x%x%x%x%x%x%x%x\n", test.p[0], test.p[1], test.p[2], test.p[3], test.p[4], test.p[5], test.p[6], test.p[7]);
	hash_sha256_to_string(str,&test,64);
	pr_info("%s\n", str);*/

	return 0;
}

void hash_exit(void)
{
	if(hash_algo) {
		crypto_free_shash(hash_algo);
		hash_algo = NULL;
	}
	if(hash_sdesc) {
		kfree(hash_sdesc);
		hash_sdesc = NULL;
	}
}

int hash_compute_sha256(const char *data, unsigned int datalen, struct hash_sha256* hash)
{
	unsigned char digest[32];
	int ret = crypto_shash_digest(&hash_sdesc->shash, data, datalen, digest);
	if(ret)
		goto end;
	
	hash->p[0] = (digest[0] << 24) | (digest[1] << 16) 
				| (digest[2] << 8) | (digest[3]);
	hash->p[1] = (digest[4] << 24) | (digest[5] << 16) 
				| (digest[6] << 8) | (digest[7]);
	hash->p[2] = (digest[8] << 24) | (digest[9] << 16) 
				| (digest[10] << 8) | (digest[11]);
	hash->p[3] = (digest[12] << 24) | (digest[13] << 16) 
				| (digest[14] << 8) | (digest[15]);
	hash->p[4] = (digest[16] << 24) | (digest[17] << 16) 
				| (digest[18] << 8) | (digest[19]);
	hash->p[5] = (digest[20] << 24) | (digest[21] << 16) 
				| (digest[22] << 8) | (digest[23]);
	hash->p[6] = (digest[24] << 24) | (digest[25] << 16) 
				| (digest[26] << 8) | (digest[27]);
	hash->p[7] = (digest[28] << 24) | (digest[29] << 16) 
				| (digest[30] << 8) | (digest[31]);

	end:
	return ret;
}

int hash_sha256_cmp(struct hash_sha256* h1, struct hash_sha256* h2)
{
	int i;
	for(i=0;i<8;i++) {
		if(h1->p[i] < h2->p[i])
			return -1;
		else if(h1->p[i] > h2->p[i])
			return 1;
	}
	return 0;
}

void hash_sha256_to_string(unsigned char* dest, struct hash_sha256* hash, unsigned hsize)
{
	scnprintf(dest, hsize + 1, "%x%x%x%x%x%x%x%x\n", 
				hash->p[0], hash->p[1], hash->p[2], hash->p[3], 
				hash->p[4], hash->p[5], hash->p[6], hash->p[7]);
}
