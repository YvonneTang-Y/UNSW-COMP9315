select convert_string(a.coord)
from Locations a join Locations b on (a.coord = b.coord)
order by a.coord; 

select convert_string(coord) as coords, count(*)
from Locations 
group by coord
order by coord;