drop type if exists SchemaTuple cascade;
create type SchemaTuple as ("table" text, "attributes" text);

create or replace function schema1() returns setof SchemaTuple
as $$
declare
        rec   record;
        rel   text := '';
	    attr  text := '';
        attrs text := '';
        out   SchemaTuple;
	    len   integer := 0;
begin
	for rec in
		select c.relname, a.attname, t.typname
		from   pg_class c, pg_attribute a, pg_namespace n,pg_type t
		where  c.relkind='r'
			and c.relnamespace = n.oid
			and n.nspname = 'public'
			and a.attrelid = c.oid
			and a.attnum > 0
            and a.atttypid = t.oid
		order by c.relname, a.attnum
	loop
		if (rec.relname <> rel) then
			if (rel <> '') then
				out := rel || '(' || att || ')';
				return next out;
			end if;
			rel := rec.relname;
			att := '';
			len := 0;
		end if;
		if (att <> '') then
			att := att || ', ';
			len := len + 2;
		end if;
		if (len + length(rec.attname) > 70) then
			att := att || E'\n        ';
			len := 0;
		end if;
		att := att || rec.attname;
		len := len + length(rec.attname);
	end loop;
	-- deal with last table
	if (rel <> '') then
		out := rel || '(' || att || ')';
		return next out;
	end if;
end;
$$ language plpgsql;
