#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ro.h"
#include "db.h"


typedef struct table_pool {
    UINT oid;
    char name[10];
    FILE *file;
    UINT nattrs;
    UINT ntuples;
} *table_single;




// using FIFO 因为是单线程 并且 select join 固定 所以 此处应当不用担心 冲突
typedef struct tables_pool
{
    table_single  *Table_single;
    UINT file_limit;
    UINT next_idx;
}Tables;

static Tables FilePool;

int get_db_idx(const char* table_name)
{
    Database *db = get_db();
    INT target_table_idx = -1;
    //确定 属于 db中的第几个表
    for (int i = 0; i < db->ntables; ++i) {
        if(strcmp(table_name, db->tables[i].name) == 0)
        {
            target_table_idx = i;
            break;
        }
    }
    return target_table_idx;
}

table_single process_next_victim_file(const char* table_name)
{
    table_single ret = NULL;
    Database *db = get_db();
    INT target_table_idx  = get_db_idx(table_name);

    // 如果表的名字不在db中 报错
    if(target_table_idx == -1)
    {
        perror("table_name is not in database");
        return NULL;
    }
    else
    {
        // 如果在db中找到了 table的名字 和 oid
        // 如果原先FilePool中的file指针不为空 应该先关闭 旧的File指针
        if(FilePool.Table_single[FilePool.next_idx]->file != NULL)
        {
            // log_close_file
            log_close_file(FilePool.Table_single[FilePool.next_idx]->oid);
            fclose(FilePool.Table_single[FilePool.next_idx]->file);
        }
        // 开始将 替代过程
        // 创建文件的临时变量
        char *temp_path = malloc(sizeof(db->path) + sizeof(db->tables[target_table_idx].oid) + 1);
        sprintf(temp_path, "%s/%d", db->path, db->tables[target_table_idx].oid);
        FILE *path = fopen(temp_path, "rb");
        // 替代
        FilePool.Table_single[FilePool.next_idx]->file = path;
        // 填入信息 校准
        FilePool.Table_single[FilePool.next_idx]->oid = db->tables[target_table_idx].oid;
        // log_open_file
        log_open_file(FilePool.Table_single[FilePool.next_idx]->oid);
        strcpy(FilePool.Table_single[FilePool.next_idx]->name, db->tables[target_table_idx].name);
        FilePool.Table_single[FilePool.next_idx]->nattrs = db->tables[target_table_idx].nattrs;
        FilePool.Table_single[FilePool.next_idx]->ntuples = db->tables[target_table_idx].ntuples;
        ret = FilePool.Table_single[FilePool.next_idx];
        // 使用FIFO替换策略
        FilePool.next_idx ++;
        // 超过limit 回到第一个
        if(FilePool.next_idx >= FilePool.file_limit)
        {
            FilePool.next_idx = 0;
        }
    }
    return  ret;
}

table_single request_file(const char* table_name)
{
    Conf *cf = get_conf();
    table_single ret = NULL;
    // 判断 table name是否在数据库中
    for (int i = 0; i < cf->file_limit; ++i) {
        if(strcmp(FilePool.Table_single[i]->name, table_name) == 0)
        {
            ret = FilePool.Table_single[i];
        }
    }
    // 如果 目标不在数据库中 将db中的表加载进 file pool中 并返回 file指针
    if(ret == NULL)
    {
        ret = process_next_victim_file(table_name);
    }
    return ret;
}



void release_tables_pool()
{
    // 释放FilePool.Table_single File 指针数组
    for (int i = 0; i < FilePool.file_limit; ++i) {
        // 如果 没有打开 就没有 close
        if(FilePool.Table_single[i]->file != NULL)
        {
            // log_close_file
            //log_close_file(FilePool.Table_single[i]->oid);
            fclose(FilePool.Table_single[i]->file);
        }// 释放内存
        if(FilePool.Table_single[i])
        {
            free(FilePool.Table_single[i]);
        }

    }
    // 释放 FilePool.Table_single指针
    if(FilePool.Table_single != NULL)
    {
        free(FilePool.Table_single);
    }
}

typedef struct buf_page
{
    Tuple *tupesInBuffer;
    int nattrs;
    int ntuples;
    char table_name[10];
    UINT64 page_id;
    UINT64 Real_page_id;
    int pin;
    UINT usage;
}*buf_page;

// page_id 是 page_number
// Real_page_id 是 page_id

typedef struct buf_pool
{
    buf_page *buffers;
    UINT nbuffer;
    UINT nvb;
}buf_pool;
static buf_pool Buf_pool;

void  release_buff_pool()
{
    //printf("开始释放最终内存\n");
    for (int i = 0; i < Buf_pool.nbuffer ; ++i) {
        if(Buf_pool.buffers[i]->tupesInBuffer != NULL)
        {
            //printf("buffers[%d].nutuples: %d\n", i, Buf_pool.buffers[i]->ntuples);
            for (int j = 0; j < Buf_pool.buffers[i]->ntuples; ++j) {
                //printf("释放buffer[%d], tuple[%d]\n", i, j);
                free(Buf_pool.buffers[i]->tupesInBuffer[j]);
            }
            free(Buf_pool.buffers[i]->tupesInBuffer);
            Buf_pool.buffers[i]->tupesInBuffer = NULL;
        }
        //log_release_page(Buf_pool.buffers[i]->page_id);
        if(Buf_pool.buffers[i] != NULL)
        {
            free(Buf_pool.buffers[i]);
            Buf_pool.buffers[i] = NULL;
        }

    }
    if(Buf_pool.buffers != NULL)
    {
        free(Buf_pool.buffers);
    }


}

INT find_buff(const char* table_name, UINT64 page_id)
{
    INT buffer_slot_idx = -1;
    for (int i = 0; i < Buf_pool.nbuffer; ++i) {
        if(strcmp(Buf_pool.buffers[i]->table_name, table_name) == 0 &&
           Buf_pool.buffers[i]->page_id == page_id)
        {
            buffer_slot_idx = i;
        }
    }
    return buffer_slot_idx;
}

void read_page_into_buffer(table_single target_table, UINT64 page_id, buf_page buffer)
{

    Conf *cf = get_conf();

    UINT64 cur_page_id = -1;
    FILE *table_fp = target_table->file;
    fseek(table_fp, page_id * cf->page_size, SEEK_SET);
    INT ntuples_per_buffer = (cf->page_size-sizeof(UINT64))/sizeof(INT)/target_table->nattrs;


    fread(&cur_page_id,sizeof(UINT64), 1, table_fp);

//    printf("cur_page_id: %lu\n", cur_page_id);
    buffer->ntuples = 0;


//    if(cur_page_id != page_id)
//    {
//        printf("target_table: %s\n", target_table->name);
//        printf("cannot find corrent page, current page id is: %lu and target_page is : %lu\n", cur_page_id, page_id);
//    }
//    else
//    {
        log_read_page(cur_page_id);
        int pre_tuples = page_id * ntuples_per_buffer;
        log_release_page(buffer->Real_page_id);
//        printf("pre_tuples: %d\n", pre_tuples);
        for ( buffer->ntuples = 0; buffer->ntuples < ntuples_per_buffer; ++buffer->ntuples) {
            if (pre_tuples + buffer->ntuples  >= target_table->ntuples  && buffer->ntuples < ntuples_per_buffer-1)
            {
                if(buffer->tupesInBuffer[buffer->ntuples] != NULL)
                    free(buffer->tupesInBuffer[buffer->ntuples]);
                buffer->tupesInBuffer[buffer->ntuples] = NULL;
                break;
            }
            //getchar();
            Tuple temp = malloc(target_table->nattrs * sizeof(INT));
            fread(temp, sizeof(INT), target_table->nattrs, table_fp);
            if (buffer->tupesInBuffer[buffer->ntuples])
            {
                // printf("准备释放内存\n");
                // getchar();
                free(buffer->tupesInBuffer[buffer->ntuples]);

                // printf("释放成功\n");
                // getchar();
            }
            //     getchar();
            buffer->tupesInBuffer[buffer->ntuples] = temp;
            //    for (int i = 0; i < target_table->nattrs; ++i) {
            //        printf("buffer[%d]: %d",buffer->ntuples , buffer->tupesInBuffer[buffer->ntuples][i]);
            //    }
            //getchar();
//        }
    }
        buffer->page_id = page_id;
        buffer->Real_page_id = cur_page_id;
}

buf_page request_page(const char* table_name, UINT64 page_id)
{

    INT target_buffer_idx = find_buff(table_name, page_id);
    if(target_buffer_idx != -1)
    {
        Buf_pool.buffers[target_buffer_idx]->usage ++;
        Buf_pool.buffers[target_buffer_idx]->pin = 1;
        return Buf_pool.buffers[target_buffer_idx];
    }
    table_single target_table = request_file(table_name);

    while(1)
    {
        if (Buf_pool.buffers[Buf_pool.nvb]->pin == 0 && Buf_pool.buffers[Buf_pool.nvb]->usage == 0)
        {
            //
            //log_release_page(Buf_pool.buffers[Buf_pool.nvb]->page_id);
            // 将tuples 读入 buffer slot中
            //printf("target_table name: %s\n", target_table->name);
            read_page_into_buffer( target_table, page_id, Buf_pool.buffers[Buf_pool.nvb]);


            strcpy(Buf_pool.buffers[Buf_pool.nvb]->table_name, target_table->name);
            Buf_pool.buffers[Buf_pool.nvb]->page_id = page_id;
            Buf_pool.buffers[Buf_pool.nvb]->pin = 1;
            Buf_pool.buffers[Buf_pool.nvb]->usage = 1;
            INT ret_idx = Buf_pool.nvb;
            Buf_pool.nvb = (Buf_pool.nvb+1) % Buf_pool.nbuffer;
            fseek(target_table->file, 0, SEEK_SET);
            return Buf_pool.buffers[ret_idx];
        }
        else
        {
            if(Buf_pool.buffers[Buf_pool.nvb]->usage > 0) Buf_pool.buffers[Buf_pool.nvb]->usage --;
            Buf_pool.nvb = (Buf_pool.nvb+1) % Buf_pool.nbuffer;
        }
    }
}

void release_page(buf_page buffer)
{
    buffer->pin = 0;
    //log_release_page(buffer->page_id);
}


void current_buffer_free()
{
    int  count = 0;
    for (int i = 0; i < Buf_pool.nbuffer; ++i) {
        count += (1 - Buf_pool.buffers[i]->pin);
    }
    printf("当前有 %d buffer free\n", count);
}

void init(){

    Conf *cf = get_conf();
    // do some initialization here.
    // 初始化 filepool
    // 初始化 单个 table的指针数组 Table_single
    FilePool.file_limit = cf->file_limit;
    FilePool.Table_single = malloc( FilePool.file_limit * sizeof(table_single));
    for (int i = 0; i < cf->file_limit; ++i) {
        FilePool.Table_single[i] = malloc(sizeof(struct table_pool));
        FilePool.Table_single[i]->nattrs = 0;
    }
    // 因为为空 所以 第一个nvb为0

    FilePool.next_idx = 0;

    // 初始化 buffer pool
    Buf_pool.nbuffer = cf->buf_slots;
    //初始化 buffer_pool中 buf_page的数组指针
    Buf_pool.buffers = malloc(Buf_pool.nbuffer  * sizeof(buf_page));
    for (int i = 0; i < Buf_pool.nbuffer; ++i) {
        // buf_page的数组指针 中初始化 tuplesInBuffer中的
        Buf_pool.buffers[i] = malloc(sizeof(struct buf_page));
        Buf_pool.buffers[i]->tupesInBuffer = malloc(cf->page_size);
        for (int j = 0; j < cf->page_size / sizeof(Tuple); ++j) {
            Buf_pool.buffers[i]->tupesInBuffer[j] = NULL;
        }
        // 变为极大数
        Buf_pool.buffers[i]->page_id = -1;
        Buf_pool.buffers[i]->ntuples = -1;
        Buf_pool.buffers[i]->usage = 0;
        Buf_pool.buffers[i]->pin = 0;
    }
    Buf_pool.nvb = 0;


    //初始化完成

    //current_buffer_free();


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
    // free file pool
    release_tables_pool();
    release_buff_pool();

    printf("release() is invoked.\n");
}



_Table* sel(const UINT idx, const INT cond_val, const char* table_name){

    printf("sel() is invoked.\n");

    Conf *cf = get_conf();
    Database * db = get_db();
    // 创建返回对象

    Tuple Tuple_sat;
    // 读取文件
    // invoke log_read_page() every time a page is read from the hard drive.
    // 1. 计算有多少个page

    //table_single target_table = request_file(table_name);
    INT target_table_idx = get_db_idx(table_name);
    _Table* result = malloc(sizeof(_Table) + db->tables[target_table_idx].ntuples * sizeof(Tuple));

    result->nattrs = db->tables[target_table_idx].nattrs;
    result->ntuples = 0;


    int ntuples_per_page = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / db->tables[target_table_idx].nattrs;
    int npage = db->tables[target_table_idx].ntuples/ntuples_per_page +
                (db->tables[target_table_idx].ntuples % ntuples_per_page != 0);
//    printf("npage: %d\n", npage);
    for (int i = 0; i < npage; ++i) {
        buf_page cur_buffer = request_page(table_name, i);
//        printf("cur_buffer->ntuples: %d\n",cur_buffer->ntuples );
        for (int j = 0; j < cur_buffer->ntuples; ++j) {
            if(cur_buffer->tupesInBuffer[j][idx] == cond_val)
            {
                Tuple_sat = malloc(result->nattrs * sizeof(INT));
                memcpy(Tuple_sat, cur_buffer->tupesInBuffer[j], result->nattrs * sizeof(INT));
                result->tuples[result->ntuples++] = Tuple_sat;
            }
        }
        release_page(cur_buffer);
    }
//    for (int j = 0; j < result->ntuples; ++j) {
//    for (int k = 0; k < result->nattrs; ++k) {
//        printf("buffer[%d]: %d\n",j , result->tuples[j][k]);
//    }
//}


    return result;


    // invoke log_release_page() every time a page is released from the memory.
    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.
}




_Table* nested_join(const UINT idx1, const char* table1_name, const UINT idx2, const char* table2_name, const INT sequence)
{
    //printf("\n========================执行join=============================\n");
    //printf("执行指令: join %d %s %d %s\n", idx1, table1_name, idx2, table2_name);

    // 第一个表更小
    //四重循环 判断相等
    // 计算大小表中npage
    Conf *cf = get_conf();
    Database *db = get_db();
    INT table1_idx = get_db_idx(table1_name);
    INT table2_idx = get_db_idx(table2_name);

    // 准备result
    _Table* result = malloc(sizeof(_Table) + (db->tables[table1_idx].ntuples + db->tables[table2_idx].ntuples) * sizeof(Tuple));

    result->nattrs = db->tables[table1_idx].nattrs + db->tables[table2_idx].nattrs;
    result->ntuples = 0;
    Tuple Tuple_sat;

    INT ntuples_per_page_1 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / db->tables[table1_idx].nattrs;
    int npage_1 = db->tables[table1_idx].ntuples/ntuples_per_page_1 +
                  (db->tables[table1_idx].ntuples % ntuples_per_page_1 != 0);

    INT ntuples_per_page_2 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / db->tables[table2_idx].nattrs;
    int npage_2 = db->tables[table2_idx].ntuples/ntuples_per_page_2 +
                  (db->tables[table2_idx].ntuples % ntuples_per_page_2 != 0);
    // 小表1 pin住 大表2 循环

    int page_limit = cf->buf_slots-1 > npage_1 ? npage_1:cf->buf_slots-1;
    for(int a = 0; a < npage_1/(cf->buf_slots-1) + (npage_1%(cf->buf_slots-1) != 0); a++)
    {

        buf_page page_s[cf->buf_slots-1];
        int page_limit_inner = page_limit;
        //printf("a: %d, nloop: %d\n",a + 1, npage_1/(cf->buf_slots-1));
        if(a == npage_1/(cf->buf_slots-1))
        {
            page_limit_inner = npage_1%(cf->buf_slots-1);
        }
        //printf("page_limit_inner %d\n",page_limit_inner);


        // 最后一页 不完全

        for (int i = 0; i < page_limit_inner; i++) {
            //printf("request_page table: %s, id: %d\n",table1_name, (a*(cf->buf_slots-1)) + i);
            page_s[i] =  request_page(table1_name,(a*(cf->buf_slots-1) + i));
        }


        for (int i = 0; i < npage_2; ++i) {
            //printf("请求大表%s, %d页:\n", table2_name, i);
            buf_page page_b = request_page(table2_name,i);
            for (int j = 0; j < ntuples_per_page_2; ++j) {
                for (int k = 0; k < db->tables[table2_idx].nattrs; ++k) {
                    //printf("%d\t", page_b->tupesInBuffer[j][k]);
                }
                //printf("\n");


            }
            for (int j = 0; j < page_limit_inner; ++j) {
                for (int l = 0; l <ntuples_per_page_1; ++l) {
                    for (int k = 0; k < ntuples_per_page_2; ++k) {
                        // ?
                        //printf("当前是大表%s的 %d页 和 小表: %s 的 %d页进行第%d次join\n", db->tables[table2_idx].name, i, db->tables[table1_idx].name, a*cf->buf_slots+j,++count);
                        if( page_b->tupesInBuffer[k][idx2] == page_s[j]->tupesInBuffer[l][idx1])
                        {
                            Tuple_sat = malloc( result->nattrs * sizeof(INT));
                            // 正序
                            if(sequence == 1)
                            {
                                memcpy(Tuple_sat, page_s[j]->tupesInBuffer[l], db->tables[table1_idx].nattrs * sizeof(INT));
                                memcpy(Tuple_sat + db->tables[table1_idx].nattrs,
                                       page_b->tupesInBuffer[k], db->tables[table2_idx].nattrs * sizeof(INT));
                            }
                            else
                            {
                                memcpy(Tuple_sat, page_b->tupesInBuffer[k], db->tables[table2_idx].nattrs * sizeof(INT));
                                memcpy(Tuple_sat + db->tables[table2_idx].nattrs,
                                       page_s[j]->tupesInBuffer[l], db->tables[table1_idx].nattrs * sizeof(INT));
                                //printf("当前是大表: %s 的 %d页 和 小表: %s 的 %d页进行join\n", db->tables[table2_idx].name, npage_2, db->tables[table1_idx].name, npage_1);
                            }
                            result->tuples[result->ntuples++] = Tuple_sat;
                        }
                    }
                }
            }
            release_page(page_b);
        }

        for (int x = 0; x < page_limit_inner; x++) {
            release_page(page_s[x]);
        }
        //getchar();
    }
    return result;

}



void BBSort(Tuple table[], int n, int idx)
{
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (table[j][idx] > table[j + 1][idx]) {
                //printf("table_left: %d, table_right %d\n", table[j][0], table[j+1][0]);
                Tuple temp = table[j];
                table[j] = table[j+1];
                table[j+1] = temp;
                //printf("table_left: %d, table_right %d\n", table[j][0], table[j+1][0]);

            }
        }
    }
}




_Table* join(const UINT idx1, const char* table1_name, const UINT idx2, const char* table2_name){
    printf("join() is invoked.\n");
    // nested loop join
    // 判断空间是否足够
    INT target_table_one = get_db_idx(table1_name);
    INT target_table_two = get_db_idx(table2_name);
    _Table* result = NULL;
    Database* db = get_db();
    Conf* cf = get_conf();
    //获取两个表的大小
    INT ntuples_per_page_1 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / db->tables[target_table_one].nattrs;
    int npage_1 = db->tables[target_table_one].ntuples/ntuples_per_page_1 +
                  (db->tables[target_table_one].ntuples % ntuples_per_page_1 != 0);
    //printf("\ntable_name: %s pages:%d\n", table1_name,(npage_1 ));

    INT ntuples_per_page_2 = (cf->page_size - sizeof(UINT64)) / sizeof(INT) / db->tables[target_table_two].nattrs;
    int npage_2 = db->tables[target_table_two].ntuples/ntuples_per_page_2 +
                  (db->tables[target_table_two].ntuples % ntuples_per_page_2 != 0);
    //printf("\ntable_name: %s pages:%d\n", table2_name,(npage_2 ));

    if((npage_1 + npage_2) >  cf->buf_slots)
    {
        //使用nested join
        // 同时区分大小
        //printf("buffer_slot not enougth!\n");
        if(npage_1 > npage_2)
        {
            //printf("-1\n");
            result =  nested_join(idx2, table2_name, idx1, table1_name, -1);
        }
        else
        {   //printf("1\n");
            result = nested_join(idx1, table1_name, idx2, table2_name, 1);
        }
    }
    else // sort merge
    {
        //printf("buffer_slot enougth!\n");
        // 创建两个temp_table 用于merge
        Tuple *temp_table_one = malloc(db->tables[target_table_one].ntuples * sizeof(Tuple));
        Tuple *temp_table_two = malloc(db->tables[target_table_two].ntuples * sizeof(Tuple));
        INT count_one = 0, count_two = 0;
        for (int i = 0; i < npage_1; ++i) {
            buf_page buf = request_page(table1_name, i);
            for (int j = 0; j < ntuples_per_page_1; ++j) {
                //printf("开始copy %s的第%d页的第%d行\n", table1_name, i, j);
                if (count_one >= db->tables[target_table_one].ntuples) {
                    break;
                }
                Tuple temp_tuple = malloc(db->tables[target_table_one].nattrs * sizeof(INT)+1);
                memcpy(temp_tuple, buf->tupesInBuffer[j], db->tables[target_table_one].nattrs * sizeof(INT));
                temp_table_one[count_one++] = temp_tuple;
            }
            release_page(buf);
        }
        // temp_table_one 装载完成 排序
        BBSort(temp_table_one, count_one, idx1);

        for (int i = 0; i < npage_2; ++i) {
            buf_page buf = request_page(table2_name, i);
            for (int j = 0; j < ntuples_per_page_2; ++j) {
                //printf("开始copy %s的第%d页的第%d行\n", table2_name, i, j);
                if (count_two >= db->tables[target_table_two].ntuples) {
                    break;
                }
                Tuple temp_tuple = malloc(db->tables[target_table_two].nattrs * sizeof(INT)+1);
                memcpy(temp_tuple, buf->tupesInBuffer[j], db->tables[target_table_two].nattrs * sizeof(INT));
                temp_table_two[count_two++] = temp_tuple;
            }
            release_page(buf);
        }

//        printf("前:\n");
//        for (int i = 0; i < count_two; ++i) {
//            printf("第%d个tuple: ", i);
//            for (int j = 0; j < db->tables[target_table_two].nattrs; ++j) {
//                printf("%d\t", temp_table_two[i][j]);
//            }
//            printf("\n");
//        }
        BBSort(temp_table_two, count_two, idx2);
//        printf("后:\n");
//        for (int i = 0; i < count_two; ++i) {
//            printf("第%d个tuple: ", i);
//            for (int j = 0; j < db->tables[target_table_two].nattrs; ++j) {
//                printf("%d\t", temp_table_two[i][j]);
//            }
//            printf("\n");
//        }

        Tuple Tuple_sat;
        result = malloc(sizeof(_Table) + (db->tables[target_table_one].ntuples + db->tables[target_table_one].ntuples) * sizeof(Tuple));
        result->nattrs = db->tables[target_table_one].nattrs + db->tables[target_table_two].nattrs;
        result->ntuples = 0;
        INT last_value = -111111;

        ////////////////////////////////////////////////
//        Tuple tuple_r = NULL;
//        Tuple tuple_s = NULL;
//        while((tuple_r = nextTuple(temp_table_one, r++, count_one)) != NULL && (tuple_s = nextTuple(temp_table_one, s++, count_one)) != NULL)
//        {
//
//        }
//
//        getchar();

        ////////////////////////////////////////////////
        for (int r = 0, ss = 0; r < count_one; ++r) {
            for (int s = ss; s < count_two; ++s) {
                if(temp_table_two[s][idx2] > temp_table_one[r][idx1])
                {
                    break;
                }
                if(temp_table_two[s][idx2] < temp_table_one[r][idx1])
                {
                    continue;
                }
                if(last_value != temp_table_one[s][idx2])
                {
                    last_value = temp_table_one[s][idx2];
                    ss = s;
                }
                //printf("cur_table_one: ");
//                for (int i = 0; i < db->tables[target_table_one].nattrs; ++i) {
//                    printf("%d ", temp_table_one[r][i]);
//                }
                //printf("\n");
                //printf("cur_table_two: ");
//                for (int j = 0; j < db->tables[target_table_two].nattrs; ++j) {
//                    printf("%d ", temp_table_two[s][j]);
//                }
                //printf("\n");

                if(temp_table_one[r][idx1] == temp_table_two[s][idx2])
                {
                    //getchar();
                    Tuple_sat = malloc( result->nattrs * sizeof(INT));
                    memcpy(Tuple_sat, temp_table_one[r], db->tables[target_table_one].nattrs * sizeof(INT));
                    memcpy(Tuple_sat + db->tables[target_table_one].nattrs,
                           temp_table_two[s], db->tables[target_table_two].nattrs * sizeof(INT));
//                    for (int i = 0; i < db->tables[target_table_one].nattrs + db->tables[target_table_two].nattrs; ++i) {
//                        printf("%d ", Tuple_sat[i]);
//                    }
//                    getchar();
                    result->tuples[result->ntuples++] = Tuple_sat;
                }
            }

        }




//释放 两个tuple
        for (int i = 0; i < count_one; ++i) {
            free(temp_table_one[i]);
        }
        free(temp_table_one);
        for (int i = 0; i < count_two; ++i) {
            free(temp_table_two[i]);
        }
        free(temp_table_two);
    }



    return result;

    // write your code to join two tables
    // invoke log_read_page() every time a page is read from the hard drive.
    // invoke log_release_page() every time a page is released from the memory.

    // invoke log_open_file() every time a page is read from the hard drive.
    // invoke log_close_file() every time a page is released from the memory.

}

