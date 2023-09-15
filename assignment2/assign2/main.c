// COMP9315 23T1 Assignment 2
// Implementing relation algebra operators and memory buffer management

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
// #include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "db.h"
#include "ro.h"


void run(char* ra_path, char* log_path);
void freeT(_Table* t);
void logT(_Table* t, FILE* log_fp);

int main(int argc, char **argv){
    // argv[1] int: page size
    // argv[2] int: buffer size
    // argv[3] int: limit for opened files
    // argv[4] string: buffer page replacement policy
    // argv[5] string: path for the database
    // argv[6] string: path for data file
    // argv[7] string: path for test cases
    // argv[8] string: path for output log

    if (argc < 8) {
        printf("Insufficient arguments\n");
        return -1;
    }

    // load configurations
    UINT page_size,buf_slots,file_limit;
    sscanf(argv[1],"%u",&page_size);
    sscanf(argv[2],"%u",&buf_slots);
    sscanf(argv[3],"%u",&file_limit);
    Conf* cf = init_conf(page_size,buf_slots,file_limit,argv[4]);

    printf("Page size: %u, buffer slots: %u, limit of opened files: %u, buffer replacement policy: %s\n",cf->page_size, cf->buf_slots, cf->file_limit, cf->buf_policy);

    // load data and write database files
    init_db(argv[6],argv[5]);
    
    
    // implement your initialization function.
    init();


    // run test cases and write the log file
    run(argv[7],argv[8]);


    // implement your release function.
    release();


    // release database instance and system configuration instance
    free_db();
    free_conf();
    return 0;

}



// load test cases and test sel and join
void run(char* ra_path, char* log_path){

    

    FILE* query_fp = fopen(ra_path,"r");
    char line[100];


    // replace the old log file if exists
    FILE* log_fp = fopen(log_path,"w");

    while(fgets(line,100,query_fp)){

        // lines to write comments
        if(line[0] == '#') continue;

        // process selection operator
        if(line[0] == 's'){
            char ra[20];
            UINT idx = 0;
            INT val = 0;
            char operator[10];
            char table_name[50];

            // ra is "sel"
            // operator is not used for now, i.e., only consider "=="
            // we assume operator is = for simplicity
            sscanf(line,"%s %u %d %s %s",ra,&idx,&val,operator,table_name);

            reset_IO();

            _Table* result = sel(idx,val,table_name);
            

            // write the result to log file
            logT(result, log_fp);
            
            // release the result table
            freeT(result);
            
            continue;
        }

        // process join operator
        if(line[0] == 'j'){
            char ra[20];
            UINT idx1 = 0;
            UINT idx2 = 0;

            char table1_name[50];
            char table2_name[50];

            // ra is "join"

            // we assume operator is = for simplicity
            sscanf(line,"%s %u %s %u %s",ra,&idx1,table1_name,&idx2,table2_name);

            reset_IO();
            // execute join
            _Table* result = join(idx1,table1_name,idx2,table2_name);

            logT(result, log_fp);

            freeT(result);
            
            continue;
        }

        // other operators...

    }
    fclose(log_fp);
    fclose(query_fp);
}

// write a _Table to the log file
void logT(_Table* t, FILE* log_fp){
    // output to log
    if(t == NULL) return;

    // a separator "######"
    fprintf(log_fp,"\n######\n");
    // write the number of attributes for each tuple and the number of tuples
    Conf* cf = get_conf();
    fprintf(log_fp,"%u %u %u\n\n",t->nattrs,t->ntuples,cf->read_io);


    for (UINT i = 0; i < t->ntuples; i++){
        // write each tuple, separate attributes by space
        for (UINT j = 0; j < t->nattrs; j++){
                fprintf(log_fp,"%d ", t->tuples[i][j]);
        }
        // add '\n' to the end of each tuple
        fprintf(log_fp,"\n");
    }
    
}

// free the space of _Table
void freeT(_Table* t){
    if(t == NULL) return;

    // release memory for each tuple
    for (UINT i = 0; i < t->ntuples; i++){
        free(t->tuples[i]);
    }
    // release the whole table
    free(t);
}





