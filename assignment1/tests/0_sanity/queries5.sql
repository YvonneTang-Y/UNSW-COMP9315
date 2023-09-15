\echo
\echo -- Checking Convert2DMS
\echo

select dms_string(Convert2DMS('Melbourne,37.84°S,144.95°E'::GeoCoord)::text);
select dms_string(Convert2DMS('Stamford,41.10°N,73.55°W'::GeoCoord)::text);
select dms_string(Convert2DMS('Cholula de Rivadabia,19.0633°N 98.3064°W'::GeoCoord)::text);
select dms_string(Convert2DMS('Elizabeth,40.66°N,74.19°W'::GeoCoord)::text);
select dms_string(Convert2DMS('Spanish Town,17.9961°N,76.9547°W'::GeoCoord)::text);
select dms_string(Convert2DMS('Formosa,47°W,15.0°S'::GeoCoord)::text);
select dms_string(Convert2DMS('San Angelo,100.45°W,31.44°N'::GeoCoord)::text);
select dms_string(Convert2DMS('Santa Monica,118.4813°W 34.0232°N'::GeoCoord)::text);
select dms_string(Convert2DMS('Tangxing,35.72°N,111.7108°E'::GeoCoord)::text);


