create index hash_file on StoreInfo using hash(coord);

explain analyze
select * from StoreInfo
where coord = 'Pattani,6.8664°N,101.2508°E';


explain analyze
select * from StoreInfo
where coord = 'Pattani,6.8664°N,101.2508°E';

drop index hash_file;