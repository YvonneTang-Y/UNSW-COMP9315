#include <stdio.h>
#include <stdlib.h>
#include "ro.h"
#include "db.h"

void init(){
    // do some initialization here.

    // example to get the Conf pointer
    // Conf* cf = get_conf();

    // example to get the Database pointer
    // Database* db = get_db();
    
    printf("init() is invoked.\n");
}

void release(){
    // optional
    // do some end tasks here.
    // free space to avoid memory leak
    printf("release() is invoked.\n");
}

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

    UINT ntuples = 10;
    UINT nattrs = 4;

    _Table* result = malloc(sizeof(_Table)+ntuples*sizeof(Tuple));
    result->nattrs = nattrs;
    result->ntuples = ntuples;

    INT value = 0;
    for (UINT i = 0; i < result->ntuples; i++){
        Tuple t = malloc(sizeof(INT)*result->nattrs);
        result->tuples[i] = t;
        for (UINT j = 0; j < result->nattrs; j++){
            t[j] = value;
            ++value;
        }
    }
    
    return result;

    // return NULL;
}



_Table* join(const UINT idx1, const char* table1_name, const UINT idx2, const char* table2_name){

    printf("join() is invoked.\n");
    // write your code to join two tables
    // invoke log_read_page() every time a page is read from the hard drive.
    // invoke log_release_page() every time a page is released from the memory.

    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.
    
    
    return NULL;
}
