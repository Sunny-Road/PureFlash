#ifndef pf_replica_h__
#define pf_replica_h__

#include "pf_volume.h"
#include "pf_replicator.h"

class IoSubTask;
class PfFlashStore;


class PfLocalReplica : public PfReplica
{
public:
	virtual int submit_io(IoSubTask* subtask);
public:
	PfFlashStore* disk;
};

class PfSyncRemoteReplica : public PfReplica
{
public:
	virtual int submit_io(IoSubTask* subtask);
public:
	PfReplicator* replicator;
};

#endif // pf_replica_h__