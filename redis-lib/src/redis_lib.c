#include "redis_lib.h"

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

int redis_syncGet(redisContext *c, char *key, int key_len, char *value, int *value_len) {
	redisReply *reply;
	
	if ((!key) || (!key_len) || (!value) || (!value_len)) {
		return 0;
	}

	reply = redisCommand(c,"GET %b", key, (size_t) key_len);
	strncpy(value, reply->str, *value_len);
	*value_len = strlen(reply->str);

	printf("GET foo: %s\n", reply->str);
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
