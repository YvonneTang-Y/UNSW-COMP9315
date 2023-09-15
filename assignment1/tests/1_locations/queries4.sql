select convert_string(coord) as coord from Locations where coord > 'Ankara,39.9300°N,32.8500°E';

select convert_string(coord) as coord from Locations where coord < 'Ankara,39.9300°N,32.8500°E';

select convert_string(coord) as coord from Locations where coord <> 'Saint Helier,49.1858°N,2.1100°W';