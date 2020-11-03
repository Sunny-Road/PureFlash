#ifndef pf_restful_api_h__
#define pf_restful_api_h__

#include <string>
#include <vector>
#include <stdint.h>
#include <nlohmann/json.hpp>

class ReplicaArg
{
public:
	int index;
	int store_id;
	std::string tray_uuid;
	std::string status;
	std::string rep_ports;
};
class ShardArg
{
public:
	int index;
	std::vector<ReplicaArg> replicas;
	int primary_rep_index;
	std::string status;
};
class PrepareVolumeArg
{
public:
	std::string op;
	std::string status;
	std::string volume_name;
	uint64_t volume_size;
	uint64_t volume_id;
	uint64_t meta_ver;
	int snap_seq;
	int shard_count;
	int rep_count;
	std::vector<ShardArg> shards;
};

class RestfulReply
{
public:
	std::string reason;
	std::string op;
	int ret_code;//code defined in interface RetCode
	const static int OK = 0;
	const static int INVALID_OP = 1;
	const static int INVALID_ARG = 2;
	const static int DUPLICATED_NODE = 3;
	const static int DB_ERROR = 4;
	const static int REMOTE_ERROR = 5;
	const static int ALREADY_DONE = 6;
	const static int INVALID_STATE = 7;

	RestfulReply();
	RestfulReply(std::string op, int ret_code=RestfulReply::OK, const char* reason="");
};
void from_json(const nlohmann::json& j, RestfulReply& p);

class GetSnapListReply : public RestfulReply {
public:
	std::vector<int> snap_list;
};
void from_json(const nlohmann::json& j, GetSnapListReply& p);

class BackgroudTaskReply : public RestfulReply {
public:
	int64_t task_id;
	std::string status; //WAITING, RUNNING, SUCCEEDED, FAILED
	int progress;

	BackgroudTaskReply():task_id(0), progress(0){ }
};
class ErrorHandleReply : public RestfulReply {

};
struct mg_connection;
struct http_message;

std::string get_http_param_as_string(const struct mg_str *http_str, const char *name, const char* def_val, bool mandatory=false);
int64_t get_http_param_as_int64(const struct mg_str *http_content, const char *name, int64_t def_val, bool mandatory=false);

void handle_prepare_volume(struct mg_connection *nc, struct http_message * hm);
void handle_set_snap_seq(struct mg_connection *nc, struct http_message * hm);
void handle_set_meta_ver(struct mg_connection *nc, struct http_message * hm);
void handle_delete_snapshot(struct mg_connection *nc, struct http_message * hm);
void handle_get_obj_count(struct mg_connection *nc, struct http_message * hm);
void handle_begin_recovery(struct mg_connection *nc, struct http_message * hm);
void handle_end_recovery(struct mg_connection *nc, struct http_message *hm);
void handle_recovery_replica(struct mg_connection *nc, struct http_message * hm);
void handle_get_snap_list(struct mg_connection *nc, struct http_message * hm);
void handle_delete_replica(struct mg_connection *nc, struct http_message * hm);
void handle_query_task(struct mg_connection *nc, struct http_message * hm);
#endif // pf_restful_api_h__
