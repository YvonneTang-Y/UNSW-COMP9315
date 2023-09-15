create or replace view pop0(tabname, ntuples)
as
select relname, reltuples
from   pg_class
where  relnamespace = 2200 and relkind = 'r';
