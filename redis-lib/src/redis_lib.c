#include "redis_lib.h"

pthread_t thread_id;

static void *thread_start(void *arg)
{
	struct thread_info *tinfo = arg;
        char *uargv, *p;
	int i;
	item *it;

	sleep(5);

	while (1) {
		sleep(1);
		
		for (i=0; i<BUCKET_SIZE; i++) {
			it = client.passet[i];
			while ((it) && (it->key != NULL)) {
				pthread_mutex_lock(&it->mutex);
				redis_syncSet(client.context,
				              it->key,
					      it->nkey,
					      it->data,
					      it->size);
				pthread_mutex_unlock(&it->mutex);
				it = it->next;
			}
		}
	}

}

redis_client *create_cache(char *host, int port) {
	int i;

	client.context = createClient(host, port);

	if (NULL == client.context) {
		printf("No connection to server \n");	
		return NULL;
	}

	for (i=0; i<BUCKET_SIZE; i++) {
		client.passet[i] = malloc(sizeof(item));
		if (client.passet[i]) {
			memset(client.passet[i], 0, sizeof(item));
			pthread_mutex_init(&client.passet[i]->mutex, NULL);
		}
	}
	pthread_create(&thread_id, NULL, &thread_start, NULL);
	return &client;
}
// returns the client context after setting up the connection
redisContext *createClient(char *host, int port) {
	redisContext *c;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds

	if (host == NULL) {
		return 0;
	}

	c = redisConnectWithTimeout(host, port, timeout);
	if (c == NULL || c->err) {
		if (c) {
			printf("Connection error: %s\n", c->errstr);
			redisFree(c);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		return 0;
	}
	return c;
}

int create_item(const void* key, size_t nkey, void *data,
                size_t size, uint32_t flags, time_t exp) {
	uint64_t *hash = (uint64_t *) key;
	item *it;
	// we know the key is uint64_t, so just cast it to uint64_t
	// this has to be modified to generalize things


	if (client.passet[(int)*hash]->key) {
		printf("some one already in this location\n");
		it = malloc(sizeof(item));
		memset(it, 0, sizeof(item));
		it->next = client.passet[(int)*hash];
		client.passet[(int)*hash] = it;
	} else {
		it = client.passet[(int) *hash];
	}

	pthread_mutex_lock(&client.passet[(int) *hash]->mutex);	

	client.passet[(int) *hash]->key = (char *) malloc(sizeof(uint64_t));

	if (!client.passet[(int) *hash]->key) {
		printf("no space for data\n");
		return 0;
	}

	memcpy(client.passet[(int) *hash]->key, hash, sizeof(uint64_t));
	client.passet[(int) *hash]->nkey = nkey;

	client.passet[(int) *hash]->data = (char *) malloc(size);
	client.passet[(int) *hash]->size = size;
	data = client.passet[(int) *hash]->data;

	// If data is available /* set it */	
	redis_syncGet(client.context, client.passet[(int) *hash]->key, nkey,
		                      client.passet[(int) *hash]->data,
				      &client.passet[(int) *hash]->size);

	// if the data is present in key-value store, update the cache.
	
	pthread_mutex_unlock(&client.passet[(int) *hash]->mutex);	
	return 1;
}

int free_item(const void* key, size_t nkey) {
	uint64_t *hash = (uint64_t *) key;

	pthread_mutex_lock(&client.passet[(int) *hash]->mutex);

	free(client.passet[(int) *hash]->key);
	client.passet[(int) *hash]->key = NULL;

	free(client.passet[(int) *hash]->data);
	client.passet[(int) *hash]->data = NULL;
	
	pthread_mutex_unlock(&client.passet[(int) *hash]->mutex);
	return 1;
}

// Syncronous Get and Set methods. return 0 on failure, > 0 success.
int redis_syncSet(redisContext *c, char *key, int key_len, char *value, int value_len) {
	redisReply *reply;
	
	if ((!key) || (!key_len) || (!value) || (!value_len)) {
		return 0;
	}
	
	reply = redisCommand(c, "SET %b %b", key, (size_t) key_len, value, (size_t) value_len);
	printf("SET: %s\n", reply->str);
	freeReplyObject(reply);

	return 1;
}

int redis_syncGet(redisContext *c, char *key, size_t key_len, char *value, size_t *value_len) {
	redisReply *reply;
	
	if ((!key) || (!key_len) || (!value) || (!value_len)) {
		return 0;
	}

	reply = redisCommand(c,"GET %b", key, (size_t) key_len);
	if (reply->type != REDIS_REPLY_NIL) {
		strncpy(value, reply->str, *value_len);
		*value_len = strlen(reply->str);
		printf("Got Value: %s\n", reply->str);
		freeReplyObject(reply);
		return 0;
	}

	printf("No data available for key\n");
	freeReplyObject(reply);

	return 1;
}

// Syncronous Get and Set methods. return 0 on failure, > 0 success.
int redis_asyncSet(char *key, int key_len, char *value, int value_len) {
	return 0;
}

int redis_asyncGet(char *key, int key_len, char *value, int *value_len) {
	return 0;
}

int destroyClient(redisContext *context) {
	return 0;
}
