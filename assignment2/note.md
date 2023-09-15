C document on how to read binary data from file: 

<https://en.cppreference.com/w/c/io>



When we use memory from heap to sort our pages, can we use heap memory to save the sorted result and join the tables?

Does your "heap memory" mean the space applied by malloc? If so, I think the answer is yes.

You can use additional memory for sorting or hashing, **but the additional memory must only related to the data stored in buffer slots**. **When you release the data in a buffer slot, all additional memory related to the buffer must be released.**

只可以给buffer中的data进行malloc，当data从buffer中release，那么所有malloc的相关空间都要free



Buffer page replacement policy #408

只考虑 clock sweep

https://edstem.org/au/courses/10647/discussion/1285421



方法介绍：

https://zhuanlan.zhihu.com/p/418275148