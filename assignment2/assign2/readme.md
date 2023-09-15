## Update Log

*28 Mar* - Update `db.c`. Fix bug of writing unexpected values, e.g., change `fwrite(&f,sizeof(INT8),nbytes_free,table_fp)` to `fwrite(&f,sizeof(INT8),1,table_fp);`.