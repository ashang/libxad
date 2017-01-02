/* XFD external slave for CPic format by Kyzer/CSG <kyzer@4u.net>
 * This slave code is in the Public Domain.
 *
 * CPic format (C) Thomas Zipproth
 */

/* CPIC FORMAT:
 *
 * offset size what
 * 0x00   4    "CPic"
 * 0x04   2    width of picture (in pixels)
 * 0x06   2    height of picture (in pixels)
 * 0x08   2    PICX: offset (in bytes) of left edge of picture
 * 0x0A   2    PICY: offset (in lines) of top edge of picture
 * 0x0C   2    PICW: width of stored picture (in bytes)
 * 0x0E   2    PICH: height of stored picture (in lines)
 * 0x10   2    number of bitplanes
 * 0x12   6*4  offset of each bitplane (from the start of the file)
 * 0x2A   6*4  length of each bitplane (each bitplane is individually packed)
 * 0x42   2    viewport mode (see <graphics/view.h>)
 * 0x44   32*2 colour palette
 * 0x84   SIZEOF
 *
 * At each bitplane's offset, there is an RLE bytestream, which works like so:
 *   read decision byte
 *   byte is   0..31  -- output 1..32 zero bytes (no bits set)
 *   byte is  32..127 -- copy the next 1..96 source bytes directly to output
 *   byte is 128..159 -- output 1..32 0xFF bytes (all bits set)
 *   byte is 160..255 -- read one source byte, then output it 1..96 times
 *   repeat until no more bytes needed
 *
 * The decrunched bytestream is written in vertical 1-byte strips from
 * picx,picy to picx,picy+pich, then picx+1,picy to picx+1,picy+pich, and
 * so on until the final strip, picx+picw,picy to picx+picw,picy+pich.
 */



#include <libraries/xfdmaster.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>
#include "SDI_compiler.h"

#ifdef DEBUG
void KPrintF(char *fmt, ...);
# define D(x) { KPrintF x ; }
#else
# define D(x)
#endif

static char version[]="$VER: CPic 1.0 (22.12.2002) by <kyzer@4u.net>";

#define EndGetM32(a)  ((((a)[0])<<24)|(((a)[1])<<16)|(((a)[2])<<8)|((a)[3]))
#define EndGetM16(a)  ((((a)[0])<<8)|((a)[1]))

#define IFF_HEADER_LEN (0xA4)
#define IFF_HEADER_LEN_STATIC (0x3C)

static const UBYTE iff_header[IFF_HEADER_LEN_STATIC] = {
  'F', 'O', 'R', 'M',    /* 00 FORM                        */
   0,   0,   0,   0,     /* 04   form length               */
  'I', 'L', 'B', 'M',    /* 08   ILBM                      */ 
  'B', 'M', 'H', 'D',    /* 0c   BMHD                      */ 
   0,   0,   0,   20,    /* 10     bmhd chunk length (20)  */ 
   0,   0,               /* 14     width                   */
   0,   0,               /* 16     height                  */
   0,   0,               /* 18     x offset (0)            */
   0,   0,               /* 1a     y offset (0)            */
   0,                    /* 1c     number of bitplanes     */
   0,                    /* 1d     masking (0)             */
   0,                    /* 1e     compression (0)         */
   0,                    /* 1e     reserved1 (0)           */
   0,   0,               /* 20     transparent colour (0)  */
   1,                    /* 22     x aspect (1)            */
   1,                    /* 23     x aspect (1)            */
   0,   0,               /* 24     page width              */
   0,   0,               /* 26     page height             */
  'C', 'A', 'M', 'G',    /* 28   CAMG                      */
   0,   0,   0,   4,     /* 2c     camg chunk length (4)   */
   0,   0,   0,   0,     /* 30     viewmode                */
  'C', 'M', 'A', 'P',    /* 34   CMAP                      */
   0,   0,   0,   96     /* 38     cmap chunk length (96)  */
                         /* 3c     {UBYTE r,g,b}[32]       */
                         /* 9c   BODY                      */
                         /* a0     body chunk length       */
                         /* a4     unpacked raw bitplanes  */
};

/* BITPLANE format:
 * width_bytes of line 0 plane 0
 * width_bytes of line 0 plane 1 ...
 * width_bytes of line 1 plane 0
 * width_bytes of line 1 plane 1 ...
 * ...
 */

ASM(BOOL) CPic_recog(REG(a0, UBYTE *d), REG(d0, ULONG len),
  REG(a1, struct xfdRecogResult *rr))
{
  int width_bytes;
  if (EndGetM32(d) == 0x43506963 /* CPic */) {
    /* aligned to 16 pixels -- IFF ILBM word alignment */
    width_bytes = ((EndGetM16(&d[0x04]) + 15) >> 3) & 0xFFFE;
    rr->xfdrr_MinTargetLen = rr->xfdrr_FinalTargetLen =
      IFF_HEADER_LEN + (width_bytes * EndGetM16(&d[0x06]) * d[0x11]);
    return (BOOL) 1;
  }
  return (BOOL) 0;
}

#define RLE_REP (0)
#define RLE_CPY (1)

ASM(BOOL) CPic_decrunch(REG(a0, struct xfdBufferInfo * xbi),
  REG(a6, struct xfdMasterBase *xfdMasterBase))
{
  UBYTE *src  = (UBYTE *) xbi->xfdbi_SourceBuffer;
  UBYTE *dest = (UBYTE *) xbi->xfdbi_UserTargetBuf;
  UBYTE *d = dest, *d2, *d3, *s, *ends, rlem, rleb, rlec;
  UWORD width_bytes, height, bitplanes, picw, pich, picx, picy;
  int iff_line_bytes, x, y, pl;
  ULONG length;

  width_bytes = ((EndGetM16(&src[0x04]) + 15) >> 3) & 0xFFFE;
  height      = EndGetM16(&src[0x06]);
  picx        = EndGetM16(&src[0x08]);
  picy        = EndGetM16(&src[0x0A]);
  picw        = EndGetM16(&src[0x0C]);
  pich        = EndGetM16(&src[0x0E]);
  bitplanes   = EndGetM16(&src[0x10]);

  if ((picx+picw) > width_bytes || (picy+pich) > height || bitplanes > 6 ||
      picw == 0 || pich == 0 || bitplanes == 0)
  {
    xbi->xfdbi_Error = XFDERR_CORRUPTEDDATA;
    return 0;
  }

  /* number of bytes jump to get to the next line in IFF */
  iff_line_bytes = width_bytes * bitplanes;

  /* write IFF headers */

  memcpy(d, &iff_header[0], IFF_HEADER_LEN_STATIC);
  d += 4;
  length = xbi->xfdbi_TargetBufSaveLen - 8;
  *d++ = (length >> 24) & 0xFF;
  *d++ = (length >> 16) & 0xFF;
  *d++ = (length >>  8) & 0xFF;
  *d++ = (length      ) & 0xFF;
  d += 12;
  *d++ = src[0x04];  /* picture width (pixels) */
  *d++ = src[0x05];
  *d++ = src[0x06];  /* picture height (pixels) */
  *d++ = src[0x07];
  d += 4;
  *d++ = src[0x11]; /* bitplanes */
  d += 7;
  *d++ = src[0x04]; /* picture width (pixels) */
  *d++ = src[0x05];
  *d++ = src[0x06];  /* picture height (pixels) */
  *d++ = src[0x07];
  d += 10;
  *d++ = src[0x42] & 0x88; /* viewport mode, useful bits are 8000 (hires), */
  *d++ = src[0x43] & 0x84; /* 800 (ham), 80 (halfbrite) and 4 (lace)       */
  d += 8;
  for (x = 0; x < 32; x++) {
    UWORD colour = EndGetM16(&src[0x44 + x*2]);
    *d++ = ((colour >> 8) & 0xF) * 0x11;
    *d++ = ((colour >> 4) & 0xF) * 0x11;
    *d++ = ((colour     ) & 0xF) * 0x11;
  }
  *d++ = 'B';
  *d++ = 'O';
  *d++ = 'D';
  *d++ = 'Y';
  length = xbi->xfdbi_TargetBufSaveLen - IFF_HEADER_LEN;
  *d++ = (length >> 24) & 0xFF;
  *d++ = (length >> 16) & 0xFF;
  *d++ = (length >>  8) & 0xFF;
  *d++ = (length      ) & 0xFF;

  /* now do the actual decrunching... */
  memset(d, 0, length);                    /* clear memory */
  d += (iff_line_bytes * picy) + picx;     /* move to x,y offset */

  for (pl = 0; pl < bitplanes; pl++, d += width_bytes) {
    s    = &src[EndGetM32(&src[0x12 + pl*4])];
    ends = &s[  EndGetM32(&src[0x2A + pl*4])];
    if (s==src || s==ends || ends > &src[xbi->xfdbi_SourceBufLen]) continue;
    rlec = 0; d2 = d;
    for (x = 0; x < picw; x++, d2++) {
      d3 = d2;
      for (y = 0; y < pich; y++, d3 += iff_line_bytes, rlec--) {
        if (rlec == 0) {
          rlec = *s++;
          if (s > ends) {xbi->xfdbi_Error = XFDERR_CORRUPTEDDATA; return 0;}
          if      (rlec < 32)  rlec++,      rleb = 0x00, rlem = RLE_REP;
          else if (rlec < 128) rlec -= 31,               rlem = RLE_CPY;
          else if (rlec < 160) rlec -= 127, rleb = 0xFF, rlem = RLE_REP;
          else                 rlec -= 159, rleb = *s++, rlem = RLE_REP;
        }
        *d3 = (rlem == RLE_REP) ? rleb : *s++;
      }
    }
  }
  return 1;
}

struct xfdSlave FirstSlave = {
  NULL, XFDS_VERSION, 39, "(CPic) Data Cruncher",
  XFDPFF_DATA|XFDPFF_RECOGLEN|XFDPFF_USERTARGET,
  0, (BOOL (*)()) CPic_recog, (BOOL (*)()) CPic_decrunch,
  NULL, NULL, 0, 0, 0x84
};
