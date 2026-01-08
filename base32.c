/* base32.c - RFC4648 Base32 encode/decode for PostgreSQL
 *
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/varlena.h"
#include "funcapi.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1 (base32_encode);
PG_FUNCTION_INFO_V1 (base32_decode);

static const char *b32_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

Datum
base32_encode (PG_FUNCTION_ARGS)
{
  bytea *input = PG_GETARG_BYTEA_P (0);
  unsigned char *data = (unsigned char *) VARDATA_ANY (input);
  int len = VARSIZE_ANY_EXHDR (input);

  /* output length: ceil(len * 8 / 5) padded to multiple of 8 */
  int out_len = ((len * 8 + 4) / 5);
  int padded_len = ((out_len + 7) / 8) * 8;

  char *out = (char *) palloc0 (padded_len + 1);
  unsigned int bitbuf = 0;
  int bits = 0;
  int out_pos = 0;
  int i;

  for (i = 0; i < len; i++)
    {
      bitbuf = (bitbuf << 8) | data[i];
      bits += 8;
      while (bits >= 5)
        {
          bits -= 5;
          int idx = (bitbuf >> bits) & 0x1F;
          out[out_pos++] = b32_alphabet[idx];
        }
    }

  if (bits > 0)
    {
      int idx = (bitbuf << (5 - bits)) & 0x1F;
      out[out_pos++] = b32_alphabet[idx];
    }

  /* padding with '=' to padded_len */
  while (out_pos < padded_len)
    out[out_pos++] = '=';

  text *result = cstring_to_text_with_len (out, out_pos);
  pfree (out);
  PG_RETURN_TEXT_P (result);
}

static int
b32_val (char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= '2' && c <= '7')
    return 26 + (c - '2');
  return -1;
}

Datum
base32_decode (PG_FUNCTION_ARGS)
{
  text *input = PG_GETARG_TEXT_PP (0);
  char *s = VARDATA_ANY (input);
  int slen = VARSIZE_ANY_EXHDR (input);
  int i;
  int out_cap = (slen * 5) / 8 + 1;
  unsigned char *out = (unsigned char *) palloc0 (out_cap);
  int out_pos = 0;

  unsigned int bitbuf = 0;
  int bits = 0;

  for (i = 0; i < slen; i++)
    {
      char c = s[i];
      if (c == '=' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
        break;                  /* stop at padding or whitespace */

      /* tolerate lowercase */
      if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';

      int v = b32_val (c);
      if (v < 0)
        ereport (ERROR, (errmsg ("invalid base32 character: '%c'", s[i])));

      bitbuf = (bitbuf << 5) | v;
      bits += 5;

      if (bits >= 8)
        {
          bits -= 8;
          unsigned char byte = (bitbuf >> bits) & 0xFF;
          out[out_pos++] = byte;
        }
    }

  bytea *result = (bytea *) palloc (out_pos + VARHDRSZ);
  SET_VARSIZE (result, out_pos + VARHDRSZ);
  memcpy (VARDATA (result), out, out_pos);
  pfree (out);

  PG_RETURN_BYTEA_P (result);
}
