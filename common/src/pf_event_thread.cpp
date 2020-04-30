#include <sys/prctl.h>
#include <string.h>

#include "pf_event_thread.h"
PfEventThread::PfEventThread() {
	inited = false;
}
int PfEventThread::init(const char* name, int qd)
{
	int rc = event_queue.init(name, qd, 0);
	if(rc)
		return rc;
	strncpy(this->name, name, sizeof(this->name));
	inited = true;
	return 0;
}
void PfEventThread::destroy()
{
	if(inited) {
		if(tid)
			stop();
		event_queue.destroy();
		inited = false;
	}
}
PfEventThread::~PfEventThread()
{
	destroy();
}

int PfEventThread::start()
{
	int rc = pthread_create(&tid, NULL, thread_proc, this);
	if(rc)
	{
		S5LOG_ERROR("Failed create thread:%s, rc:%d", name, rc);
		return rc;
	}
	return 0;
}
void PfEventThread::stop()
{
	event_queue.post_event(EVT_THREAD_EXIT, 0, NULL);
	pthread_join(tid, NULL);
	tid=0;

}

void *PfEventThread::thread_proc(void* arg)
{
	PfEventThread* pThis = (PfEventThread*)arg;
	prctl(PR_SET_NAME, pThis->name);
	PfFixedSizeQueue<S5Event>* q;
	int rc = 0;
	while ((rc = pThis->event_queue.get_events(&q)) == 0)
	{
		while(!q->is_empty())
		{
			S5Event* t = &q->data[q->head];
			q->head = (q->head + 1) % q->queue_depth;
			pThis->process_event(t->type, t->arg_i, t->arg_p);
		}
	}
	return NULL;
}

int PfEventThread::sync_invoke(std::function<int(void)> _f)
{
	S5LOG_FATAL("sync_invoke not implemented");
	return 0;
}

