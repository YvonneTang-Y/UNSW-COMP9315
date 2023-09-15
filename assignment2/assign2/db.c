#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "db.h"

Conf* cf = NULL;
Database* db = NULL;

Conf* init_conf(const UINT page_size, const UINT buf_slots, const UINT file_limit, const char* buf_policy){
    cf = malloc(sizeof(Conf));
    cf->page_size = page_size;
    cf->buf_slots = buf_slots;
    cf->file_limit = file_limit;
    strcpy(cf->buf_policy,buf_policy);
    return cf;
}

void free_conf(){
    free(cf);
}

Conf* get_conf(){
    return cf;
}

Database* get_db(){
    return db;
}

void free_db(){
    if (db != NULL) free(db);
}


// build database
Database* init_db(char* input_data_path, char* data_path){
    
    
    // check if db folder exists
    struct stat st = {0};
    if(stat(data_path, &st) == -1){
        printf("Database folder does not exist. Create the folder %s.\n",data_path);
        mkdir(data_path, 0777);
    }

    
    db = NULL;

    
    // open the input data file
    FILE* input_fp = fopen(input_data_path,"r");
    printf("Input data path:%s\n",input_data_path);
    if (input_fp == NULL){
        perror("Fail to open the input data file.\n");
        exit(-1);
    }



    // file pointer to write tuples 
    FILE* table_fp = NULL;

    
    
    
    INT table_idx = -1;
    INT ntuples_per_page = 0;
    INT nbytes_free = 0;

    INT processed_ntuples = 0;

    UINT64 page_id = 0;
    Table t;
    
    char line[100];
    while(fgets(line,100,input_fp)){
        
        // lines to write comments
        if(line[0] == '#') continue;
        
        // line for database meta info
        if(line[0] == 'd'){
            UINT ntables;
            char desc[50];
            
            // get number of tables
            sscanf(line,"%s %d",desc,&ntables);

            
            
            // initialize Database instance
            db = malloc(sizeof(Database)+ntables*sizeof(Table));
            db->ntables = ntables;
            strcpy(db->path,data_path);

            // printf("data path: %s\n",db->path);
            
            
            continue;
        }

        // line for table meta info
        if(line[0] == 't'){

            ++table_idx;

            // close the old one if exists
            if(table_idx > 0){
                // the current table is not the first

                if(processed_ntuples != 0){
                    // the last page is not full
                    // add 0 to the end
                    INT8 f = 0;
                    UINT left_space = sizeof(INT)*t.nattrs*(ntuples_per_page-processed_ntuples)+nbytes_free;
                    
                    for (UINT i = 0; i < left_space; i++) fwrite(&f,sizeof(INT8),1,table_fp);
                

                    processed_ntuples = 0;
                }
                
                fclose(table_fp);

            }



            char desc[50];

            // initialzie a table instance
            // Table t;
            sscanf(line,"%s %u %s %u",desc,&t.oid,t.name,&t.nattrs);
            t.ntuples = 0;
            
            
            // add the table pointer to the DB instance
            db->tables[table_idx] = t;


            
            // produce table file path
            char table_path[200];
            sprintf(table_path,"%s/%u",db->path,t.oid);
            // open file pointer for the table
            table_fp = fopen(table_path,"wb");
            

            // reset page id
            page_id = 0;
            
            
            // calculate number of tuples per page
            ntuples_per_page = (cf->page_size-sizeof(UINT64))/sizeof(INT)/t.nattrs;
            nbytes_free = (cf->page_size-sizeof(UINT64)) % (sizeof(INT)*t.nattrs);
            // printf("ntuples = %u, free bytes = %u\n",ntuples_per_page,nbytes_free);

            processed_ntuples = 0;
            
            continue;
        }
        // skip empty lines
        if(!isdigit(line[0])) continue;

        // we are processing the first tuple for a page
        if(processed_ntuples == 0){
            // write a page id to the file
            fwrite(&page_id,sizeof(UINT64),1,table_fp);
            ++page_id;
        }
        ++processed_ntuples;
        ++db->tables[table_idx].ntuples;
        // printf("processed tuples = %u, tuples per page = %u\n",processed_ntuples,ntuples_per_page);


        // write tuple to file
        // assume each table has only one file
        
        char* token = strtok(line," ");
        
        INT attr;
        while(token != NULL){
            
            // read each attribute and write it to the hard drive
            sscanf(token,"%d",&attr);
            
            fwrite(&attr,sizeof(INT),1,table_fp);
            
            token = strtok(NULL," ");
        }

        
        
        // when the derived number of tuples reaches the maximum number of tuples per page
        // we add 0 to the end if necessary
        if(processed_ntuples == ntuples_per_page){
            INT8 f = 0;
            for (UINT i = 0; i < nbytes_free; i++) fwrite(&f,sizeof(INT8),1,table_fp);
            processed_ntuples = 0;

        }
        

    }

    if(processed_ntuples != 0){
        // the last page is not full
        // add 0 to the end
        INT8 f = 0;
        UINT left_space = sizeof(INT)*t.nattrs*(ntuples_per_page-processed_ntuples)+nbytes_free;
        for (UINT i = 0; i < left_space; i++) fwrite(&f,sizeof(INT8),1,table_fp);
        processed_ntuples = 0;
    }
    
    fclose(table_fp);
    fclose(input_fp);

    return db;
}

void reset_IO(){
    cf->read_io = 0;
    cf->write_io = 0;
}

void log_read_page(UINT64 pid){
    // the following print info is for testing
    // comment it out to avoid too much output in terminal when developing
    // printf("Read page %llu\n",pid);
    cf->read_io ++;
}
void log_release_page(UINT64 pid){
    // the following print info is for testing
    // comment it out to avoid too much output in terminal when developing
    // printf("Release page %llu\n",pid);

}

void log_open_file(UINT oid){
    // the following print info is for testing
    // comment it out to avoid too much output in terminal when developing
    // printf("Open file %u\n",oid);
}
void log_close_file(UINT oid){
    // the following print info is for testing
    // comment it out to avoid too much output in terminal when developing
    // printf("Close file %u\n",oid);
}