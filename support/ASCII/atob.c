/* atob: version 4.0
 * stream filter to change printable ascii from "btoa" back into 8 bit bytes
 * if bad chars, or Csums do not match: exit(1) [and NO output]
 *
 *  Paul Rutter		Joe Orost
 *  philabs!per		petsd!joe
 */

#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define WORD_32
#define reg
#define ulong unsigned long

#define streq(s0, s1)  strcmp(s0, s1) == 0

#ifdef WORD_32
#define times85(x)     ((((((x<<2)+x)<<2)+x)<<2)+x)
#else
#define times85(x)     (x*85L)
#endif

ulong int Ceor = 0;
ulong int Csum = 0;
ulong int Crot = 0;
ulong int word = 0;
short int bcount = 0;

fatal() {
  fprintf(stderr, "bad format or Csum to atob\n");
  exit(1);
}

#define DE(c) ((c) - '!')

decode(c) 
  reg unsigned c;
{
  if (c == 'z') {
    if (bcount != 0) {
      fatal();
    } else {
      byteout(0);
      byteout(0);
      byteout(0);
      byteout(0);
    }
  } else if ((c >= '!') && (c < ('!' + 85))) {
    if (bcount == 0) {
      word = DE(c);
      ++bcount;
    } else if (bcount < 4) {
      word = times85 (word);
      word += DE(c);
      ++bcount;
    } else {
      word = times85(word) + DE(c);
      byteout ((unsigned)((word >> 24) & 255));
      byteout ((unsigned)((word >> 16) & 255));
      byteout ((unsigned)((word >> 8) & 255));
      byteout ((unsigned)(word & 255));
      word   = 0;
      bcount = 0;
    }
  } else {
    fatal();
  }
}

FILE *tmp_file;

byteout(c) 
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
  putc(c, tmp_file);
}

main(argc, argv) 
  char **argv;
{
  reg unsigned c;
  reg long int i;
  char tmp_name[100];
  char buf[100];
  ulong int n1, n2, oeor, osum, orot;

  if (argc != 1) {
    fprintf(stderr,"bad args to %s\n", argv[0]);
    exit(2);
  }
  sprintf(tmp_name, "atob%04X.a2b", getpid());
  tmp_file = fopen(tmp_name, "w+b");
  if (tmp_file == NULL) {
    fatal();
  }
  unlink(tmp_name);	/* Make file disappear */
  /* search for header line */
  for (;;) {
    if (fgets(buf, sizeof buf, stdin) == NULL) {
      fatal();
    }
    if (streq(buf, "xbtoa Begin\n")) {
      break;
    }
  }

  while ((c = getchar()) != EOF) {
    if (c == '\n') {
      continue;
    } else if (c == 'x') {
      break;
    } else {
      decode (c);
    }
  }
  if(scanf("btoa End N %ld %lX E %lX S %lX R %lX\n",
         &n1, &n2, &oeor, &osum, &orot) != 5) {
    fatal();
  }
  if ((n1 != n2) || (oeor != Ceor) || (osum != Csum) || (orot != Crot)) {
    fatal();
  } else {
    setmode (fileno(stdout), O_BINARY);
    /* copy OK tmp file to stdout */;
    fseek (tmp_file, 0L, 0);
    for (i = n1; --i >= 0;) {
      putchar (getc(tmp_file));
    }
  }
  exit (0);
}
