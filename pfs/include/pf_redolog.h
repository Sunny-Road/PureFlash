#ifndef pf_redolog_h__
#define pf_redolog_h__
#include <stdint.h>
#include <thread>

#include "pf_flash_store.h"
#include "pf_tray.h"

class PfRedoLog
{
	enum class ItemType : uint32_t {
		ALLOCATE_OBJ = 1,
		TRIM_OBJ = 2,
		FREE_OBJ = 3,
		SNAP_SEQ_CHANGE = 4,
	};

	struct Item{
		int64_t phase;
		ItemType type;
		union {
			struct {
				struct lmt_key bkey;
				struct lmt_entry bentry;
				int free_queue_head;
			} allocation;
			struct {
				struct lmt_key bkey;
				struct lmt_entry bentry;
				int trim_queue_tail;
			}trim;
			struct {
				int obj_id;
				int trim_queue_head;
				int free_queue_tail;
			}free;
			struct {
				struct lmt_key bkey;
				struct lmt_entry bentry;
				uint32_t old_snap_seq;
			}snap_seq_change;
		};
	};

public:
	int disk_fd;
	int64_t phase;
	size_t size;
	struct PfFlashStore* store;
	off_t start_offset;
	off_t current_offset;
	void* entry_buff;
	std::thread auto_save_thread;

	int init(struct PfFlashStore* ssd);
	int load(struct PfFlashStore* ssd);
	int replay();
	int discard();
	int log_allocation(const struct lmt_key* key, const struct lmt_entry* entry, int free_list_head);
	int log_free(int block_id, int trim_list_head, int free_list_tail);
	int log_trim(const struct lmt_key* key, const struct lmt_entry* entry, int trim_list_tail);
	int redo_allocation(Item* e);
	int redo_trim(Item* e);
	int redo_free(Item* e);
	int log_snap_seq_change(const struct lmt_key* key, const struct lmt_entry* entry, int old_seq);
	int redo_snap_seq_change(PfRedoLog::Item* e);
	int stop();
	int start();
private:
	int write_entry();
};

#endif // pf_redolog_h__
