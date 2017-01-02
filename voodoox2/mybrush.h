/*
** $Id: mybrush.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
*/

#ifndef MYBRUSH_H
#define MYBRUSH_H

struct MyImage {
        UWORD           Width;
        UWORD           Height;
        UWORD          *ImageData;
        APTR            Datatype;
        ULONG           BytesPerRow;
        UBYTE           Depth;
        UBYTE          *CReg;
        UBYTE          *Planes[8];
};

struct MyBrush {
        UWORD           Width;
        UWORD           Height;
        struct BitMap  *BitMap;
        ULONG          *Colors;
};

extern struct MyBrush *LoadBrush( STRPTR );
extern void FreeBrush( struct MyBrush * );

#endif
