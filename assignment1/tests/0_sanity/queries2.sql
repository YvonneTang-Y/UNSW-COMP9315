\echo
\echo -- Checking invalid GeoCoord ...
\echo

select ' Melbourne,37.84°S,144.95°E'::GeoCoord;
select 'Melbourne ,37.84°S,144.95°E'::GeoCoord;
select 'Melbourne,37.84°S 1144.95°E'::GeoCoord;
select 'Melbourne,3.7.84°S 144.95°E'::GeoCoord;
select 'Melbourne,-3.7.84°S 144.95°E'::GeoCoord;

select 'Santa-Monica,118.4813°W 34.0232°N'::GeoCoord;
select 'Santa-Monica,118.4813°N 34.0232°W'::GeoCoord;
select 'Tangxing,35.72°N, 111.7108°N'::GeoCoord;
select 'Tangxing,35.72°N,111.7108°'::GeoCoord;