select count(*) from Locations;
select convert_string(coord) as coord from Locations;
select lid, dms_string(Convert2DMS(coord)::text) as dms_coord from Locations;