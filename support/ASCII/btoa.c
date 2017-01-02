/* btoa: version 4.0
 * stream filter to change 8 bit bytes into printable ascii
 * computes the number of bytes, and three kinds of simple checksums
 * incoming bytes are collected into 32-bit words, then printed in base 85
 *  exp(85,5)     > exp(2,32)
 *  4.437.053.125 > 4.294.967.296
 * the ASCII characters used are between '!' and 'u'.
 * 'z' encodes 32-bit zero; 'x' is used to mark the end of encoded data.
 *
 *  Paul Rutter		Joe Orost
 *  philabs!per		petsd!joe
 *
 *  WARNING: this version is not compatible with the original as sent out
 *  on the net.  The original encoded from ' ' to 't'; which cause problems
 *  with some mailers (stripping off trailing blanks).
 */

#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define reg
#define ulong unsigned long

#define MAXPERLINE 78

ulong int Ceor = 0;
ulong int Csum = 0;
ulong int Crot = 0;

short int ccount = 0;
short int bcount = 0;
ulong int word32 = 0;

#define EN(c)	(unsigned) ((c) + '!')

encode (c)
  reg unsigned c;
{
  Ceor ^= c;
  Csum += c;
  Csum += 1;
  if ((Crot & 0x80000000)) {
    Crot <<= 1;
    Crot += 1;
  } else {
    Crot <<= 1;
  }
  Crot += c;

  word32 <<= 8;
  word32 |= c;
  if (bcount == 3) {
    wordout(word32);
   word32 = 0;
    bcount = 0;
  } else {
    bcount += 1;
  }
}

wordout (word32)
  ulong int word32;
{

  if (word32 == 0) {
    charout('z');
  } else {
    reg unsigned tmp = 0;
    
    if(word32 < 0) {	/* Because some don't support unsigned long */
      tmp = 32;
      word32 = word32 - (ulong)(85L * 85L * 85L * 85L * 32);
    }
    if(word32 < 0) {
      tmp = 64;
      word32 = word32 - (ulong)(85L * 85L * 85L * 85L * 32);
    }

    charout (EN((word32 / (ulong)((ulong)85L * 85L * 85L * 85L)) + tmp));
    word32 %= (ulong)(85L * 85L * 85L * 85L);

    charout (EN(word32 / (ulong)(85L * 85L * 85L)));
    word32 %= (ulong)(85L * 85L * 85L);

    charout (EN(word32 / (ulong)(85L * 85L)));
    word32 %= (ulong)(85L * 85L);

    charout (EN(word32 / (ulong)85L));
    word32 %= (ulong)85L;

    charout (EN(word32));
  }
}

charout (c)
  unsigned c;
{
  putchar(c);
  ccount += 1;
  if (ccount == MAXPERLINE) {
    putchar('\n');
    ccount = 0;
  }
}

main (argc,argv)
  char **argv;
{
  unsigned c;
  reg long int n;

  if (argc != 1) {
    fprintf(stderr,"bad args to %s\n", argv[0]);
    exit(2);
  }
  setmode (fileno(stdin), O_BINARY);

  printf("xbtoa Begin\n");
  n = 0;
  while ((c = getchar()) != EOF) {
    encode(c);
    n += 1;
  }
  while (bcount != 0) {
    encode(0);
  }
  /* n is written twice as crude cross check */
  printf("\nxbtoa End N %ld %lX E %lX S %lX R %lX\n", n, n, Ceor, Csum, Crot);
  exit(0);
}
