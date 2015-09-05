#include "../deps/hiredis/hiredis.h"
#include <string.h>
#include <stdio.h>

#define ASSET_HASH4(ip) ((ip) % BUCKET_SIZE)
#define BUCKET_SIZE  31337

typedef struct item_t {
  uint8_t  in_use:1;
  uint64_t cas;
  void*    key;
  size_t   nkey;
  void*    data;
  size_t   size;
  uint32_t flags;
  time_t   exp;
}item;

typdef struct redis_client_t {
	redisContext *context;
	item *passet[BUCKET_SIZE];
	// hash
} redis_client;

redis_client client;

// Policy should be update with this cache
redis_client *create_cache(void);

void destroy_cache(redis_client *client);

// create item, hash item and update local cache with the data 
int create_item(const void* key, size_t nkey, void *data,
		  size_t size, uint32_t flags, time_t exp);

int free_item(const void*key);

// returns the client context after setting up the connection
redisContext *createClient(char *host, int port);

// Syncronous Get and Set methods.
int redis_syncSet(redisContext *c, char *key, int key_len, char *value, int value_len);
int redis_syncGet(redisContext *c, char *key, int key_len, char *value, int *value_len);

// Syncronous Get and Set methods.
int redis_asyncSet(char *key, int key_len, char *value, int value_len);
int redis_asyncGet(char *key, int key_len, char *value, int *value_len);

int destroyClient(redisContext *context);
