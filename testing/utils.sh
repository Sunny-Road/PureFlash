#!/bin/bash
function fatal {
    echo -e "\033[31m$* \033[0m"
    exit 1
}
function info {
    echo -e "\033[32m$* \033[0m"
}

function assert()
{
    local cmd=$*
    echo "Run:$cmd" > /dev/stderr
    eval ${cmd}
    if [ $? -ne 0 ]; then
        fatal "Failed to run:$cmd"
    fi
}

function assert_equal()
{
    if [ "$1" != "$2" ]; then
        fatal "Assert fail, $1 != $2"
    fi
}
function curlex () {
    echo "curl $@"
    rsp=$(curl --write-out '\n%{http_code}\n'  "$@" 2>/dev/null)
    code=$(echo "$rsp" | tail -n 1)
    ret=$(echo "$rsp" | head -n -1 | jq -r ".ret_code")
    echo "$rsp, $ret"
    if [ ! $ret ];then
        ret=0
    fi
    if (( $code >= 400 )); then
        return 22
    elif (( $ret != 0 )); then
        return 22
    else
        return 0
    fi
}
function query_db () {
    mysql -h$DB_IP -u$DB_USER -p$DB_PASS $DB_NAME -B --disable-column-names -e "$*"
}

function get_obj_count() {
    total=0
    store_ip=$(query_db "select mngt_ip from t_store where id in (select store_id from v_replica_ext where volume_name='$1')")
    for ip in $store_ip; do
        cnt=$(curl "http://$ip:49181/debug?op=get_obj_count")
        total=$(($total + $cnt))
    done
    echo $total
}

async_curl() {
	echo "curl $@"
    rsp=$(curl --write-out '\n%{http_code}\n'  "$@" 2>/dev/null)
	code=$(echo "$rsp" | tail -n 1)
	ret=$(echo "$rsp" | head -n 1 | jq -r ".ret_code")
	echo "$rsp, $ret"
	if [ ! $ret ];then
		ret=0
	fi
	if (( $code >= 400 )); then
		return 22
	fi
	if (( $ret != 0 )); then
		return 22
	fi
	task_id=$(echo "$rsp" | head -n 1 | jq -r ".task_id")
	echo "task_id:$task_id"
	echo "queue_name:$queue_name"
	while sleep 5 ; do
		rsp=$(curl --write-out '\n%{http_code}\n'  "http://$(pfcip):49180/pfapi?op=list_job&id=$task_id&queue_name=$queue_name" 2>/dev/null)
		echo $rsp
		status=$(echo "$rsp" | head -n 1 | jq -r ".jobs[0].status")
		echo $status
		if [ "$status" == "pending" ] ; then
			continue
		elif [ "$status" == "processing" ] ; then
			continue
		elif [ "$status" == "failed" ] ; then
			return -2
		elif [ "$status" == "succeeded" ] ; then
			return 0
		fi
	done
	return -1
}