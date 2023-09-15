---------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------
create table test_gcoord(
   a GeoCoord
);

insert into test_gcoord values('Melbourne,37.84°S,144.95°E'),('Melbourne,37.84°S 144.9500°E'),('Melbourne,144.95°E,37.84°S'),('San Francisco,122.42°W,37.7700°N'),('san francisco,122.4200°W 37.77°N'),('Sydney,33.86°S,151.21°E'),('syDneY,151.2100°E 33.8600°S'),('Sydney,35.96°S,121.78°E'),('Chuuk Islands,5.45°N,153.51°E'),('Chuuk Islands,5.4500°N,153.5100°E'),('Alaska,70.2°S,144.0°W'),('Alaska,70.2000°S,144.0000°W');

select * from test_gcoord where a < 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a <= 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a = 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a = 'Chuuk Islands,5.4500°N,153.5100°E';
select Convert2DMS(a) from test_gcoord where a = 'Melbourne,37.84°S,144.95°E';
select * from test_gcoord where a >= 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a > 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a <> 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a ~ 'san francisco,122.4200°W 37.77°N';
select * from test_gcoord where a !~ 'san francisco,122.4200°W 37.77°N';

create index on test_gcoord using hash(a);
explain analyze select * from test_gcoord where a='Melbourne,37.84°S,144.95°E';

drop table test_gcoord;
drop type GeoCoord CASCADE;



