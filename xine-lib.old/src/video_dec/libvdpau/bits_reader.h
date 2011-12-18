#include <sys/types.h>



typedef struct {
  uint8_t *buffer, *start;
  int      offbits, length, oflow;
} bits_reader_t;



static void bits_reader_set( bits_reader_t *br, uint8_t *buf, int len )
{
  br->buffer = br->start = buf;
  br->offbits = 0;
  br->length = len;
  br->oflow = 0;
}



static uint32_t read_bits( bits_reader_t *br, int nbits )
{
  int i, nbytes;
  uint32_t ret = 0;
  uint8_t *buf;

  buf = br->buffer;
  nbytes = (br->offbits + nbits)/8;
  if ( ((br->offbits + nbits) %8 ) > 0 )
    nbytes++;
  if ( (buf + nbytes) > (br->start + br->length) ) {
    br->oflow = 1;
    return 0;
  }
  for ( i=0; i<nbytes; i++ )
    ret += buf[i]<<((nbytes-i-1)*8);
  i = (4-nbytes)*8+br->offbits;
  ret = ((ret<<i)>>i)>>((nbytes*8)-nbits-br->offbits);

  br->offbits += nbits;
  br->buffer += br->offbits / 8;
  br->offbits %= 8;

  return ret;
}



static void skip_bits( bits_reader_t *br, int nbits )
{
  br->offbits += nbits;
  br->buffer += br->offbits / 8;
  br->offbits %= 8;
  if ( br->buffer > (br->start + br->length) ) {
    br->oflow = 1;
  }
}



static uint32_t get_bits( bits_reader_t *br, int nbits )
{
  int i, nbytes;
  uint32_t ret = 0;
  uint8_t *buf;

  buf = br->buffer;
  nbytes = (br->offbits + nbits)/8;
  if ( ((br->offbits + nbits) %8 ) > 0 )
    nbytes++;
  if ( (buf + nbytes) > (br->start + br->length) ) {
    br->oflow = 1;
    return 0;
  }
  for ( i=0; i<nbytes; i++ )
    ret += buf[i]<<((nbytes-i-1)*8);
  i = (4-nbytes)*8+br->offbits;
  ret = ((ret<<i)>>i)>>((nbytes*8)-nbits-br->offbits);

  return ret;
}
