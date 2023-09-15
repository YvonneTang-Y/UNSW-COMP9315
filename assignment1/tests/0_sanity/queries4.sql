\echo
\echo -- Checking operators ... tests below should return False
\echo

select 'Melbourne,37.84°S,144.95°E'::GeoCoord <> 'melbourne,144.95°E 37.84°S'::GeoCoord;
select 'Melbourne,37.84°S,144.95°E'::GeoCoord <> 'melbourne,37.8400°S,144.9500°E'::GeoCoord;
select 'San Angelo,100.45°W,31.44°N'::GeoCoord =  'Santa Monica,118.4813°W 34.0232°N'::GeoCoord;
select 'Cholula de Rivadabia,19.0633°N 98.3064°W'::GeoCoord <= 'Melbourne,37.84°S,144.95°E'::GeoCoord;
select 'melbourne,37.8400°S,144.9500°E'::GeoCoord < 'Melbourne,37.84°S,144.95°E'::GeoCoord;
select 'Melbourne,37.84°S,144.95°E'::GeoCoord >= 'Spanish Town,17.9961°N,76.9547°W'::GeoCoord;
select 'Melbourne,37.84°S,144.95°E'::GeoCoord > 'San Angelo,100.45°W,31.44°N'::GeoCoord;
select 'Stamford,41.10°N,73.55°W'::GeoCoord !~ 'Elizabeth,40.66°N,74.19°W'::GeoCoord;
select 'Rayleigh,51.58°N 0.6049°E'::GeoCoord ~  'Formosa,47°W,15.0°S'::GeoCoord;