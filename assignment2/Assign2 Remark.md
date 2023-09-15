# Assign2 Remark

**Incorrect read page id but not in log_read_page function**

I fail most of the tests. And only `1/9` for the part "`Final Test Score (query + I/O, max 9)`".



**The reason is:**

In the beginning, I record the page into buffer just with `loop variable` 0, 1, 2,…(in the sample test, it’s the same as the `pid`). After I finished my code and checked the forum, I know the `pid` and `loop variable` are different. 

I try to modify my code but miss one function (this problem can't be found via sample tests since `pid` is the same as `loop variable` )

It means I give the wrong `pid`. Just 4 lines need to be modified:

**Add one line in the selection operator** 

```c
339 fseek(table_fp, i * cf_ro->page_size, SEEK_SET);
```

**Change one parameter in the same function**

from

```
365 release_page(pool, table_name, i);

621 release_page(pool, table_name_inner, j); 

626 release_page(pool, table_name_outer, loop * (cf_ro->buf_slots - 1) + i);
```

to

```
366 release_page(pool, table_name, pid);

622 release_page(pool, table_name_inner, pid2);

627 fseek(table_fp1, (loop * (cf_ro->buf_slots - 1) + i) * cf_ro->page_size, SEEK_SET);
628 fread(&pid1,sizeof(UINT64),1,table_fp1);           
629 release_page(pool, table_name_outer, pid1);
```

![1683069316243](C:\Users\Yvonne\AppData\Roaming\Typora\typora-user-images\1683069316243.png)

![1683069332004](C:\Users\Yvonne\AppData\Roaming\Typora\typora-user-images\1683069332004.png)

![1683069360693](C:\Users\Yvonne\AppData\Roaming\Typora\typora-user-images\1683069360693.png)





