#ifndef DB_H
#define DB_H

#include <stdint.h>
#include <stdio.h>


#define INT int32_t
#define UINT uint32_t
#define INT8 uint8_t
#define UINT64 uint64_t

#define Tuple INT*

// returned data type by relational operators
typedef struct _Table{
    UINT nattrs;
    UINT ntuples;
    Tuple tuples[];
} _Table;

// internal table meta information
typedef struct Table{
    UINT oid;
    char name[10];
    UINT nattrs;
    UINT ntuples;
} Table;

// internal database meta information
typedef struct Database {
    UINT ntables;
    char path[100];
    Table tables[];
} Database;

// system configuration
typedef struct Conf{
    UINT read_io;
    UINT write_io;
    UINT page_size;
    UINT buf_slots;
    UINT file_limit;
    char buf_policy[4];
} Conf;



// declaration for functions in db.c

Conf* init_conf(const UINT page_size, const UINT buf_slots, const UINT file_limit, const char* buf_policy);
void free_conf();
Conf* get_conf();

Database* init_db(char* input_data_path, char* data_path);
Database* get_db();
void free_db();

void reset_IO();
void log_read_page(UINT64 pid);
void log_release_page(UINT64 pid);
void log_open_file(UINT oid);
void log_close_file(UINT oid);

// void log_write_page(UINT64 pid);


#endif