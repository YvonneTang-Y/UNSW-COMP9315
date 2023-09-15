#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ro.h"
#include "db.h"

void init(){
    // do some initialization here.

    // example to get the Conf pointer
    // Conf* cf = get_conf();

    // example to get the Database pointer
    // Database* db = get_db();
    
    Conf* cf = get_conf();
    Database* db = get_db();
    
    printf("init() is invoked.\n");
}

void release(){
    // optional
    // do some end tasks here.
    // free space to avoid memory leak
    printf("release() is invoked.\n");
}


/*****************************************************************************
 * buffer management
 *****************************************************************************/

#define MAXID 200

// for Cycle strategy, we simply re-use the nused counter
// since we don't actually need to maintain a usedList
#define currSlot nused

// one buffer
struct buffer {
	char  id[MAXID];
	int   pin;
	int   usage;
	char  *data;
};

// collection of buffers + stats
struct bufPool {
	int   nbufs;         // how many buffers
	// char  strategy;      // LRU, MRU, Cycle
	// int   nrequests;     // stats counters
	// int   nreleases;
	// int   nhits;
	// int   nreads;
	// int   nwrites;
	int   *freeList;     // list of completely unused bufs
	int   nfree;
	// int   *usedList;     // implements replacement strategy
	// int   nused;
	struct buffer *bufs;
    int   *clock;        // add clock
    // int   clock;         // add clock
};

typedef struct bufPool *BufPool;

BufPool initBufPool(int nbufs, char strategy)
{
	BufPool newPool;
	struct buffer *bufs;

	newPool = malloc(sizeof(struct bufPool));
	assert(newPool != NULL);
	newPool->nbufs = nbufs;
	// newPool->strategy = strategy;
	// newPool->nrequests = 0;
	// newPool->nreleases = 0;
	// newPool->nhits = 0;
	// newPool->nreads = 0;
	// newPool->nwrites = 0;
	newPool->nfree = nbufs;
	// newPool->nused = 0;
	newPool->freeList = malloc(nbufs * sizeof(int));
	assert(newPool->freeList != NULL);
	// newPool->usedList = malloc(nbufs * sizeof(int));
	// assert(newPool->usedList != NULL);
	newPool->bufs = malloc(nbufs * sizeof(struct buffer));
	assert(newPool->bufs != NULL);
    newPool->clock = malloc(nbufs * sizeof(int)); // add clock
    assert(newPool->clock != NULL);

	int i;
	for (i = 0; i < nbufs; i++) {
		newPool->bufs[i].id[0] = '\0';
		newPool->bufs[i].pin = 0;
		newPool->bufs[i].usage = 0;
		newPool->freeList[i] = i;
		// newPool->usedList[i] = -1;
        newPool->clock[i] = -1;         // add clock
	}
    // newPool->clock = 0;
	return newPool;
}

int request_page(BufPool pool, char *rel, int page);
void release_page(BufPool pool, char *rel, int page);


// pageInPool(BufPool pool, char rel, int page)
// - check whether page from rel is already in the pool
// - returns the slot containing this page, else returns -1
static
int pageInPool(BufPool pool, char *rel, int page)
{
	int i;  char id[MAXID];
	sprintf(id, "%s%d", rel, page);
	for (i = 0; i < pool->nbufs; i++) {
		if (strcmp(id, pool->bufs[i].id) == 0) {
			return i;
		}
	}
	return -1;
}

int request_page(BufPool pool, char *rel, int page){
    int slot; char id[MAXID];
    sprintf(id, "%s%d", rel, page);
    struct buffer *buf = NULL;
    slot = pageInPool(pool,rel,page);
    // page is in buffer
    if (slot >= 0) return slot;
    // find a free buffer
    for (slot = 0; slot < pool->nbufs; slot++){
        int bufIndex = pool->freeList[slot];
        if (pool->bufs[bufIndex].pin == 0){
            buf = &pool->bufs[bufIndex];
            pool->freeList[slot] = pool->freeList[pool->nfree - 1];
            pool->nfree--;
            break;
        }        
    }

    // if buffer pool is full
    if (buf == NULL){
        int clockIndex = 0;
        while (1) {
            struct buffer *cur = &pool->bufs[pool->clock[clockIndex]];
            if (cur->pin == 0) {
                if (cur->usage == 0) {
                    // Use this buffer
                    buf = cur;
                    break;
                } else {
                    cur->usage = 0;
                }
            }
            clockIndex = (clockIndex + 1) % pool->nbufs;
        }  
        // Load the data into the buffer
        buf->pin = 1;
        buf->usage ++;
        strncpy(buf->id, id, MAXID - 1);
        // buf->data = read_from_disk(id); // or however you get the data
    }

    pool->nrequests++;
    if (buf != NULL) {
        pool->nhits++;
    }


    return slot;
}


/*****************************************************************************
 * operators
 *****************************************************************************/

_Table* sel(const UINT idx, const INT cond_val, const char* table_name){
    printf("sel() is invoked.\n");
    // invoke log_read_page() every time a page is read from the hard drive.
    // invoke log_release_page() every time a page is released from the memory.

    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.

    // testing
    // the following code constructs a synthetic _Table with 10 tuples and each tuple contains 4 attributes
    // examine log.txt to see the example outputs
    // replace all code with your implementation
    Conf*       cf = get_conf();
    Database*   db = get_db();
    BufPool     pool = initBufPool(cf->buf_slots, cf->buf_policy);
    Table       tb;
    UINT        pid;
    UINT        table_oid;
    char        buffer_id[MAXID];
    char        file_path[100];

    /*****************************************************************************
     * idx: the index to get the attribute, start from 0
     * cond_val: conditional value
     * table_name: get table via table name
     *****************************************************************************/
    for (int i = 0; i < db->ntables; i++){
        if (strcmp(table_name, db->tables[i].name) == 0){
            tb = db->tables[i];
            snprintf(file_path, sizeof(file_path), "%s/%s", db->path, db->tables[i].oid);
            table_oid = db->tables[i].oid;
            break;
        }   
    }

    // 打开file，怎么从table name获取file oid 
    // 读取table page，怎么获得pid
    // 读取page中tuple
    // 如何涉及到buffer？或许没有

    /*
    DB db = openDatabase("myDB");
    Rel r = openRelation(db,"Employee");
    Page buffer = malloc(PAGESIZE*sizeof(char));
    for (int i = 0; i < r->npages; i++) {
    PageId pid = r->start+i;
    get_page(db, pid, buffer);
    for each tuple in buffer {
    get tuple data and extract name
    add (name) to result tuples
    }
    }
    */

    // open table file
    FILE *table_fp = fopen(file_path, "rb");
    log_open_file(table_oid);
    
    // initialize result table
    UINT ntuples = tb.ntuples;
    UINT nattrs = tb.nattrs;
    _Table* result = malloc(sizeof(_Table) + ntuples * sizeof(Tuple));
    result->nattrs = nattrs;
    result->ntuples = ntuples;


    fread(&buf, sizeof(struct buffer), 1, table_fp);

    // buffer 里的id格式
    sprintf(buffer_id,"%c%d",table_name,pid);
    // 位置待定，应该要放在循环里，如果buffer满了就release，每次读page应该调用read
    log_read_page(pid);
    log_release_page(pid);


    for (UINT i = 0; i < ntuples; i++){
        Tuple tuple = table->tuples[i];
        if(tuple[idx] == cond_val){
            Tuple new_tuple = malloc(sizeof(INT) * table->nattrs);
            memcpy(new_tuple, tuple, sizeof(INT) * table->nattrs);
            result->tuples[result->ntuples++] = new_tuple;
        }
    // 已经写完result的内容结果
    }


    fclose(table_fp);
    log_close_file(table_oid);
    return result;
    // return NULL;
}

//  function for hash join
//  function for sort_merge join

_Table* join(const UINT idx1, const char* table1_name, const UINT idx2, const char* table2_name){
    printf("join() is invoked.\n");
    // write your code to join two tables
    // invoke log_read_page() every time a page is read from the hard drive.
    // invoke log_release_page() every time a page is released from the memory.

    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.
    Conf* cf = get_conf();
    Database* db = get_db();
    BufPool pool = initBufPool(cf->buf_slots, cf->buf_policy);
    Table       tb1, tb2;
    UINT        pid1, pid2;
    UINT        table1_oid, table2_oid;
    char        buffer_id[MAXID];
    char        file1_path[100], file2_path[100];

    for (int i = 0; i < db->ntables; i++){
        if (strcmp(table1_name, db->tables[i].name) == 0){
            tb1 = db->tables[i];
            snprintf(file1_path, sizeof(file1_path), "%s/%s", db->path, db->tables[i].oid);
            table1_oid = db->tables[i].oid;
        }

        if (strcmp(table2_name, db->tables[i].name) == 0){
            tb2 = db->tables[i];
            snprintf(file2_path, sizeof(file2_path), "%s/%s", db->path, db->tables[i].oid);
            table2_oid = db->tables[i].oid;
        }
    }

    if (cf.buf_slots >= T1.npages + T2.npages)
    
    else{
        
    }
    
    return NULL;
}
