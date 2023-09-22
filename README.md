### COMP9315 Database Systems Implementation
#### Course Description
This course aims to introduce students to the detailed internal structure of database management systems (DBMSs) such as Oracle or SQL Server. DBMSs contain a variety of interesting data structures and algorithms that are also potentially useful outside the DBMS context; knowing about them is a useful way of extending your general programming background. While the focus is on relational DBMSs, given that they have the best-developed technological foundation, we will also consider more recent developments in the management of large data repositories.

Relational DBMSs need to deal with a variety of issues: storage structures and management, implementation of relational operations, query optimisation, transactions, concurrency, recovery, security. The course will address most of these, along with a brief look at emerging database systems trends. The level of detail on individual topics will vary; some will be covered in significant detail, others will be covered relatively briefly.

An important aspect of this course is to give you a chance to explore the internals of a real DBMS: PostgreSQL. Lectures will discuss the general principles of how DBMSs are implemented, and will also illustrate them with examples from PostgreSQL where possible. Since DBMSs are very large pieces of software, it won't be possible to explore the entire PostgreSQL system in depth.

#### Assignment1
This assignment aims to give you
* an understanding of how data is treated inside a DBMS
* practice in adding a new base type to PostgreSQL
  
The goal is to implement a new data type for PostgreSQL, complete with input/output functions, comparison operators and the ability to build indexes on values of the type.

#### Assignment2
This assignment aims to give you
* an understanding of how data is organized inside a DBMS
* practice in implementing simple relational operators
* practice in implementing the memory buffer
  
The goal is to implement two relational operators, selection and join. Given the memory buffer slots and the page size, you need to implement your own memory buffer to read/write data files from hard drive.
