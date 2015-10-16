#include <stddef.h>
#include "serialize.h"
#include "prads.h"
#include "config.h"
#include "cxt.h"

#define SERIALIZE_PRINT(...) printf(__VA_ARGS__); printf("\n");

//global config
globalconfig config;

long overall_pserz_time = 0;
long overall_pdeserz_time = 0;
long overall_pstate_size = 0;

ser_tra_t* setup_serialize_translators()
{
    ser_tra_t *tra1, *tra2, *tra3, *tra4;
    ser_field_t *field;

    head_tra = ser_new_tra("connection",sizeof(connection),NULL);
    ser_new_field(head_tra,"time_t",0,"start_time",offsetof(connection,start_time));
    ser_new_field(head_tra,"time_t",0,"last_pkt_time",offsetof(connection,last_pkt_time));
    ser_new_field(head_tra,"uint32_t",0,"cxid",offsetof(connection,cxid));
    ser_new_field(head_tra,"uint8_t",0,"reversed",offsetof(connection,reversed));
    ser_new_field(head_tra,"uint32_t",0,"af",offsetof(connection,af));
    ser_new_field(head_tra,"uint16_t",0,"hw_proto",offsetof(connection,hw_proto));
    ser_new_field(head_tra,"uint8_t",0,"proto",offsetof(connection,proto));
    ser_new_field(head_tra,"in6",0,"s_ip",offsetof(connection,s_ip));
    ser_new_field(head_tra,"in6",0,"d_ip",offsetof(connection,d_ip));
    ser_new_field(head_tra,"uint16_t",0,"s_port",offsetof(connection,s_port));
    ser_new_field(head_tra,"uint16_t",0,"d_port",offsetof(connection,d_port));
    ser_new_field(head_tra,"uint32_t",0,"s_total_pkts",offsetof(connection,s_total_pkts));
    ser_new_field(head_tra,"uint32_t",0,"s_total_bytes",offsetof(connection,s_total_bytes));
    ser_new_field(head_tra,"uint32_t",0,"d_total_pkts",offsetof(connection,d_total_pkts));
    ser_new_field(head_tra,"uint32_t",0,"d_total_bytes",offsetof(connection,d_total_bytes));
    ser_new_field(head_tra,"uint8_t",0,"s_tcpFlags",offsetof(connection,s_tcpFlags));
    ser_new_field(head_tra,"uint8_t",0,"__pad__",offsetof(connection,__pad__));
    ser_new_field(head_tra,"uint8_t",0,"d_tcpFlags",offsetof(connection,d_tcpFlags));
    ser_new_field(head_tra,"uint8_t",0,"check",offsetof(connection,check));
    ser_new_field(head_tra,"asset",1,"c_asset",offsetof(connection,c_asset));
    ser_new_field(head_tra,"asset",1,"s_asset",offsetof(connection,s_asset));
 
    // translator for asset
    tra0 = ser_new_tra("asset",sizeof(asset),head_tra);
    //ser_new_field(tra0,"asset",1,"prev",offsetof(asset,prev));
    ser_new_field(tra0,"time_t",0,"first_seen",offsetof(asset,first_seen));
    ser_new_field(tra0,"time_t",0,"last_seen",offsetof(asset,last_seen));
    ser_new_field(tra0,"ushort",0,"i_attempts",offsetof(asset,i_attempts));
    ser_new_field(tra0,"int",0,"af",offsetof(asset,af));
    ser_new_field(tra0,"uint16_t",0,"vlan",offsetof(asset,vlan));
    ser_new_field(tra0,"in6",0,"ip_addr",offsetof(asset,ip_addr));
    ser_new_field(tra0,"serv_asset",1,"services",offsetof(asset,services));
    ser_new_field(tra0,"os_asset",1,"os",offsetof(asset,os));
    
    // translator for bstring
    tra1 = ser_new_tra("tagstr",sizeof(tagstr),tra0);
    ser_new_field(tra1,"int",0,"mlen",offsetof(tagstr,mlen));
    ser_new_field(tra1,"int",0,"slen",offsetof(tagstr,slen));
    ser_new_field(tra1,"string",0,"data",offsetof(tagstr,data));
    
    // translator for serv_asset
    tra2 = ser_new_tra("serv_asset",sizeof(serv_asset),tra1);
    ser_new_field(tra2,"serv_asset",1,"prev",offsetof(serv_asset,prev));
    ser_new_field(tra2,"serv_asset",1,"next",offsetof(serv_asset,next));
    ser_new_field(tra2,"time_t",0,"first_seen",offsetof(serv_asset,first_seen));
    ser_new_field(tra2,"time_t",0,"last_seen",offsetof(serv_asset,last_seen));
    ser_new_field(tra2,"ushort",0,"i_attempts",offsetof(serv_asset,i_attempts));
    ser_new_field(tra2,"ushort",0,"proto",offsetof(serv_asset,proto));
    ser_new_field(tra2,"uint16_t",0,"port",offsetof(serv_asset,port));
    ser_new_field(tra2,"uint8_t",0,"ttl",offsetof(serv_asset,ttl));
    ser_new_field(tra2,"tagstr",1,"service",offsetof(serv_asset,service));
    ser_new_field(tra2,"tagstr",1,"application",offsetof(serv_asset,application));
    ser_new_field(tra2,"int",0,"role",offsetof(serv_asset,role));
    ser_new_field(tra2,"int",0,"unknown",offsetof(serv_asset,unknown));
    
    // translator for os_asset
    tra3 = ser_new_tra("os_asset",sizeof(os_asset),tra2);
    ser_new_field(tra3,"serv_asset",1,"prev",offsetof(os_asset,prev));
    ser_new_field(tra3,"serv_asset",1,"next",offsetof(os_asset,next));
    ser_new_field(tra3,"time_t",0,"first_seen",offsetof(os_asset,first_seen));
    ser_new_field(tra3,"time_t",0,"last_seen",offsetof(os_asset,last_seen));
    ser_new_field(tra3,"ushort",0,"i_attempts",offsetof(os_asset,i_attempts));
    ser_new_field(tra3,"tagstr",1,"vendor",offsetof(os_asset,vendor));
    ser_new_field(tra3,"tagstr",1,"os",offsetof(os_asset,os));
    ser_new_field(tra3,"uint8_t",0,"detection",offsetof(os_asset,detection));
    ser_new_field(tra3,"tagstr",1,"raw_fp",offsetof(os_asset,raw_fp));
    ser_new_field(tra3,"tagstr",1,"matched_fp",offsetof(os_asset,matched_fp));
    ser_new_field(tra3,"string",0,"match_os",offsetof(os_asset,match_os));
    ser_new_field(tra3,"string",0,"match_desc",offsetof(os_asset,match_desc));
    ser_new_field(tra3,"int16_t",0,"port",offsetof(os_asset,port));
    ser_new_field(tra3,"int16_t",0,"mtu",offsetof(os_asset,mtu));
    ser_new_field(tra3,"int8_t",0,"ttl",offsetof(os_asset,ttl));
    ser_new_field(tra3,"int32_t",0,"uptime",offsetof(os_asset,uptime));
 
    //translator for in6_addr
    tra4 = ser_new_tra("in6", sizeof(uint8_t) * 4, tra3);
    field = ser_new_field(tra4, "uint8_t", 0, "value", 0);
    field->repeat = 4;
}

char* serialize_conn_asset(connection *conn)
{ 
    asset *c_asset = NULL, *s_asset = NULL;

    c_asset = conn->c_asset;
    s_asset = conn->s_asset;
    pthread_mutex_lock(&AssetEntryLock);  
    if (c_asset != NULL)
    {
    	//if (c_asset->sdmbn_msgid == msgid)
        //{
            // temp set the conn->c_asset to NULL
            // to avoid retransmitting the same asset struct
            conn->c_asset = NULL; 
        //}
        //c_asset->sdmbn_msgid = msgid;
    }

    if (s_asset != NULL)
    {
        //if (s_asset->sdmbn_msgid == msgid)
        //{
            // temp set the conn->s_asset to NULL
            // to avoid retransmitting the same asset struct
            conn->s_asset = NULL;
        //}
        //s_asset->sdmbn_msgid = msgid;
    }

    char *state = ser_ialize(head_tra, "connection", conn, NULL, 0);
    // restore the asset pointers
    conn->c_asset = c_asset;
    conn->s_asset = s_asset;
    pthread_mutex_unlock(&AssetEntryLock); 
    return state;
}

int eventual_cons(void *old, void *new) {
	connection *curr_data = (connection *) old;
	connection *new_data = (connection *) new;

    	pthread_mutex_lock(&AssetEntryLock);  
	

   	pthread_mutex_lock(&AssetEntryLock);  
		
	return 1;
}

int get_conn_delta(void *old, void *new) {
	connection *curr_data = (connection *) old;
	connection *new_data = (connection *) new;

	if (curr_data->start_time < new_data->start_time) {
		curr_data->start_time = new_data->start_time;
	}

	if (curr_data->last_pkt_time < new_data->last_pkt_time) {
		new_data->last_pkt_time = curr_data->last_pkt_time;
	}
	return 1;
}

// The calling function should free "data" after using it.
int get_key_value(void *key, char **data)
{
    int count = 0;
    connection *curr = NULL, *head = NULL;
    prads_key *pkey = (prads_key *) key;
    uint32_t hash;

    *data = NULL;
    hash = CXT_HASH4(pkey->src,pkey->dst,pkey->sport,pkey->dport,pkey->prot);

    pthread_mutex_lock(&ConnEntryLock);
    head = bucket[hash];

    for (curr = head; curr != NULL; curr = curr->next) {
	if (CMP_CXT4(curr,pkey->src,pkey->sport,pkey->dst,pkey->dport)){
		break;
	}
    }

    if (curr != NULL)
    {
	    if (curr->__pad__ == DATA_WAIT) {
		pthread_mutex_unlock(&ConnEntryLock);
		return 1;
	    }
	    // Prepare to send perflow state
            int hashkey = curr->cxid;

            // Serialize conn and asset structure into a single character 
            // stream.
	    struct timeval start_serialize, end_serialize;
	    gettimeofday(&start_serialize, NULL);

	    *data = (char *)serialize_conn_asset(curr);
   			
   	    gettimeofday(&end_serialize, NULL);
	    long sec = end_serialize.tv_sec - start_serialize.tv_sec;
	    long usec = end_serialize.tv_usec - start_serialize.tv_usec;
	    long total = (sec * 1000 * 1000) + usec;
			overall_pserz_time += total;
			overall_pstate_size	+= strlen(data);
  	    //printf("STATS: PERFLOW STATE SIZE CURRENT = %zu\n", strlen(*data));
	    //printf("STATS: PERFLOW STATE SIZE OVERALL = %zu\n", overall_pstate_size);
	    //printf("STATS: PERFLOW: TIME TO SERIALIZE CURRENT = %ldus\n", total);
	    //printf("STATS: PERFLOW: TIME TO SERIALIZE OVERALL = %ldus\n", overall_pserz_time);
            //SERIALIZE_PRINT("serializing connection struct with multi flow %s",*data);
 
            if (NULL == *data) {
		pthread_mutex_unlock(&ConnEntryLock);
                return count; 
	    }

            // Increment count
            count++;
    }
    pthread_mutex_unlock(&ConnEntryLock);
    return 1;
}

int put_value_struct(char *data, void *c)
{
   connection *curr, *curr_val;

   if ((!data) || (!c)) {
	return 0;
   }

   printf("received value \n");

   curr_val = (connection *) c;

   struct timeval start_deserialize, end_deserialize;
   gettimeofday(&start_deserialize, NULL);

   curr = ser_parse(head_tra, "connection", data, NULL);
   *curr_val = *curr;
   free(curr);
	
   gettimeofday(&end_deserialize, NULL);
   long sec = end_deserialize.tv_sec - start_deserialize.tv_sec;
   long usec = end_deserialize.tv_usec - start_deserialize.tv_usec;
   long total = (sec * 1000 * 1000) + usec;
   printf("STATS: PERFLOW: TIME TO DESERIALIZE CURRENT = %ldus\n", total);
   //printf("STATS: PERFLOW: TIME TO DESERIALIZE OVERALL = %ldus\n", overall_pdeserz_time);

   curr->c_asset = NULL;
   curr->s_asset = NULL;

   return 1;
}

uint32_t hash(void *key) {
   prads_key *pkey = (prads_key *) key;
   uint32_t hash;

   if (!key) {
     return 0;
   }

   hash = CXT_HASH4(pkey->src,pkey->dst,pkey->sport,pkey->dport,pkey->prot);
   return hash;
}

int async_app_handle(void *key) {
    process_pack_list();
}

int app_cwait (void *data) {
	connection *curr = (connection *) data;
	curr->__pad__ = 0;
}
