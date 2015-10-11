#ifndef __REDIS_LIB_H
#define __REDIS_LIB_H

#include "../deps/hiredis/hiredis.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#define ASSET_HASH4(ip) ((ip) % BUCKET_SIZE)
#define BUCKET_SIZE  31337

typedef enum {
	NO_CONSISTENCY = 1,
	EVENTUAL_CONSISTENCY = 2,
	SEQUENTIAL_CONSISTENCY = 4,
	ASYNC = 8
}consistency_type;

typedef int (*get_key_val)(void *, char **);
typedef int (*put_key_val) (char *, void *);
typedef int (*get_delta) (void *, void *);
typedef int (*eventual_con) (void *, void *);
typedef uint32_t (*key_hash) (void *key);

typedef struct meta_data_t {
	uint64_t vnf_id;
	uint64_t version;
	uint64_t lock;
} meta_data;

typedef struct item_t {
  	struct item_t *next;
  	struct item_t *prev;
  	uint64_t cas;
  	void*    key;
  	size_t   nkey;
  	void*    data;
  	meta_data*    mdata;
  	size_t   size;
	void*    temp_data;
	meta_data* temp_mdata;
  	uint32_t flags;
  	time_t   exp;
  	pthread_mutex_t mutex;
} item;

typedef struct redis_client_t {
	redisContext 	*context;
	item 		*passet[BUCKET_SIZE];
	char 		*key_type;
	size_t		 key_size;
	get_key_val	 get;
	put_key_val	 put;
	key_hash         hash;
	eventual_con     ev_con;
	get_delta        delta;
	uint32_t	 flags;
	uint32_t	 time;
	uint32_t 	 vnf_id;
	// hash
} redis_client;

redis_client client;

// Policy should be update with this cache
redis_client *create_cache(char *host, int port, uint32_t vnf_id, consistency_type con, int time);

void destroy_cache(redis_client *client);

// create item, hash item and update local cache with the data 
int create_item(void* key, size_t nkey, void **data,
		  size_t size, uint32_t flags, time_t exp);

int free_item(void* key, size_t nkey);

// returns the client context after setting up the connection
redisContext *createClient(char *host, int port);

// Syncronous Get and Set methods.
int redis_syncSet(redisContext *c, char *key, int key_len, char *value, int value_len);
redisReply* redis_syncGet(redisContext *c, char *key, size_t key_len);

// Syncronous Get and Set methods.
int redis_asyncSet(char *key, int key_len, char *value, int value_len);
int redis_asyncGet(char *key, int key_len, char *value, int *value_len);

int destroyClient(redisContext *context);

int register_encode_decode(get_key_val, put_key_val, key_hash, eventual_con, get_delta);

#endif
