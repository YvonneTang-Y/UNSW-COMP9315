select count(*) from StoreInfo;



select sid, name, opendate, convert_string(coord) as coord from StoreInfo
where sid = 12;

select sid, name, opendate, convert_string(coord) as coord from StoreInfo
where coord = 'Hrodna,53.6667°N 23.8167°E';

select sid, name, opendate, convert_string(coord) as coord from StoreInfo
where name = 'Store_oz0d';




select convert_string(coord) as coords, count(*)
from StoreInfo
group by coord having count(*) > 1 
order by count(*) DESC, coord ASC
limit 20;


