CREATE FUNCTION public.base32_encode(bytea) RETURNS text
    AS '$libdir/base32', 'base32_encode'
    LANGUAGE C STRICT;

CREATE FUNCTION public.base32_decode(text) RETURNS bytea
    AS '$libdir/base32', 'base32_decode'
    LANGUAGE C STRICT;
