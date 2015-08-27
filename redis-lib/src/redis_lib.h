#include "../deps/hiredis/hiredis.h"
#include <string.h>
#include <stdio.h>

// returns the client context after setting up the connection
redisContext *createClient(char *host, int port);

// Syncronous Get and Set methods.
int redis_syncSet(redisContext *c, char *key, int key_len, char *value, int value_len);
int redis_syncGet(redisContext *c, char *key, int key_len, char *value, int *value_len);

// Syncronous Get and Set methods.
int redis_asyncSet(char *key, int key_len, char *value, int value_len);
int redis_asyncGet(char *key, int key_len, char *value, int *value_len);

int destroyClient(redisContext *context);
