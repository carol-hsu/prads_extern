#include "redis_lib.h"

pthread_t thread_id;

int register_encode_decode(get_key_val get, put_key_val put, key_hash hash) {
	client.get = get;
	client.put = put;
	client.hash = hash;
}

static void *thread_start(void *arg)
{
	struct thread_info *tinfo = arg;
        char *p=NULL;
	int i;
	item *it = NULL;
	redisReply *reply;
	size_t total_size, p_size;
	char m[24];

	sleep(5);

	while (1) {
		sleep(5);
		
		for (i=0; i<BUCKET_SIZE; i++) {
			it = client.passet[i];
			pthread_mutex_lock(&it->mutex);
			while ((it) && (it->key != NULL)) {
				reply = redis_syncGet(client.context,
					              it->key,
					              it->nkey);
				if (reply) {
					printf("Data Available \n");
					total_size = reply->len;
					p_size = total_size - sizeof(meta_data);
					p = ((char *) reply->str) + p_size;
						
					printf(" Data %s\n", reply->str);
					printf("get vnf_id %lld\n", (long long)*(p));
					printf("get version %lld\n", (long long)*(p+8));
					printf("get lock %lld\n", (long long)*(p+16));
					printf("set total length %zu\n", total_size);
					freeReplyObject(reply);
				} else {
					printf("Data not Available \n");
				}
				++it->mdata->version;
				client.get((void *) it->key, &p);
				if (p) {
					p_size = strlen(p);
					total_size = p_size + sizeof(meta_data);
					p = (char *) realloc(p, total_size);
					memcpy(m, (it->data + it->size), sizeof(meta_data));
					memcpy(p+p_size, m, sizeof(meta_data));

					printf("Set vnf_id %lld\n", (long long)*(p+p_size));
					printf("Set version %lld\n", (long long)*(p+p_size+8));
					printf("set lock %lld\n", (long long)*(p+p_size+16));
					printf("set total length %zu\n", total_size);
					redis_syncSet(client.context,
				        	      it->key,
					      	      it->nkey,
					              p,
					              total_size);
					free(p);
					p = NULL;
					printf("data updated \n");
				}
				// move to next item
				it = it->next;
			}
			pthread_mutex_unlock(&client.passet[i]->mutex);
		}
	}

}

redis_client *create_cache(char *host, int port, uint32_t vnf_id) {
	int i;

	client.vnf_id  = vnf_id;
	client.context = createClient(host, port);

	if (NULL == client.context) {
		printf("No connection to server \n");
		return NULL;
	}
	printf("Size of metadata %lu", sizeof(meta_data));

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

int create_item(void* key, size_t nkey, void **data,
                size_t size, uint32_t flags, time_t exp) {
	uint32_t hash = client.hash(key);
	item *it, *temp_next;
	redisReply *reply = NULL;
	int ret = 0;
	meta_data *mdata;

	*data = NULL;
	// we know the key is uint64_t, so just cast it to uint64_t
	// this has to be modified to generalize things

	pthread_mutex_lock(&client.passet[hash]->mutex);

	if (client.passet[hash]->key) {
		it = malloc(sizeof(item));
		if (!it) {
			printf("no space for data\n");
			return 0;	
		}
		memset(it, 0 , sizeof(item));
		pthread_mutex_init(&it->mutex, NULL);
		it->prev = client.passet[hash];
		temp_next = client.passet[hash]->next;
		client.passet[hash]->next = it;
		it->next = temp_next;
		if (temp_next) {
			temp_next->prev = it;
		}
	} else {
		it = client.passet[hash];
	}

	it->key = (char *) malloc(nkey);

	if (!it->key) {
		pthread_mutex_unlock(&it->mutex);
		printf("no space for data\n");
		return 0;
	}

	memcpy((char *) it->key, (char *) key, nkey);
	it->nkey = nkey;

	it->data = (char *) malloc(size + sizeof(meta_data));
	it->mdata = (meta_data *) ((char *)it->data + size);
	it->size = size;
	*data = it->data;
	mdata = it->mdata;

	mdata->vnf_id = client.vnf_id;
	mdata->version = 0;
	mdata->lock = 0;

	// if the data is present in key-value store, update the cache.
	reply = redis_syncGet(client.context, (char *) it->key, nkey);
	if (reply) {
		client.put(reply->str, (void *) it->data);	
		ret = 1;
		freeReplyObject(reply);
	}
	
	pthread_mutex_unlock(&client.passet[hash]->mutex);
	return ret;
}

int free_item(void* key, size_t nkey) {
	uint32_t hash = client.hash(key);
	item *it;

	pthread_mutex_lock(&client.passet[hash]->mutex);
	it = client.passet[hash];	

	if ((it) && (it->key)) {
		if (!memcmp(it->key, key, nkey)) {
			free(client.passet[hash]->key);
			client.passet[hash]->key = NULL;

			free(client.passet[hash]->data);
			client.passet[hash]->data = NULL;

			if (it != client.passet[hash]) {
				it->prev->next = it->next;
				if (it->next) {
					it->next->prev = it->prev;
				}
				free(it);
			}

			pthread_mutex_unlock(&client.passet[hash]->mutex);
			redis_syncDel(client.context, (char *) key, nkey);
			return 1;
		} else {
			it = it ->next;
		}
	}
	
	// just make sure no bogus exist in the store.
	redis_syncDel(client.context, (char *) key, nkey);
	pthread_mutex_unlock(&client.passet[hash]->mutex);
	return 0;
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

redisReply* redis_syncGet(redisContext *c, char *key, size_t key_len) {
	redisReply *reply;
	
	if ((!key) || (!key_len)) {
		return NULL;
	}

	reply = redisCommand(c,"GET %b", key, (size_t) key_len);
	if (reply->type != REDIS_REPLY_NIL) {
		//strncpy(value, reply->str, *value_len);
		//*value_len = strlen(reply->str);
		printf("Got Value len: %zu\n", strlen(reply->str));
		//freeReplyObject(reply);
		return reply;
	}

	printf("No data available for key\n");
	freeReplyObject(reply);

	return NULL;
}

int redis_syncDel(redisContext *c, char *key, size_t key_len) {
	redisReply *reply;

	if ((!key) || (!key_len)) {
		return 0;
	}

	reply = redisCommand(c,"DEL %b", key, (size_t) key_len);
	printf("DEL: %s\n", reply->str);
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
