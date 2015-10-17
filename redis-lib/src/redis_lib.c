#include "redis_lib.h"

pthread_t thread_id1;
pthread_t thread_id2;

int register_encode_decode(get_key_val get, put_key_val put, key_hash hash,
			   eventual_con ev_con, get_delta delta, async_handle handle,
			   clear_waiting cwait) {
	client.get = get;
	client.put = put;
	client.hash = hash;
	client.ev_con = ev_con;
	client.delta = delta;
	client.handle = handle;
	client.cwait = cwait;
}

static void *thread_event(void *arg) {
	redisReply *reply;
        size_t total_size, p_size;
	char *p = NULL;
        uint64_t vnf_id, version, lock;

	while (1) {
		if ((client.flags & SEQUENTIAL_CONSISTENCY) && (!client.wait)) {
			sleep(1);
		}

		if (client.flags | ASYNC) {
			event_base_dispatch(client.base);
		}

		if ((client.flags & SEQUENTIAL_CONSISTENCY) && (client.wait)) {
                	reply = redis_syncGet(client.context,
                                              client.it->key,
                                              client.it->nkey);

			if (reply) {
                        	total_size = reply->len;
                        	p_size = total_size - sizeof(meta_data);
                                p = ((char *) reply->str) + p_size;

                                vnf_id = (uint64_t) *(p);
                                version = (uint64_t) *(p + 8);
                                lock = (uint64_t) *(p + 16);

                                if ((vnf_id != client.vnf_id) && (!lock)) {
                                	if (client.it->flags & ITEM_WAITING) {
                                        	// lock released from the other node, update state
                                                // and trigger application.
                                                *p = '\0';
						pthread_mutex_lock(&client.it->mutex);
                                                client.put(reply->str, (void *) client.it->data);
                                                client.cwait(client.it->data);
                                                client.handle(client.it->key);
                                                client.wait = 0;
						client.it = NULL;
                                                client.it->flags &= ~ITEM_WAITING;
						pthread_mutex_unlock(&client.it->mutex);
                                                // fall through and update the VNF
                                        }
                        	}
			}
		}
		
	}
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
	uint64_t vnf_id, version, lock;
	char temp[1024];

	sleep(10);

	while (1) {
		sleep(client.time);
		
		for (i=0; i<BUCKET_SIZE; i++) {
			it = client.passet[i];
			pthread_mutex_lock(&it->mutex);
			while ((it) && (it->key != NULL)) {

				reply = redis_syncGet(client.context,
					              it->key,
				        	      it->nkey);

				if (client.flags & NO_CONSISTENCY) {
					if (reply) {
						//printf("Data Available \n");
						total_size = reply->len;
						p_size = total_size - sizeof(meta_data);
						p = ((char *) reply->str) + p_size;

						vnf_id = (uint64_t) *(p);

						if (vnf_id != client.vnf_id) {
							// We are not the owner. We should delete
							// from our cache.
							it = it->next;
							continue;
						}
						
						//printf(" Data %s\n", reply->str);
						//printf("get vnf_id %lld\n", (long long)*(p));
						//printf("get version %lld\n", (long long)*(p+8));
						//printf("get lock %lld\n", (long long)*(p+16));
						//printf("set total length %zu\n", total_size);

						freeReplyObject(reply);
					}
				} else if (client.flags & EVENTUAL_CONSISTENCY) {
					if (reply) {
						// check the version here vs the version available in store.
						// Use the information to just update the delta.
                                                //printf("Data Available \n");
                                                total_size = reply->len;
                                                p_size = total_size - sizeof(meta_data);
                                                p = ((char *) reply->str) + p_size;

                                                vnf_id = (uint64_t) *(p);
						version = (uint64_t) *(p + 8);

                                                if (vnf_id != client.vnf_id) {
							
							// we might still be waiting for the updated version

							if (version == it->mdata->version) {	
                                                        	// We don't see any update. we will update our
								// state later.
								it = it->next;
                                                        	continue;
							} else if (version > it->mdata->version) {
								// handle updating the delta of both versions.
								// we need to let this fall through and update
								// the new version to the store.

								// This registered application procedure will 
								// take care of eventual consistency.
								*p = '\0';
								client.put(reply->str, temp);
								client.delta(it->temp_data, temp);
								client.ev_con(it->data, temp);
								freeReplyObject(reply);
								it->mdata->version = version;
								it->mdata->vnf_id  = client.vnf_id;
							} // else should just fall through
                                                }
					}
				} else if (client.flags & SEQUENTIAL_CONSISTENCY) {
                                        if (reply) {
                                                // check the version here vs the version available in store.
                                                // Use the information to just update the delta.
                                                //printf("Data Available \n");
                                                total_size = reply->len;
                                                p_size = total_size - sizeof(meta_data);
                                                p = ((char *) reply->str) + p_size;

                                                vnf_id = (uint64_t) *(p);
                                                version = (uint64_t) *(p + 8);
                                                lock = (uint64_t) *(p + 16);

                                                if ((vnf_id != client.vnf_id) && (lock)) {

							// we are not the owner and the data is locked.
							// This cache entry should be deleted.
							// In sequential, don't care about the version.
                                                        it = it->next;
                                                        continue;
                                                }

						if ((vnf_id != client.vnf_id) && (!lock)) {
							if (it->flags & ITEM_WAITING) {
								// lock released from the other node, update state
								// and trigger application.
								*p = '\0';
								client.put(reply->str, (void *) it->data);
								client.cwait(it->data);
								client.handle(it->key);
								client.wait = 0;
								it->flags &= ~ITEM_WAITING;
								// fall through and update the VNF
							}
						}
                                        }
				}
				// all pre-checks are done based on consistency. Now update the state.
				++it->mdata->version;
				if (client.flags & SEQUENTIAL_CONSISTENCY) {
					if (it->mdata->lock == 0) {
						it->mdata->lock = 1;
					}
				}
                                client.get((void *) it->key, &p);
                                if (p) {
                                	p_size = strlen(p);
                                        total_size = p_size + sizeof(meta_data);
                                        p = (char *) realloc(p, total_size);
                                        memcpy(m, (it->data + it->size), sizeof(meta_data));
                                        memcpy(p+p_size, m, sizeof(meta_data));

                                        redis_syncSet(client.context,
                                                      it->key,
                                                      it->nkey,
                                                      p,
                                                      total_size);
                                        free(p);
                                        p = NULL;
                                 }
                                 // move to next item
                                 it = it->next;

			}
			pthread_mutex_unlock(&client.passet[i]->mutex);
		}
	}
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

redis_client *create_cache(char *host, int port, uint32_t vnf_id,
			   consistency_type con, int time) {
	int i;

        client.base = event_base_new();
	client.vnf_id  = vnf_id;
	client.context = createClient(host, port);
	client.flags |= con;
	client.time = time;  // time in seconds to sync state in background.
	client.wait = 0;

	if (NULL == client.context) {
		printf("No connection to server \n");
		return NULL;
	}

	client.async_context = redisAsyncConnect(host, port);
	signal(SIGPIPE, SIG_IGN);

	if (NULL == client.async_context) {
		printf("No connection to server \n");
		return NULL;
	}
	redisLibeventAttach(client.async_context, client.base);
    	redisAsyncSetConnectCallback(client.async_context, connectCallback);
    	redisAsyncSetDisconnectCallback(client.async_context, disconnectCallback);

	printf("Size of metadata %lu", sizeof(meta_data));

	for (i=0; i<BUCKET_SIZE; i++) {
		client.passet[i] = malloc(sizeof(item));
		if (client.passet[i]) {
			memset(client.passet[i], 0, sizeof(item));
			pthread_mutex_init(&client.passet[i]->mutex, NULL);
		}
	}
	pthread_create(&thread_id1, NULL, &thread_start, NULL);
	if ((client.flags & ASYNC) || (client.flags & SEQUENTIAL_CONSISTENCY)) {
		pthread_create(&thread_id2, NULL, &thread_event, NULL);
	}
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

int unlock_and_update_item(void* key) {
	uint32_t hash = client.hash(key);
	char *p=NULL;
	size_t total_size, p_size;
	char m[24];
	item *it;

	pthread_mutex_lock(&client.passet[hash]->mutex);
	it = client.passet[hash];

	if (it->key) {
		client.get((void *) it->key, &p);
                if (client.flags & SEQUENTIAL_CONSISTENCY) {
	                if (it->mdata->lock == 1) {
                        	it->mdata->lock = 0;
                        } else {
				return 0;
			}
                } else {
			return 0;
		}
                client.get((void *) it->key, &p);
                if (p) {
                	p_size = strlen(p);
                	total_size = p_size + sizeof(meta_data);
                	p = (char *) realloc(p, total_size);
                	memcpy(m, (it->data + it->size), sizeof(meta_data));
                	memcpy(p+p_size, m, sizeof(meta_data));

			printf("Released Lock \n");

                	redis_syncSet(client.context,
                	              it->key,
                                      it->nkey,
                                      p,
                                      total_size);
                        free(p);
			return 1;
                }
	}
	return 0;
}

int create_item(void* key, size_t nkey, void **data,
                size_t size, uint32_t flags, time_t exp) {
	uint32_t hash = client.hash(key);
	item *it, *temp_next;
	redisReply *reply = NULL;
	int ret = 0;
	meta_data *mdata;
	uint64_t vnf_id, version, lock;
	char *p;
	size_t p_size, total_size;

	*data = NULL;
	// we know the key is uint64_t, so just cast it to uint64_t
	// this has to be modified to generalize things

	pthread_mutex_lock(&client.passet[hash]->mutex);

	if (client.passet[hash]->key) {
		it = malloc(sizeof(item));
		if (!it) {
			printf("no space for data\n");
			return DATA_NO;	
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
		return DATA_NO;
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
        gettimeofday(&start_deserialize, NULL);

	if (!(client.flags & ASYNC)) {
		reply = redis_syncGet(client.context, (char *) it->key, nkey);
	} else {
		redis_asyncGet(client.async_context, (char *) it->key, nkey, it);

   		gettimeofday(&end_deserialize, NULL);
   		long sec = end_deserialize.tv_sec - start_deserialize.tv_sec;
   		long usec = end_deserialize.tv_usec - start_deserialize.tv_usec;
   		long total = (sec * 1000 * 1000) + usec;
   		printf("STATS: PERFLOW: State Get Timestamp = %ldus\n", total);

		pthread_mutex_unlock(&client.passet[hash]->mutex);
		return DATA_WAIT;
	}

	if (reply) {
   		gettimeofday(&end_deserialize, NULL);
   		long sec = end_deserialize.tv_sec - start_deserialize.tv_sec;
   		long usec = end_deserialize.tv_usec - start_deserialize.tv_usec;
   		long total = (sec * 1000 * 1000) + usec;
   		printf("STATS: PERFLOW: State Get Timestamp = %ldus\n", total);

                total_size = reply->len;
                p_size = total_size - sizeof(meta_data);
                p = ((char *) reply->str) + p_size;

                vnf_id = (uint64_t) *(p);
                version = (uint64_t) *(p + 8);
                lock = (uint64_t) *(p + 16);

		*p = '\0';

		if (client.flags & SEQUENTIAL_CONSISTENCY) {
			if (lock) {
				// Don't process the packets yet. Wait till the other
				// VNF releases the lock.
				client.it = it;
				it->flags |= ITEM_WAITING;
		                freeReplyObject(reply);
       			        ret = DATA_WAIT;
				pthread_mutex_unlock(&client.passet[hash]->mutex);
				client.wait = 1;
				return ret;
			}
		}

		client.put(reply->str, (void *) it->data);
		if (client.flags & EVENTUAL_CONSISTENCY) {
			// copy the item and maintain the data
			it->temp_data = (char *) malloc(size + sizeof(meta_data));
			it->temp_mdata = (meta_data *) ((char *)it->temp_data + size);
			memcpy(it->temp_data, it->data, (size + sizeof(meta_data)));
		} 
		freeReplyObject(reply);
		ret = DATA_READY;
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
	//printf("SET: %s\n", reply->str);
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
		//printf("Got Value len: %zu\n", strlen(reply->str));
		//freeReplyObject(reply);
		return reply;
	}

	//printf("No data available for key\n");
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

void state_getcb(redisAsyncContext *c, void *r, void *privdata) {
	redisReply *reply = r;
	uint64_t vnf_id, version, lock, p_size, total_size;
	char *p;
	item *it = (item *) privdata;
	uint32_t hash = client.hash(it->key);

    	if (reply == NULL) return;

	if (reply->type != REDIS_REPLY_NIL) {
   		gettimeofday(&end_deserialize, NULL);
   		long sec = end_deserialize.tv_sec - start_deserialize.tv_sec;
   		long usec = end_deserialize.tv_usec - start_deserialize.tv_usec;
   		long total = (sec * 1000 * 1000) + usec;
   		printf("STATS: PERFLOW: State Get Timestamp = %ldus\n", total);

        	total_size = reply->len;
        	p_size = total_size - sizeof(meta_data);
        	p = ((char *) reply->str) + p_size;

        	vnf_id = (uint64_t) *(p);
        	version = (uint64_t) *(p + 8);
        	lock = (uint64_t) *(p + 16);

        	*p = '\0';

		pthread_mutex_lock(&client.passet[hash]->mutex);
        	client.put(reply->str, (void *) it->data);
		pthread_mutex_unlock(&client.passet[hash]->mutex);
	} else {
   		gettimeofday(&end_deserialize, NULL);
   		long sec = end_deserialize.tv_sec - start_deserialize.tv_sec;
   		long usec = end_deserialize.tv_usec - start_deserialize.tv_usec;
   		long total = (sec * 1000 * 1000) + usec;
   		printf("STATS: PERFLOW: State Get Timestamp = %ldus\n", total);
		//memset(it->data, 0, it->size);
		client.cwait(it->data);
	}
	freeReplyObject(reply);
	client.handle(it->key);
}

int redis_asyncGet(redisAsyncContext *c, char *key, int key_len, void *item) {
        if ((!key) || (!key_len)) {
                return 0;
        }

        redisAsyncCommand(c, state_getcb, item, "GET %b", key, (size_t) key_len);

	return 1;
}

int destroyClient(redisContext *context) {
	return 0;
}
