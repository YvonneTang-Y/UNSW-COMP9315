\echo
\echo -- Checking valid GeoCoord ...
\echo

select convert_string('Melbourne,37.84°S,144.95°E'::GeoCoord);
select convert_string('Stamford,41.10°N,73.55°W'::GeoCoord);
select convert_string('Cholula de Rivadabia,19.0633°N 98.3064°W'::GeoCoord);
select convert_string('Elizabeth,40.66°N,74.19°W'::GeoCoord);
select convert_string('Spanish Town,17.9961°N,76.9547°W'::GeoCoord);
select convert_string('Formosa,47°W,15.0°S'::GeoCoord);
select convert_string('Rayleigh,51.58°N 0.6049°E'::GeoCoord);
select convert_string('San Angelo,100.45°W,31.44°N'::GeoCoord);
select convert_string('Santa Monica,118.4813°W 34.0232°N'::GeoCoord);
select convert_string('Tangxing,35.72°N,111.7108°E'::GeoCoord);

