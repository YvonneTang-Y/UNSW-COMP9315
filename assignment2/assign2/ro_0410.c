#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "ro.h"
#include "db.h"


#define MAXID 200
#define PATHZ_SIZE 200

/*****************************************************************************
 * file pool
 *****************************************************************************/
typedef struct filePool{
    FILE    **files;
    int     *freeList;
    bool     *usedList;      //0 - unused, 1 - used
    int     *table_oids;
	int     nfiles;         // how many files
    int     nvb;
}filePool;

filePool* initFilePool(int nfiles)
{
	filePool* newPool;

	newPool = malloc(sizeof(struct filePool));
	assert(newPool != NULL);
	newPool->nfiles = nfiles;
	newPool->files = malloc(nfiles * sizeof(FILE*));
	assert(newPool->files != NULL);
	newPool->freeList = malloc(nfiles * sizeof(int));
	assert(newPool->freeList != NULL);
	newPool->usedList = malloc(nfiles * sizeof(bool));
	assert(newPool->usedList != NULL);
	newPool->table_oids = malloc(nfiles * sizeof(int));
	assert(newPool->table_oids != NULL);    
	for (int i = 0; i < nfiles; i++) {
        newPool->freeList[i] = -1;
        newPool->usedList[i] = 0;
	}
    newPool->nvb = 0;
	return newPool;
}

int removeFirstFree(filePool *pool)
{
	int v, i;
	v = pool->freeList[0];
	for (i = 0; i < pool->nfiles-1; i++){
        pool->freeList[i] = pool->freeList[i+1];
    }
    pool->freeList[pool->nfiles - 1] = v;
	return v;
}

FILE* request_file(filePool *pool, char *file_path, UINT table_oid);

FILE* request_file(filePool *pool, char *file_path, UINT table_oid){
    if (pool->freeList[pool->nvb] >=0){
        fclose(pool->files[nvb]);
        log_close_file(pool->table_oids[nvb]);
    }
    FILE *newfile = fopen(file_path, "rb");
    pool->files[nvb] = newfile;
    pool->table_oids[nvb] = table_oid;
    log_open_file(table_oid);
    pool->nvb = (pool->nvb + 1) % pool->nfiles;
    int file_seq = removeFirstFree(pool);
    // if there is a open file in file pool, close it
    if (pool->usedList[file_seq]){
        fclose(pool->files[file_seq]);
        pool->usedList[file_seq] = 0;
        log_close_file(pool->table_oids[file_seq]);
    }
    FILE *newfile = fopen(file_path, "rb");
    pool->files[file_seq] = newfile;
    pool->usedList[file_seq] = 1;
    pool->table_oids[file_seq] = table_oid;
    log_open_file(table_oid);
    return newfile;
}

/*****************************************************************************
 * buffer management
 *****************************************************************************/
// one buffer
typedef struct buffer {
	// char  *id;
    char  id[MAXID];
	int   pin;
	int   usage;
	char  *data;
}buffer;

// collection of buffers + stats
typedef struct bufPool {
	int     nbufs;         // how many buffers
	struct  buffer *bufs;
    int     nvb;
}bufPool;

typedef struct bufPool *BufPool;

BufPool initBufPool(int nbufs)
{
	BufPool newPool;
	// struct buffer *bufs;

	newPool = malloc(sizeof(struct bufPool));
	assert(newPool != NULL);
	newPool->nbufs = nbufs;
	newPool->bufs = malloc(nbufs * sizeof(struct buffer));
	assert(newPool->bufs != NULL);

	int i;
	for (i = 0; i < nbufs; i++) {
		newPool->bufs[i].id[0] = '\0';
		newPool->bufs[i].pin = 0;
		newPool->bufs[i].usage = 0;
	}
    newPool->nvb = 0;
	return newPool;
}

int request_page(BufPool pool, const char *rel, UINT64 pid, UINT offset, FILE *table_fp);
void release_page(BufPool pool, const char *rel, UINT64 pid);


// pageInPool(BufPool pool, const char *rel, UINT64 pid)
// - check whether page from rel is already in the pool
// - returns the slot containing this page, else returns -1
int pageInPool(BufPool pool, const char *rel, UINT64 pid)
{
	int i;  char id[MAXID];
	sprintf(id, "%s%ld", rel, pid);
	for (i = 0; i < pool->nbufs; i++) {
		if (strcmp(id, pool->bufs[i].id) == 0) {
			return i;
		}
	}
	return -1;
}

// request page function
int request_page(BufPool pool, const char *rel, UINT64 pid, UINT page_size, FILE *table_fp){
    int slot; char id[MAXID];
    sprintf(id, "%s%ld", rel, pid);
    struct buffer *buf = NULL;
    slot = pageInPool(pool,rel,pid);
    // page is in buffer
    if (slot >= 0){
        buf = &pool->bufs[slot];
        buf->pin = 1;
        buf->usage ++;
        return slot;
    }

    // clock sweep strategy
    while (1) {
        buf = &pool->bufs[pool->nvb];
        if (buf->pin == 0 && buf->usage == 0){
            // read page from disk to buffer[nvb]
            buf->data = (char*) malloc(page_size * sizeof(char));
            fseek(table_fp, pid * page_size, SEEK_SET);
            fread(buf->data, page_size, 1, table_fp);
            log_read_page(pid);

            // grab buffer
            strcpy(buf->id, id);
            buf->pin = 1;
            buf->usage ++;
            slot = pool->nvb;
            pool->nvb = (pool->nvb + 1) % pool->nbufs;
            return slot;
        }
        else{
            if(buf->usage > 0){
                buf->usage--;
            }
            pool->nvb = (pool->nvb + 1) % pool->nbufs;
        }
    }
}

// release page function
void release_page(BufPool pool, const char *rel, UINT64 pid){
    int slot;
    struct buffer *buf = NULL;
    slot = pageInPool(pool,rel,pid);
    if (slot >= 0){
        buf = &pool->bufs[slot];
        buf->pin = 0;
        log_release_page(pid);
    }
}

/*****************************************************************************
 * init function and release function
 *****************************************************************************/

// global varible
filePool* file_pool;
BufPool     pool;

void init(){
    // do some initialization here.

    // example to get the Conf pointer
    // Conf* cf = get_conf();

    // example to get the Database pointer
    // Database* db = get_db();
    
    Conf* cf = get_conf();
    Database* db = get_db();
    file_pool = initFilePool(cf->file_limit);
    pool = initBufPool(cf->buf_slots);
    printf("init() is invoked.\n");
}

void release(){
    // optional
    // do some end tasks here.
    // free space to avoid memory leak
    printf("release() is invoked.\n");
}



/*****************************************************************************
 * selection operator
 *****************************************************************************/

_Table* sel(const UINT idx, const INT cond_val, const char* table_name){
    printf("sel() is invoked.\n");
    Conf* cf = get_conf();
    Database* db = get_db();
    Table       tb;
    // UINT        pid;
    UINT        table_oid;
    // char        buffer_id[MAXID];
    char        file_path[PATHZ_SIZE]; //sizeof(db->path) = 100, sizeof(table.oid) = 4
    UINT        npages;
    UINT        ntuples_per_page;
    UINT        ntuples_last_page;
    char        *tuple;
    INT         cond_attr;
    struct buffer *buf;

    /*****************************************************************************
     * idx: the index to get the attribute, start from 0
     * cond_val: conditional value
     * table_name: get table via table name
     *****************************************************************************/

    // get table_oid
    for (int i = 0; i < db->ntables; i++){
        if (strcmp(table_name, db->tables[i].name) == 0){
            tb = db->tables[i];
            snprintf(file_path, sizeof(file_path), "%s/%d", db->path, db->tables[i].oid);
            table_oid = db->tables[i].oid;
            break;
        }   
    }

    // get page number
    ntuples_per_page = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / tb.nattrs;
    if (tb.ntuples % ntuples_per_page == 0){
        ntuples_last_page = ntuples_per_page;
        npages = tb.ntuples / ntuples_per_page;
    }
    else{
        ntuples_last_page = tb.ntuples % ntuples_per_page;
        npages = tb.ntuples / ntuples_per_page + 1;
    }

    // open table file
    FILE *table_fp = request_file(file_pool, file_path, table_oid);

    // initialize result table
    _Table* result = malloc(sizeof(_Table) + tb.ntuples * sizeof(Tuple));
    result->nattrs = tb.nattrs;

    // selection
    UINT ntuples_page;
    int tuple_seq = 0, slot, count = 0;
    for (UINT i = 0; i < npages; i++){
        slot = request_page(pool, table_name, i, cf->page_size, table_fp);
        buf = &pool->bufs[slot];
        // check tuple number of each page
        if (i < npages - 1){
            ntuples_page = ntuples_per_page;   
        }
        else{
            ntuples_page = ntuples_last_page;   
        }

        // read each tuple from buffer
        for (int j = 0; j < ntuples_page; j++){
            // header and j tuples before at this page
            tuple = buf->data + sizeof(UINT64) + j * sizeof(INT) * tb.nattrs;
            cond_attr = *(INT *)(tuple + sizeof(INT) * idx);
            if (cond_attr == cond_val){
                // write tuple into result
                Tuple new_tuple = malloc(sizeof(INT) * tb.nattrs);
                result->tuples[tuple_seq] = new_tuple;
                for (int k = 0; k < tb.nattrs; k++){
                    new_tuple[k] = *(INT *)(tuple + sizeof(INT) * k);
                }
                tuple_seq++;
                count++;
            }
        }
        release_page(pool, table_name, i);
    }
    result->ntuples = count;
    return result;
    // return NULL;
}

/*****************************************************************************
 * join operator
 *****************************************************************************/

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
    Table       tb1, tb2;
    // UINT        pid1, pid2;
    UINT        table_oid1, table_oid2;
    // char        buffer_id[MAXID];
    char        file_path1[PATHZ_SIZE], file_path2[PATHZ_SIZE];
    UINT        npages1, npages2;
    UINT        ntuples_per_page1, ntuples_per_page2;
    UINT        ntuples_last_page1, ntuples_last_page2;
    char        *tuple1;
    char        *tuple2;
    UINT        ntuples_page1, ntuples_page2;
    int         count = 0, tuple_seq = 0;
    int         slot1, slot2;
    INT         cond_attr1, cond_attr2;

    struct buffer *buf1;
    struct buffer *buf2;
    

    // get table_oid
    for (int i = 0; i < db->ntables; i++){
        if (strcmp(table1_name, db->tables[i].name) == 0){
            tb1 = db->tables[i];
            snprintf(file_path1, sizeof(file_path1), "%s/%d", db->path, db->tables[i].oid);
            table_oid1 = db->tables[i].oid;
        }

        if (strcmp(table2_name, db->tables[i].name) == 0){
            tb2 = db->tables[i];
            snprintf(file_path2, sizeof(file_path2), "%s/%d", db->path, db->tables[i].oid);
            table_oid2 = db->tables[i].oid;
        }
    }

    // initialize result table
    _Table* result = malloc(sizeof(_Table) + (tb1.ntuples + tb2.ntuples) * sizeof(Tuple));
    result->nattrs = tb1.nattrs + tb2.nattrs;

    // get page number
    ntuples_per_page1 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / tb1.nattrs;
    if (tb1.ntuples % ntuples_per_page1 == 0){
        ntuples_last_page1 = ntuples_per_page1;
        npages1 = tb1.ntuples / ntuples_per_page1;
    }
    else{
        ntuples_last_page1 = tb1.ntuples % ntuples_per_page1;
        npages1 = tb1.ntuples / ntuples_per_page1 + 1;
    }

    ntuples_per_page2 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / tb2.nattrs;
    if (tb2.ntuples % ntuples_per_page2 == 0){
        ntuples_last_page2 = ntuples_per_page2;
        npages2 = tb2.ntuples / ntuples_per_page2;
    }
    else{
        ntuples_last_page2 = tb2.ntuples % ntuples_per_page2;
        npages2 = tb2.ntuples / ntuples_per_page2 + 1;
    }

    // open table file
    FILE *table_fp1 = request_file(file_pool, file_path1, table_oid1);
    FILE *table_fp2 = request_file(file_pool, file_path2, table_oid2);
    
    // nested for-loop join
    if (cf->buf_slots < npages1 + npages2){
        int nout_loops, last_out_loop, out_loop;
        int slot_list[cf->buf_slots - 1];       // store slot sequence

        if (npages1 % (cf->buf_slots - 1) == 0){
            nout_loops = npages1 / (cf->buf_slots - 1);
            last_out_loop = cf->buf_slots - 1;
        }
        else{
            nout_loops = npages1 / (cf->buf_slots - 1) + 1;
            last_out_loop = npages1 % (cf->buf_slots - 1);
        }

        // bufferpool n-1 for outer table, 1 for inner table
        BufPool     pool1 = initBufPool(cf->buf_slots - 1);
        BufPool     pool2 = initBufPool(1);

        // nout_loops = ceil(bR / (N-1))
        for(int loop = 0; loop < nout_loops; loop++){
            if (loop == nout_loops - 1){
                out_loop = last_out_loop;
            }
            else{
                out_loop = cf->buf_slots - 1;
            }
            // request outer table page
            for (UINT i = 0; i < out_loop; i++){
                slot1 = request_page(pool1, table1_name, loop * (cf->buf_slots - 1) + i, cf->page_size, table_fp1);
                buf1 = &pool1->bufs[slot1];
                slot_list[i] = slot1;
            }
            
            // request inner table page
            for (UINT j = 0; j < npages2; j++){
                slot2 = request_page(pool2, table2_name, j, cf->page_size, table_fp2);
                buf2 = &pool2->bufs[slot2]; 
                if (j < npages2 - 1){
                    ntuples_page2 = ntuples_per_page2;   
                }
                else{
                    ntuples_page2 = ntuples_last_page2;   
                }
                for (UINT i = 0; i < out_loop; i++){
                    buf1 = &pool1->bufs[slot_list[i]];
                    // the last page in outer table
                    if (loop == nout_loops - 1 && i == out_loop - 1){
                        ntuples_page1 = ntuples_last_page1; 
                    }
                    else{
                        ntuples_page1 = ntuples_per_page1;
                    }
                    for (int m = 0; m < ntuples_page1; m++){
                        // header and m tuples before at this page
                        tuple1 = buf1->data + sizeof(UINT64) + m * sizeof(INT) * tb1.nattrs;
                        cond_attr1 = *(INT *)(tuple1 + sizeof(INT) * idx1);
                        for (int n = 0; n < ntuples_page2; n++){
                            // header and n tuples before at this page
                            tuple2 = buf2->data + sizeof(UINT64) + n * sizeof(INT) * tb2.nattrs;
                            cond_attr2= *(INT *)(tuple2 + sizeof(INT) * idx2);
                            if (cond_attr1 == cond_attr2){
                                // write tuple into result
                                Tuple new_tuple = malloc(sizeof(INT) * (tb1.nattrs + tb2.nattrs));
                                result->tuples[tuple_seq] = new_tuple;
                                for (int k = 0; k < tb1.nattrs; k++){
                                    new_tuple[k] = *(INT *)(tuple1 + sizeof(INT) * k);
                                }
                                for (int k = 0; k < tb2.nattrs; k++){
                                    new_tuple[tb1.nattrs + k] = *(INT *)(tuple2 + sizeof(INT) * k);
                                }                            
                                tuple_seq++;
                                count++;
                            }
                        }
                    }
                }

                release_page(pool2, table2_name, j);   
            }
            // release outer table page
            for (UINT i = 0; i < out_loop; i++){
                release_page(pool1, table1_name, slot_list[i]);
            }
        }

        result->ntuples = count;
        return result;
    }
    // hash join or sort-merge join
    // else{
    //     BufPool     pool = initBufPool(cf->buf_slots);
        
    // }
    return NULL;
}
