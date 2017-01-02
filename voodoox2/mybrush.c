
// This code is NOT GPL! Included in VX with the kind
// permission of the author Simone Tellini <wiz@vapor.com>
// It is part of the SpeedBar developer distribution.
// Please do not make modifications to it.
// A GPL compatible reimplementation needs to be written.


/*
** Id: mybrush.c,v 1.4 1999/09/27 18:16:53 wiz Exp 
*/


#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/modeid.h>
#include <graphics/gfxbase.h>
#include <datatypes/pictureclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include "mybrush.h"


extern struct GfxBase  *GfxBase;

void FreeImg( struct MyImage * );

/* LoadIFF */
UBYTE *LoadIFF( STRPTR File, struct MyImage *Img )
{
	BOOL                ok = FALSE;
	UBYTE              *Data = NULL;
	struct IFFHandle   *iff;
	
	if( !IFFParseBase )
	    return( NULL );
	if( iff = AllocIFF() )
	{		
		if( iff->iff_Stream = Open( File, MODE_OLDFILE ))
		{
			InitIFFasDOS( iff );
			if(!( OpenIFF( iff, IFFF_READ )))
			{
				WORD ret; /* AB: Changed from UWORD to WORD to stop VBCC from complaining. */
				PropChunk( iff, ID_ILBM, ID_BMHD );
				PropChunk( iff, ID_ILBM, ID_CMAP );
				StopChunk( iff, ID_ILBM, ID_BODY );	
				ret = ParseIFF( iff, IFFPARSE_SCAN );
				if(( ret == 0 ) || ( ret == IFFERR_EOF ))
				{	
					struct StoredProperty  *prop;
					
					if( prop = FindProp( iff, ID_ILBM, ID_BMHD ))
					{
						struct BitMapHeader *bmh;
						ULONG                Size, PlaneSize;
						UWORD                RowBytes;
						bmh = prop->sp_Data;
						if( prop = FindProp( iff, ID_ILBM, ID_CMAP ))
						{
							if( Img->CReg = AllocVec( prop->sp_Size, MEMF_ANY ))
								CopyMem( prop->sp_Data, Img->CReg, prop->sp_Size );
						}
						Img->Width  = bmh->bmh_Width;
						Img->Height = bmh->bmh_Height;
						Img->Depth  = bmh->bmh_Depth;
						RowBytes  = ((( bmh->bmh_Width + 15 ) & 0xFFF0 ) >> 3 );
						PlaneSize = RowBytes  * Img->Height;
						Size      = PlaneSize * bmh->bmh_Depth;
						Img->BytesPerRow = RowBytes;
						if(( Img->Depth <= 8 ) && ( Data = AllocVec( Size, MEMF_CLEAR )))
						{
							struct ContextNode *cn;
							UWORD               i;
							UBYTE              *plane;
							plane = Data;
							for( i = 0; i < bmh->bmh_Depth; i++ )
							{
								Img->Planes[ i ]  = plane;
								plane            += PlaneSize;
							}
							cn = CurrentChunk( iff );
							if( cn->cn_ID == ID_BODY )
							{
								BYTE *buf;							
								if( buf = AllocVec( cn->cn_Size, MEMF_CLEAR ))
								{
									BYTE *buf2;
									buf2 = buf;						
									ReadChunkBytes( iff, buf, cn->cn_Size );
									switch( bmh->bmh_Compression )
									{
										UWORD  c, d, e, f;
										BYTE  *dest, *dest2;	
										case 0:
											for( c = 0; c < bmh->bmh_Height; c++ )
											{
												dest = Data;
												for( d = 0; d < bmh->bmh_Depth; d++ )
												{
													dest2 = dest + ( c * RowBytes );
													for( e = 0; e < RowBytes; e++ )
														*dest2++ = *buf++;
													dest += PlaneSize;
												}
											}
											ok = TRUE;
											break;										
										case 1:
											for( c = 0; c < bmh->bmh_Height; c++ )
											{
												dest = Data;
												for( d = 0; d < bmh->bmh_Depth; d++ )
												{
													dest2 = dest + ( c * RowBytes );
													e = 0;
													do
													{
														BYTE new = *buf++;
														if( new >= 0 )
														{
															e += ( new + 1 );
															for( f = 0; f <= new; f++ )
																*dest2++ = *buf++;
														}
														else
														{
															if( new != -128 )
															{
																BYTE put;
																new = -new;
																e += ( new + 1 );
																put = *buf++;
																for( f = 0; f <= new; f++ )
																	*dest2++ = put;
															}
														}
													}
													while( e < RowBytes );
													dest += PlaneSize;
												}
											}
											ok = TRUE;
											break;
									}									
									FreeVec( buf2 );
								}
							}
						}
					}
				}		
				CloseIFF( iff );
			}		
			Close( iff->iff_Stream );
		}		
		FreeIFF( iff );
	}
	if(!( ok ))
	{
		FreeVec( Data );
		Data = NULL;
	}
	return( Data );
}

/* LoadImage */
struct MyImage *LoadImage( STRPTR File )
{
	struct MyImage *Img;
	//BOOL            ok = FALSE;

	if( Img = AllocMem( sizeof( struct MyImage ), MEMF_CLEAR ))
	{
		if(!(Img->ImageData = (UWORD *)LoadIFF( File, Img ))) /* Casting changed by AB */
		{
			FreeImg( Img );
			Img = NULL;
		}
	}
	return( Img );
}

/* FreeImg */
void FreeImg( struct MyImage *Img )
{
	if( Img )
	{
		FreeVec( Img->ImageData );
		FreeVec( Img->CReg );
		FreeMem( Img, sizeof( struct MyImage ));
	}
}

/* CreateBitMap */
struct BitMap *CreateBitMap( ULONG width, ULONG height, ULONG depth, ULONG flags, struct BitMap *friend )
{
	struct BitMap *bm;

	if( GfxBase->LibNode.lib_Version >= 39 )
	{
		bm = AllocBitMap( width, height, depth, flags | BMF_CLEAR, friend );
	}
	else
	{
		if( bm = AllocMem( sizeof( struct BitMap ), 0L ))
		{
			InitBitMap( bm, depth, width, height );

			if( bm->Planes[0] = (PLANEPTR) AllocVec( depth * RASSIZE( width, height ), MEMF_CHIP | MEMF_CLEAR ))
			{
				LONG i;

				for( i = 1; i < depth; i++ )
					bm->Planes[ i ] = bm->Planes[ i - 1 ] + RASSIZE( width, height );

			}
			else
			{
				FreeMem( bm, sizeof( struct BitMap ));
				bm = NULL;
			}
		}
	}
	return( bm );
}

/* DeleteBitMap */
void DeleteBitMap( struct BitMap *bm )
{
	if( bm )
	{
		if( GfxBase->LibNode.lib_Version >= 39 )
		{
			FreeBitMap( bm );
		}
		else
		{
			FreeVec( bm->Planes[0] );
			FreeMem( bm, sizeof( struct BitMap ));
		}
	}
}

/* LoadBrush */
struct MyBrush *LoadBrush( STRPTR File )
{
	struct MyImage *Img;
	struct MyBrush *Brush = NULL;

	if( Img = LoadImage( File ))
	{
		if( Brush = AllocMem( sizeof( struct MyBrush ), MEMF_CLEAR ))
		{
			BOOL    err = TRUE;
			Brush->Width  = Img->Width;
			Brush->Height = Img->Height;
			if( Brush->BitMap = CreateBitMap( Brush->Width, Brush->Height, Img->Depth, BMF_DISPLAYABLE, NULL ))
			{
				ULONG   i, row;
				row = ( Brush->Width + 7 ) >> 3;
				for( i = 0; i < Img->Depth; i++ )
				{
					UBYTE  *from, *to;
					ULONG   h;
					from = Img->Planes[ i ];
					to   = Brush->BitMap->Planes[ i ];
					for( h = 0; h < Img->Height; h++ )
					{
						CopyMem( from, to, row );
						from += Img->BytesPerRow;
						to   += Brush->BitMap->BytesPerRow;
					}
				}
				i = ( 1L << Img->Depth ) * 3;
				if( Brush->Colors = AllocVec( i * sizeof( ULONG ), MEMF_ANY ))
				{
					UBYTE  *from;
					if( from = Img->CReg )
					{
						ULONG   n, *to;
						to = Brush->Colors;	
						for( n = 0; n < i; n++ )
						{
							UBYTE   c;
							c = *from++;	
							*to++ = ( c << 24 ) | ( c << 16 ) | ( c << 8 ) | c;
						}
					}
					err = FALSE;
				}
			}	
			if( err )
			{
				FreeMem( Brush, sizeof( struct MyBrush ));
				Brush = NULL;
			}
		}
		FreeImg( Img );
	}
	return( Brush );
}

/* FreeBrush */
void FreeBrush( struct MyBrush *Brush )
{
	if( Brush )
	{
		DeleteBitMap( Brush->BitMap );
		FreeVec( Brush->Colors );
		FreeMem( Brush, sizeof( struct MyBrush ));
	}
}




