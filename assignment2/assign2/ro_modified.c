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
 * a similar but simpler strategy than clock sweep
 * when a file slot is not in use, set the relevant freelist flag is -1
 * then this slot can be released and open new file
 * when a file slot is in use, set the relevant freelist flag is 1
 * set nvb to check the cycle, and when the freelist flag is -1, grab this slot
 * 
 * use sort-merge join when the buffer slots is enough
 *****************************************************************************/
typedef struct filePool{
    FILE    **files;
    int     *freeList;
    int     *table_oids;
	int     nfiles;         // how many files
    int     nvb;
}filePool;

// initialize filepool
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
	newPool->table_oids = malloc(nfiles * sizeof(int));
	assert(newPool->table_oids != NULL);    
	for (int i = 0; i < nfiles; i++) {
        newPool->freeList[i] = -1;
        newPool->table_oids[i] = -1;
	}
    newPool->nvb = 0;
	return newPool;
}

// request file slot from filepool
FILE* request_file(filePool *pool, char *file_path, UINT table_oid){
    // if the file has been open
    for (int i = 0; i < pool->nfiles; i++){
        if (pool->table_oids[i] == table_oid){
            return pool->files[i];
        }
    }
    // if there is a open file in file pool, close it
    if (pool->freeList[pool->nvb] >=0){
        fclose(pool->files[pool->nvb]);
        log_close_file(pool->table_oids[pool->nvb]);
    }
    FILE *newfile = fopen(file_path, "rb");
    log_open_file(table_oid);
    pool->freeList[pool->nvb] = 1;
    pool->files[pool->nvb] = newfile;
    pool->table_oids[pool->nvb] = table_oid;
    pool->nvb = (pool->nvb + 1) % pool->nfiles;
    return newfile;
}

// free filepool to avoid heap leak
void releaseFilePool(filePool* pool){
    for (int i = 0; i < pool->nfiles; i++){
        if (pool->freeList[i] >=0){
            fclose(pool->files[i]);
        }
    }
    free(pool->files);
    free(pool->freeList);
    free(pool->table_oids);
    
    free(pool);
}

/*****************************************************************************
 * buffer management
 *****************************************************************************/
// one buffer
typedef struct buffer {
	// char  *id;
    char    id[MAXID];
    UINT64  pid;
	int     pin;
	int     usage;
	char    *data;
}buffer;

// collection of buffers
typedef struct bufPool {
	int     nbufs;         // how many buffers
	struct  buffer *bufs;
    // int     nvb;
}bufPool;

typedef struct bufPool *BufPool;

// initialize bufferpool
BufPool initBufPool(int nbufs, UINT page_size)
{
	BufPool newPool;

	newPool = malloc(sizeof(struct bufPool));
	assert(newPool != NULL);
	newPool->nbufs = nbufs;
	newPool->bufs = malloc(nbufs * sizeof(struct buffer));
	assert(newPool->bufs != NULL);

	int i;
	for (i = 0; i < nbufs; i++) {
		newPool->bufs[i].id[0] = '\0';
        newPool->bufs[i].pid = 0;
		newPool->bufs[i].pin = 0;
		newPool->bufs[i].usage = 0;
        newPool->bufs[i].data = (char*) malloc(page_size * sizeof(char));
	}
    // newPool->nvb = 0;
	return newPool;
}

// free bufferpool to avoid heap leak
void releaseBufPool(BufPool pool){
    for (int i = 0; i < pool->nbufs; i++){
        free(pool->bufs[i].data);
    }
    free(pool->bufs);
    free(pool);
}


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
int request_page(BufPool pool, const char *rel, int pnum, UINT64 pid, UINT page_size, FILE *table_fp, int* nvb, int start_slot, int end_slot){
    // use start_slot and end_slot to 'divide' pool into several parts
    int slot, range_size; char id[MAXID];
    UINT64 old_pid;
    sprintf(id, "%s%ld", rel, pid);
    struct buffer *buf = NULL;
    if (start_slot < 0 && end_slot < 0){
        start_slot = 0;
        end_slot = pool->nbufs - 1;
    }
    range_size = end_slot - start_slot + 1;
    slot = pageInPool(pool, rel, pid);
    // page is in buffer
    if (slot >= 0 && slot >= start_slot && slot <= end_slot){
        buf = &pool->bufs[slot];
        buf->pin = 1;
        buf->usage ++;
        return slot;
    }

    // clock sweep strategy
    while (1){
        buf = &pool->bufs[start_slot + *nvb];
        if (buf->pin == 0 && buf->usage == 0){
            old_pid = buf->pid;
            // if there has an old page in the buffer, release it
            if (buf->id[0] != '\0'){
                log_release_page(old_pid);
            }
            // read page from disk to buffer[nvb]
            fseek(table_fp, pnum * page_size, SEEK_SET);
            fread(buf->data, page_size, 1, table_fp);
            log_read_page(pid);
            // grab buffer
            strcpy(buf->id, id);
            buf->pid = pid;
            buf->pin = 1;
            buf->usage ++;
            slot = start_slot + *nvb;
            // pool->nvb = (pool->nvb + 1) % pool->nbufs;
            *nvb = (*nvb + 1) % range_size;          
            return slot;
        }
        else{
            if(buf->usage > 0){
                buf->usage--;
            }
            *nvb = (*nvb + 1) % range_size;
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
    }
}

/*****************************************************************************
 * data structure for sort-merge join
 *****************************************************************************/

typedef struct TupleWithAttr{
    char *tuple;
    INT attr_value;
} TupleWithAttr;

/*****************************************************************************
 * init function and release function
 *****************************************************************************/

// global varible
filePool    * file_pool;
BufPool     pool;
Conf        *cf_ro;
Database    *db_ro;
int         start_nvb = 0, start_nvb1 = 0, start_nvb2 = 0;
int         *start_nvbp, *start_nvbp1, *start_nvbp2;

void init(){
    // do some initialization here.

    // example to get the Conf pointer
    // Conf* cf = get_conf();

    // example to get the Database pointer
    // Database* db_ro = get_db();
    
    cf_ro = get_conf();
    db_ro = get_db();
    file_pool = initFilePool(cf_ro->file_limit);
    pool = initBufPool(cf_ro->buf_slots, cf_ro->page_size);
    start_nvbp = &start_nvb;
    start_nvbp1 = &start_nvb1;
    start_nvbp2 = &start_nvb2;
    printf("init() is invoked.\n");
}

void release(){
    // optional
    // do some end tasks here.
    // free space to avoid memory leak
    releaseFilePool(file_pool);
    releaseBufPool(pool);

    printf("release() is invoked.\n");
}



/*****************************************************************************
 * selection operator
 *****************************************************************************/

_Table* sel(const UINT idx, const INT cond_val, const char* table_name){
    printf("sel() is invoked.\n");
    Table       tb;
    UINT        table_oid;
    char        file_path[PATHZ_SIZE]; //sizeof(db_ro->path) = 100, sizeof(table.oid) = 4
    UINT        npages;
    UINT        ntuples_per_page;
    UINT        ntuples_last_page;
    char        *tuple;
    INT         cond_attr;
    UINT64      pid;
    struct buffer *buf;
    

    /*****************************************************************************
     * idx: the index to get the attribute, start from 0
     * cond_val: conditional value
     * table_name: get table via table name
     *****************************************************************************/

    // get table_oid
    for (int i = 0; i < db_ro->ntables; i++){
        if (strcmp(table_name, db_ro->tables[i].name) == 0){
            tb = db_ro->tables[i];
            snprintf(file_path, sizeof(file_path), "%s/%d", db_ro->path, db_ro->tables[i].oid);
            table_oid = db_ro->tables[i].oid;
            break;
        }  
    }

    // initialize result table
    _Table* result = malloc(sizeof(_Table) + tb.ntuples * sizeof(Tuple));
    result->nattrs = tb.nattrs;

    // if there is an empty table
    if (tb.nattrs == 0 || tb.ntuples == 0){
        result->ntuples = 0;
        return result;
    }
    // get page number
    ntuples_per_page = (cf_ro->page_size - sizeof(UINT64)) / sizeof(INT) / tb.nattrs;
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

    // do selection
    UINT ntuples_page;
    int tuple_seq = 0, slot, count = 0;
    for (UINT i = 0; i < npages; i++){
        // get pid
        fseek(table_fp, i * cf_ro->page_size, SEEK_SET);
        fread(&pid,sizeof(UINT64),1,table_fp);
        slot = request_page(pool, table_name, i, pid, cf_ro->page_size, table_fp, start_nvbp, -1, -1);
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
        release_page(pool, table_name, pid);
    }
    result->ntuples = count;
    return result;
    // return NULL;
}

/*****************************************************************************
 * join operator
 *****************************************************************************/

//  function for sort_merge join
int compare_tuples(const void* a, const void* b){
    TupleWithAttr* tuple1 = (TupleWithAttr*) a;
    TupleWithAttr* tuple2 = (TupleWithAttr*) b;
    if (tuple1->attr_value < tuple2->attr_value) {
        return -1;
    } else if (tuple1->attr_value > tuple2->attr_value) {
        return 1;
    } else {
        return 0;
    }
}

_Table* join(const UINT idx1, const char* table1_name, const UINT idx2, const char* table2_name){
    printf("join() is invoked.\n");
    // write your code to join two tables
    // invoke log_read_page() every time a page is read from the hard drive.
    // invoke log_release_page() every time a page is released from the memory.

    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.

    Table       tb1, tb2;
    UINT        table_oid1, table_oid2;
    char        file_path1[PATHZ_SIZE], file_path2[PATHZ_SIZE];
    UINT        npages1, npages2;
    UINT        ntuples_per_page1, ntuples_per_page2;
    UINT        ntuples_last_page1, ntuples_last_page2;
    char        *tuple1;
    char        *tuple2;
    UINT        ntuples_page1, ntuples_page2;
    UINT        count = 0;
    int         tuple_seq = 0;
    int         slot1, slot2;
    INT         cond_attr1, cond_attr2;
    FILE        *table_fp1;
    FILE        *table_fp2;
    UINT64      pid1,pid2;

    struct buffer *buf1;
    struct buffer *buf2;

    *start_nvbp1 = (*start_nvbp) % (cf_ro->buf_slots - 1);
    // get table_oid
    int count_table = 0;
    for (int i = 0; i < db_ro->ntables; i++){
        if (strcmp(table1_name, db_ro->tables[i].name) == 0){
            tb1 = db_ro->tables[i];
            snprintf(file_path1, sizeof(file_path1), "%s/%d", db_ro->path, db_ro->tables[i].oid);
            table_oid1 = db_ro->tables[i].oid;
            count_table++;
        }

        if (strcmp(table2_name, db_ro->tables[i].name) == 0){
            tb2 = db_ro->tables[i];
            snprintf(file_path2, sizeof(file_path2), "%s/%d", db_ro->path, db_ro->tables[i].oid);
            table_oid2 = db_ro->tables[i].oid;
            count_table++;
        }
    }

    // initialize result table
    _Table* result = malloc(sizeof(_Table) + (tb1.ntuples + tb2.ntuples) * sizeof(Tuple));
    result->nattrs = tb1.nattrs + tb2.nattrs;

    // if there is an empty table
    if (tb1.nattrs == 0 || tb1.ntuples == 0 || tb2.nattrs == 0 || tb2.ntuples == 0){
        result->ntuples = 0;
        return result;
    }
    // get page number
    ntuples_per_page1 = (cf_ro->page_size - sizeof(UINT64)) / sizeof(INT) / tb1.nattrs;
    if (tb1.ntuples % ntuples_per_page1 == 0){
        ntuples_last_page1 = ntuples_per_page1;
        npages1 = tb1.ntuples / ntuples_per_page1;
    }
    else{
        ntuples_last_page1 = tb1.ntuples % ntuples_per_page1;
        npages1 = tb1.ntuples / ntuples_per_page1 + 1;
    }

    ntuples_per_page2 = (cf_ro->page_size - sizeof(UINT64)) / sizeof(INT) / tb2.nattrs;
    if (tb2.ntuples % ntuples_per_page2 == 0){
        ntuples_last_page2 = ntuples_per_page2;
        npages2 = tb2.ntuples / ntuples_per_page2;
    }
    else{
        ntuples_last_page2 = tb2.ntuples % ntuples_per_page2;
        npages2 = tb2.ntuples / ntuples_per_page2 + 1;
    }

    // open table file
    // nested for-loop join
    if (cf_ro->buf_slots < npages1 + npages2){
        int         nout_loops, last_out_loop, out_loop;
        int         slot_list[cf_ro->buf_slots - 1];       // store slot sequence
        char        *table_name_outer;
        char        *table_name_inner;
        int         nio1, nio2;

        // determin which table is the outer table and which table is the inner table
        nio1 = npages1 + (npages1 + cf_ro->buf_slots - 2) / (cf_ro->buf_slots - 1) * npages2; // table1 is outer table
        nio2 = npages2 + (npages2 + cf_ro->buf_slots - 2) / (cf_ro->buf_slots - 1) * npages1; // table2 is outer table
        if (nio1 <= nio2){
            table_name_outer = (char *) malloc(strlen(table1_name) + 1);
            strcpy(table_name_outer, table1_name);
            table_name_inner = (char *) malloc(strlen(table2_name) + 1);
            strcpy(table_name_inner, table2_name);
            table_fp1 = request_file(file_pool, file_path1, table_oid1);
            table_fp2 = request_file(file_pool, file_path2, table_oid2);
        }
        else{
            table_name_outer = (char *) malloc(strlen(table2_name) + 1);
            strcpy(table_name_outer, table2_name);
            table_name_inner = (char *) malloc(strlen(table1_name) + 1);
            strcpy(table_name_inner, table1_name);
            table_fp1 = request_file(file_pool, file_path2, table_oid2);
            table_fp2 = request_file(file_pool, file_path1, table_oid1);
        }

        // get table_oid
        for (int i = 0; i < db_ro->ntables; i++){
            if (strcmp(table_name_outer, db_ro->tables[i].name) == 0){
                tb1 = db_ro->tables[i];
                snprintf(file_path1, sizeof(file_path1), "%s/%d", db_ro->path, db_ro->tables[i].oid);
                table_oid1 = db_ro->tables[i].oid;
            }

            if (strcmp(table_name_inner, db_ro->tables[i].name) == 0){
                tb2 = db_ro->tables[i];
                snprintf(file_path2, sizeof(file_path2), "%s/%d", db_ro->path, db_ro->tables[i].oid);
                table_oid2 = db_ro->tables[i].oid;
            }
        }

        // get page number
        ntuples_per_page1 = (cf_ro->page_size - sizeof(UINT64)) / sizeof(INT) / tb1.nattrs;
        if (tb1.ntuples % ntuples_per_page1 == 0){
            ntuples_last_page1 = ntuples_per_page1;
            npages1 = tb1.ntuples / ntuples_per_page1;
        }
        else{
            ntuples_last_page1 = tb1.ntuples % ntuples_per_page1;
            npages1 = tb1.ntuples / ntuples_per_page1 + 1;
        }

        ntuples_per_page2 = (cf_ro->page_size - sizeof(UINT64)) / sizeof(INT) / tb2.nattrs;
        if (tb2.ntuples % ntuples_per_page2 == 0){
            ntuples_last_page2 = ntuples_per_page2;
            npages2 = tb2.ntuples / ntuples_per_page2;
        }
        else{
            ntuples_last_page2 = tb2.ntuples % ntuples_per_page2;
            npages2 = tb2.ntuples / ntuples_per_page2 + 1;
        }

        // caculate outer loop time
        if (npages1 % (cf_ro->buf_slots - 1) == 0){
            nout_loops = npages1 / (cf_ro->buf_slots - 1);
            last_out_loop = cf_ro->buf_slots - 1;
        }
        else{
            nout_loops = npages1 / (cf_ro->buf_slots - 1) + 1;
            last_out_loop = npages1 % (cf_ro->buf_slots - 1);
        }

        // nout_loops = ceil(bR / (N-1))
        for(int loop = 0; loop < nout_loops; loop++){

            if (loop == nout_loops - 1){
                out_loop = last_out_loop;
            }
            else{
                out_loop = cf_ro->buf_slots - 1;
            }
            

            // request outer table page
            for (UINT i = 0; i < out_loop; i++){
                // bufferpool n-1 for outer table, 1 for inner table
                fseek(table_fp1, (loop * (cf_ro->buf_slots - 1) + i) * cf_ro->page_size, SEEK_SET);
                fread(&pid1,sizeof(UINT64),1,table_fp1);
                slot1 = request_page(pool, table_name_outer, loop * (cf_ro->buf_slots - 1) + i, pid1, cf_ro->page_size, table_fp1, start_nvbp1, 0, cf_ro->buf_slots - 2);
                buf1 = &pool->bufs[slot1];
                slot_list[i] = slot1; 
            }

            // request inner table page
            for (UINT j = 0; j < npages2; j++){
                fseek(table_fp2, j * cf_ro->page_size, SEEK_SET);
                fread(&pid2,sizeof(UINT64),1,table_fp2);
                slot2 = request_page(pool, table_name_inner, j, pid2, cf_ro->page_size, table_fp2, start_nvbp2, cf_ro->buf_slots - 1, cf_ro->buf_slots - 1);
                buf2 = &pool->bufs[slot2]; 
                if (j < npages2 - 1){
                    ntuples_page2 = ntuples_per_page2;   
                }
                else{
                    ntuples_page2 = ntuples_last_page2;   
                }
                for (UINT i = 0; i < out_loop; i++){
                    
                    buf1 = &pool->bufs[slot_list[i]];
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
                                if (nio1 <= nio2){
                                    for (int k = 0; k < tb1.nattrs; k++){
                                        new_tuple[k] = *(INT *)(tuple1 + sizeof(INT) * k);
                                    }
                                    for (int k = 0; k < tb2.nattrs; k++){
                                        new_tuple[tb1.nattrs + k] = *(INT *)(tuple2 + sizeof(INT) * k);
                                    } 
                                }
                                else{
                                    for (int k = 0; k < tb2.nattrs; k++){
                                        new_tuple[k] = *(INT *)(tuple2 + sizeof(INT) * k);
                                    }
                                    for (int k = 0; k < tb1.nattrs; k++){
                                        new_tuple[tb2.nattrs + k] = *(INT *)(tuple1 + sizeof(INT) * k);
                                    }                                     
                                }
                           
                                tuple_seq++;
                                count++;
                            }
                        }
                    }
                }
                
                release_page(pool, table_name_inner, pid2);  
            }

            // release outer table page
            for (UINT i = 0; i < out_loop; i++){
                fseek(table_fp1, (loop * (cf_ro->buf_slots - 1) + i) * cf_ro->page_size, SEEK_SET);
                fread(&pid1,sizeof(UINT64),1,table_fp1);           
                release_page(pool, table_name_outer, pid1);
            }
        }
        result->ntuples = count;
        *start_nvbp = *start_nvbp1;
        free(table_name_outer);
        free(table_name_inner);
        if (count) return result;
    }

    else{
        // sort-merge join
        int         result_seq = 0,tuple_seq1 = 0, tuple_seq2 = 0;
        UINT        count = 0;
        char        *tuple;

        TupleWithAttr* sort1 = malloc(sizeof(TupleWithAttr) * tb1.ntuples);
        TupleWithAttr* sort2 = malloc(sizeof(TupleWithAttr) * tb2.ntuples);

        table_fp1 = request_file(file_pool, file_path1, table_oid1);
        table_fp2 = request_file(file_pool, file_path2, table_oid2);
        // read all the pages to buffer
        for (UINT i = 0; i < npages1; i++){
            fseek(table_fp1, i * cf_ro->page_size, SEEK_SET);
            fread(&pid1,sizeof(UINT64),1,table_fp1);
            slot1 = request_page(pool, table1_name, i, pid1, cf_ro->page_size, table_fp1, start_nvbp, -1, -1);
            buf1 = &pool->bufs[slot1];
            if (i < npages1 - 1){
                ntuples_page1 = ntuples_per_page1;   
            }
            else{
                ntuples_page1 = ntuples_last_page1;   
            }
            for (int m = 0; m < ntuples_page1; m++){
                // header and m tuples before at this page
                tuple = buf1->data + sizeof(UINT64) + m * sizeof(INT) * tb1.nattrs;
                cond_attr1 = *(INT *)(tuple + sizeof(INT) * idx1);

                // write tuple into result
                TupleWithAttr* new_tuple = &(sort1[tuple_seq1]);
                new_tuple->tuple = tuple;
                new_tuple->attr_value = cond_attr1;
                tuple_seq1++;
            }
        }
        for (UINT j = 0; j < npages2; j++){
            fseek(table_fp2, j * cf_ro->page_size, SEEK_SET);
            fread(&pid2,sizeof(UINT64),1,table_fp2);
            slot2 = request_page(pool, table2_name, j, pid2, cf_ro->page_size, table_fp2, start_nvbp, -1, -1);
            buf2 = &pool->bufs[slot2]; 
            if (j < npages2 - 1){
                ntuples_page2 = ntuples_per_page2;   
            }
            else{
                ntuples_page2 = ntuples_last_page2;   
            }
            for (int n = 0; n < ntuples_page2; n++){
                // header and n tuples before at this page
                tuple = buf2->data + sizeof(UINT64) + n * sizeof(INT) * tb2.nattrs;
                cond_attr2 = *(INT *)(tuple + sizeof(INT) * idx2);
                
                // write tuple into result
                TupleWithAttr* new_tuple = &(sort2[tuple_seq2]);
                new_tuple->tuple = tuple;
                new_tuple->attr_value = cond_attr2;
                tuple_seq2++;
            }
        }

        qsort(sort1, tuple_seq1, sizeof(TupleWithAttr), compare_tuples);
        qsort(sort2, tuple_seq2, sizeof(TupleWithAttr), compare_tuples);

        // use two cursors to find all the tuples
        UINT i = 0, j = 0, start_flag = 0;
        while (i < tb1.ntuples && j < tb2.ntuples){
            if (sort1[i].attr_value < sort2[j].attr_value){
                i++;
            }
            else if (sort1[i].attr_value > sort2[j].attr_value){
                j++;
            }
            else {
                start_flag = j;
                while (i < tb1.ntuples && sort1[i].attr_value == sort2[j].attr_value){
                    while (j < tb2.ntuples && sort1[i].attr_value == sort2[j].attr_value){
                        Tuple new_tuple = malloc(sizeof(INT) * (tb1.nattrs + tb2.nattrs));
                        result->tuples[result_seq] = new_tuple;
                        for (int k = 0; k < tb1.nattrs; k++){
                            new_tuple[k] = *(INT *)(sort1[i].tuple + sizeof(INT) * k);
                        }
                        for (int k = 0; k < tb2.nattrs; k++){
                            new_tuple[tb1.nattrs + k] = *(INT *)(sort2[j].tuple + sizeof(INT) * k);
                        }                            
                        result_seq++;
                        count++;   
                        j++;                     
                    }
                    j = start_flag;
                    i++;
                }
            }

        }
        free(sort1);
        free(sort2);
        result->ntuples = count;
        if (count) return result; 
    }
    return NULL;
}
