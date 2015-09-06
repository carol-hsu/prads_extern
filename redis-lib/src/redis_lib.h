#include "../deps/hiredis/hiredis.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#define ASSET_HASH4(ip) ((ip) % BUCKET_SIZE)
#define BUCKET_SIZE  31337

typedef struct item_t {
  uint64_t cas;
  pthread_mutex_t mutex;
  void*    key;
  size_t   nkey;
  void*    data;
  size_t   size;
  uint32_t flags;
  time_t   exp;
} item;

typedef struct redis_client_t {
	redisContext *context;
	item *passet[BUCKET_SIZE];
	// hash
} redis_client;

redis_client client;

// Policy should be update with this cache
redis_client *create_cache(char *host, int port);

void destroy_cache(redis_client *client);

// create item, hash item and update local cache with the data 
int create_item(const void* key, size_t nkey, void *data,
		  size_t size, uint32_t flags, time_t exp);

int free_item(const void* key, size_t nkey);

// returns the client context after setting up the connection
redisContext *createClient(char *host, int port);

// Syncronous Get and Set methods.
int redis_syncSet(redisContext *c, char *key, int key_len, char *value, int value_len);
int redis_syncGet(redisContext *c, char *key, int key_len, char *value, int *value_len);

// Syncronous Get and Set methods.
int redis_asyncSet(char *key, int key_len, char *value, int value_len);
int redis_asyncGet(char *key, int key_len, char *value, int *value_len);

int destroyClient(redisContext *context);
