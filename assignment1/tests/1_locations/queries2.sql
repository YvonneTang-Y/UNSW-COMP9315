create index on Locations using hash (coord);

select count(*) from Locations where coord = 'Saint Helier,49.1858°N,2.1100°W';
select lid, convert_string(coord) as coord from Locations where coord = 'Saint Helier,49.1858°N,2.1100°W';
explain analyze select * from Locations where coord = 'Saint Helier,49.1858°N,2.1100°W';