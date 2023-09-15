create table StoreInfo (
    sid         SERIAL PRIMARY KEY, 
    name        text,
    noproduct   int,  
    coord       GeoCoord,
    opendate    date
);

create or replace function convert_string(coord GeoCoord)
returns text 
as $$
DECLARE
    gc_string text;
    coord_string text;
    coordvalues text[];
    temp1 numeric; temp2 numeric;
    result text;
BEGIN
    gc_string := coord::text;
    result := split_part(gc_string, ',',1);
    result := lower(result);
    coord_string :=  substring(gc_string, position(',' in gc_string)+1);
    if coord_string = '' then 
        RAISE EXCEPTION 'Invalid Canonical Form';
    end if;

    if coord_string like '%,%' then
        coordvalues := string_to_array(coord_string, ',');
    else 
        coordvalues := string_to_array(coord_string, ' ');
    end if;

    if cardinality(coordvalues) != 2 then
        RAISE EXCEPTION 'Invalid Canonical Form';
    end if;

    temp1 := split_part(coordvalues[1], '°',1)::numeric;
    temp2 := split_part(coordvalues[2], '°',1)::numeric;
    
    if coordvalues[1] ~ '^[\d.°]*[NS]' then
        result := result || ',' || round(temp1,4)::text || '°' || right(coordvalues[1], 1) || ',' ||round(temp2,4) || '°' || right(coordvalues[2], 1);
    else 
        result := result || ',' || round(temp2,4)::text || '°' || right(coordvalues[2], 1) || ',' || round(temp1,4)  || '°' || right(coordvalues[1], 1);
    end if;
    return result;

    EXCEPTION when others then 
    RAISE EXCEPTION 'Invalid Canonical Form';
END;
$$ LANGUAGE plpgsql;

