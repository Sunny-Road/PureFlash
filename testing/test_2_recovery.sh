#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $DIR/utils.sh

VOL_NAME=test_2
VOL_SIZE=$((5<<30)) #5G on my testing platform
COND_IP=$(pfcli get_pfc)
read DB_IP DB_NAME DB_USER DB_PASS <<< $(assert pfcli get_conn_str)
export DB_IP DB_NAME DB_USER DB_PASS

assert pfcli delete_volume  -v $VOL_NAME
assert pfcli create_volume  -v $VOL_NAME -s $VOL_SIZE -r 3
assert "fio --enghelp | grep pfbd "
fio -name=test -engine=pfbd -volume=$VOL_NAME -iodepth=1  -rw=randwrite -size=$VOL_SIZE -bs=4k -direct=1 &
FIO_PID=$!
sleep 10 #wait fio to start

STORE_IP=$(query_db "select mngt_ip from t_store where id in (select store_id from v_replica_ext where volume_name='$VOL_NAME') limit 1")
ssh root@$STORE_IP supervisorctl stop pfs #stop pfs
sleep 3

assert_equal $(query_db "select status from t_volume where name='$VOL_NAME'") "DEGRADED"

curlex "http://$COND_IP:49180/s5c/?op=recovery_volume&name=$VOL_NAME"

assert_equal $(query_db "select status from t_volume where name='$VOL_NAME'") "OK"

assert wait $FIO_PID
