 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Floppy disk emulation
  *
  * (c) 1995 Bernd Schmidt, Hannu Rummukainen
  */

#include <stdio.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "threaddep/penguin.h"
#include "memory.h"
#include "gensound.h"
#include "sounddep/sound.h"
#include "events.h"
#include "custom.h"
#include "ersatz.h"
#include "disk.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "readcpu.h"
#include "newcpu.h"
#include "xwin.h"
#include "osemu.h"
#include "execlib.h"

#define FLOPPY_GAP_LEN 360

#define FLOPPY_SPEED 4

uae_u16* mfmwrite;
static uae_u16 mfmwrbuffer[16384]; /* space for maximum disk DMA transfer */

static int side, direction, step;
static uae_u8 selected = 15;
static int dskready;

typedef struct {
    uae_u16 sync;
    uae_u16 len;
    uae_u32 offs;
} trackid;

#define MAX_TRACKS 328

typedef enum { ADF_NORMAL, ADF_EXT1 } drive_filetype;
typedef struct {
    FILE *diskfile;
    drive_filetype filetype;
    trackid trackdata[MAX_TRACKS];
    unsigned int buffered_cyl, buffered_side;
    unsigned int cyl;
    int side;
    int motoroff;
    int wrprot;
    uae_u16 bigmfmbuf[0x4000];
    int tracklen;
    unsigned long last_cycles;
    int mfmpos;
    unsigned int num_tracks, num_secs;
    /* dskchange is set to a small number when a disk is removed. At a step
     * impulse, it is reset if there is a new disk in the drive or counted
     * down to 1 if the drive contains a disk. DISK_check_change will only
     * insert a new disk when dskchange is 1, or if the dskchange_time timer
     * expires. The last condition can happen if AmigaOS isn't running and a
     * game/demo doesn't use the step motor to check for a new disk.
     */
    int dskchange;
    int dskchange_time;
} drive;

drive floppy[4];

static void drive_fill_bigbuf(drive *drv);

FILE *DISK_validate_filename (const char *fname, int leave_open, int *wrprot)
{
    FILE *f = zfile_open(fname, "r+b");
    if (f) {
	if (wrprot)
	    *wrprot = 0;
    } else {
	if (wrprot)
	    *wrprot = 1;
	f = zfile_open(fname, "rb");
    }
    if (!leave_open)
	fclose (f);
    return f;
}

static int drive_insert(drive *drv, int dnum, const char *fname)
{
    unsigned char buffer[10];

    drv->diskfile = DISK_validate_filename (fname, 1, &drv->wrprot);
    if (drv->diskfile == 0)
	return 0;

    strncpy (currprefs.df[dnum], fname, 255);
    currprefs.df[dnum][255] = 0;
    strncpy (changed_prefs.df[dnum], fname, 255);
    changed_prefs.df[dnum][255] = 0;
    gui_filename(dnum, fname);

    fread(buffer, sizeof(char), 8, drv->diskfile);
    if (strncmp((char *)buffer,"UAE--ADF",8) == 0) {
	int offs = 160*4+8;
	int i;

	drv->filetype = ADF_EXT1;
	drv->num_tracks = 160;
	drv->num_secs = 11;

	drv->wrprot = 1; /* write to adf_ext1 not implemented */
	for(i=0; i<160; i++) {
	    fread(buffer, 4, 1, drv->diskfile);
	    drv->trackdata[i].sync = buffer[0]*256 + buffer[1];
	    drv->trackdata[i].len = buffer[2]*256 + buffer[3];
	    drv->trackdata[i].offs = offs;
	    offs += drv->trackdata[i].len;
	}
    } else {
	unsigned int i;

	drv->filetype = ADF_NORMAL;

	fseek(drv->diskfile,0,SEEK_END);
	i = ftell(drv->diskfile);
	fseek(drv->diskfile,0,SEEK_SET);

	/* High-density disk? */
	if (i >= 160*22*512)
	    drv->num_tracks = i / (512*(drv->num_secs = 22));
	else
	    drv->num_tracks = i / (512*(drv->num_secs = 11));

	if (drv->num_tracks > MAX_TRACKS)
	    write_log ("Your diskfile is too big!\n");
	for(i = 0; i < drv->num_tracks; i++) {
	    drv->trackdata[i].len = 512 * drv->num_secs;
	    drv->trackdata[i].sync = 0;
	    drv->trackdata[i].offs = i * 512 * drv->num_secs;
	}
    }
    drv->buffered_side = 2; /* will force read */
    return 1;
}

static int drive_empty(drive *drv)
{
    return drv->diskfile == 0;
}

static void drive_step(drive *drv)
{
    if (!drive_empty(drv))
	drv->dskchange = 0;
    else if (drv->dskchange > 1) {
/*	printf("Stepping...\n");
	drv->dskchange--;*/
    }
    if (direction) {
	if (drv->cyl) drv->cyl--;
    } else {
	if (drv->cyl < drv->num_tracks/2) drv->cyl++;
    }
}


static int drive_track0(drive *drv)
{
    return drv->cyl == 0;
}

static int drive_writeprotected(drive *drv)
{
    return drv->wrprot || drv->diskfile == NULL;
}

static int drive_running(drive *drv)
{
    return !drv->motoroff;
}

static void drive_motor(drive *drv, int off)
{
    if (drv->motoroff && !off) {
	drv->last_cycles = cycles;
	drv->mfmpos = 0;
	eventtab[ev_diskindex].active = 1;
	if (drv->tracklen)
	    eventtab[ev_diskindex].evtime = FLOPPY_SPEED * drv->tracklen + cycles;
	else
	    eventtab[ev_diskindex].evtime = cycles + 1000;
	eventtab[ev_diskindex].oldcycles = cycles;
    } else if (off)
	eventtab[ev_diskindex].active = 0;
    drv->motoroff = off;
    events_schedule();
}

static void drive_fill_bigbuf(drive *drv)
{
    int tr = drv->cyl*2 + side;

    if (!drv->diskfile) {
	drv->tracklen = 10;
	memset (drv->bigmfmbuf,0xaa,drv->tracklen*2);
	return;
    }
    if (drv->buffered_cyl == drv->cyl && drv->buffered_side == side)
	return;

/*    if (drv->num_tracks <= tr) {
	write_log ("Access beyond end of floppy image!\n");
    }*/
    
    if (drv->trackdata[tr].sync == 0) {
	/* Normal AmigaDOS format track */
	unsigned int sec;
	drv->tracklen = drv->num_secs*544 + FLOPPY_GAP_LEN;
	memset(drv->bigmfmbuf,0xaa,FLOPPY_GAP_LEN*2);

	for (sec = 0; sec < drv->num_secs; sec++) {
	    uae_u8 secbuf[544];
	    int i;
	    uae_u16 *mfmbuf = drv->bigmfmbuf + 544*sec + FLOPPY_GAP_LEN;
	    uae_u32 deven,dodd;
	    uae_u32 hck=0,dck=0;

	    secbuf[0] = secbuf[1] = 0x00;
	    secbuf[2] = secbuf[3] = 0xa1;
	    secbuf[4] = 0xff;
	    secbuf[5] = tr;
	    secbuf[6] = sec;
	    secbuf[7] = drv->num_secs-sec;

	    for(i = 8; i < 24; i++)
		secbuf[i] = 0;

	    fseek(drv->diskfile,
		  drv->trackdata[tr].offs + sec*512,
		  SEEK_SET);
	    fread(&secbuf[32],1,512,drv->diskfile);

	    mfmbuf[0] = mfmbuf[1] = 0xaaaa;
	    mfmbuf[2] = mfmbuf[3] = 0x4489;

	    deven = ((secbuf[4] << 24) | (secbuf[5] << 16)
		     | (secbuf[6] << 8) | (secbuf[7]));
	    dodd = deven >> 1;
	    deven &= 0x55555555; dodd &= 0x55555555;

	    mfmbuf[4] = dodd >> 16;
	    mfmbuf[5] = dodd;
	    mfmbuf[6] = deven>> 16;
	    mfmbuf[7] = deven;

	    for (i = 8; i < 48; i++)
		mfmbuf[i] = 0;
	    for (i = 0; i < 512; i += 4){
		deven = ((secbuf[i+32] << 24) | (secbuf[i+33] << 16)
			 | (secbuf[i+34] << 8) | (secbuf[i+35]));
		dodd = deven >> 1;
		deven &= 0x55555555; dodd &= 0x55555555;
		mfmbuf[(i>>1)+32] = dodd >> 16;
		mfmbuf[(i>>1)+33] = dodd;
		mfmbuf[(i>>1)+256+32] = deven >> 16;
		mfmbuf[(i>>1)+256+33] = deven;
	    }

	    for(i = 4; i < 24; i += 2)
		hck ^= (mfmbuf[i] << 16) | mfmbuf[i+1];

	    deven = dodd = hck; dodd >>= 1;
	    mfmbuf[24] = dodd >> 16; mfmbuf[25] = dodd;
	    mfmbuf[26] = deven>> 16; mfmbuf[27] = deven;

	    for(i = 32; i < 544; i += 2)
		dck ^= (mfmbuf[i] << 16) | mfmbuf[i+1];

	    deven = dodd = dck; dodd >>= 1;
	    mfmbuf[28] = dodd >> 16; mfmbuf[29] = dodd;
	    mfmbuf[30] = deven>> 16; mfmbuf[31] = deven;
	}
    } else {
	int i;
	drv->tracklen = drv->trackdata[tr].len/2 + 1;
	drv->bigmfmbuf[0] = drv->trackdata[tr].sync;
	fseek(drv->diskfile, drv->trackdata[tr].offs, SEEK_SET);
	fread((char *)(drv->bigmfmbuf + 1), 1, drv->trackdata[tr].len, drv->diskfile);
	for (i = 0; i < drv->trackdata[tr].len/2; i++) {
	    uae_u16 *mfm = drv->bigmfmbuf + i + 1;
	    uae_u8 *data = (uae_u8 *)mfm;

	    *mfm = 256 * *data + *(data+1);
	}
    }
    drv->buffered_side = side;
    drv->buffered_cyl = drv->cyl;
    drv->last_cycles = cycles;
    drv->mfmpos = 0;
}

static int drive_get_data(drive *drv, uae_u16 *mfm, uae_u16 *byt)
{
    int offset,mfmpos;

    drive_fill_bigbuf(drv);
    offset = (cycles - drv->last_cycles) / FLOPPY_SPEED;
    mfmpos = (drv->mfmpos + offset) % drv->tracklen;
    drv->last_cycles += offset*FLOPPY_SPEED;
    drv->mfmpos = mfmpos;
    *mfm = drv->bigmfmbuf[mfmpos];
    return offset > 0;
}

#define MFMMASK 0x55555555
static __inline__ uae_u32 getmfmlong(uae_u16* mbuf)
{
    return ((*mbuf << 16) | *(mbuf + 1)) & MFMMASK;
}

static void drive_write_data(drive *drv, uae_u16 *mbuf, int length)
{
    int i, secwritten = 0;
    uae_u32 odd, even, chksum, id, dlong;
    uae_u8* secdata;
    uae_u8 secbuf[544];
    uae_u16 *mend = mbuf + length;

    if (drive_writeprotected(drv))
	return;

    drive_fill_bigbuf(drv);
    mend -= (4 + 16 + 8 + 512);
    while (length > 0) {
	int trackoffs;

	do {
	    while (*mbuf++ != 0x4489) {
		if (mbuf >= mend) return;
	    }
	} while (*mbuf++ != 0x4489);

	odd = getmfmlong(mbuf);
	even = getmfmlong(mbuf+2);
	mbuf += 4;
	id = (odd << 1) | even;

	trackoffs = (id & 0xff00) >> 8;
	if (trackoffs > 10) {
	    printf("Disk write: weird sector number %d\n", trackoffs);
	    continue;
	}
	chksum = odd ^ even;
	for (i=0; i<4; i++) {
	    odd = getmfmlong(mbuf);
	    even = getmfmlong(mbuf+8);
	    mbuf += 2;

	    dlong = (odd << 1) | even;
	    if (dlong)  secwritten = -200;
	    chksum ^= odd ^ even;
	}  /* could check here if the label is nonstandard */
	mbuf += 8;
	odd = getmfmlong(mbuf); even = getmfmlong(mbuf+2); mbuf += 4;
	if ((((odd << 1) | even) != chksum) ||
	    (((id & 0x00ff0000) >> 16) != drv->cyl*2 + side)) {
	    write_log ("Disk write: checksum error on sector header\n");
	    continue;
	}
	odd = getmfmlong(mbuf); even = getmfmlong(mbuf+2); mbuf += 4;
	chksum = (odd << 1) | even;
	secdata = secbuf + 32;
	for (i=0; i<128; i++) {
	    odd = getmfmlong(mbuf); even = getmfmlong(mbuf+256); mbuf += 2;
	    dlong = (odd << 1) | even;
	    *secdata++ = dlong >> 24; *secdata++ = (dlong >> 16) & 0xff;
	    *secdata++ = dlong >> 8; *secdata++ = dlong;
	    chksum ^= odd ^ even;
	}
	mbuf += 256;
	if (chksum) {
	    write_log ("Disk write: data checksum error\n");
	    continue;
	}
	secwritten++;
	fseek(drv->diskfile,
	      drv->trackdata[drv->cyl*2 + side].offs + trackoffs*512,
	      SEEK_SET);
	fwrite(secbuf+32, sizeof(uae_u8), 512, drv->diskfile);
    }
    drv->buffered_side = 2; /* will force read */

    if (secwritten == 0)
	write_log ("Disk write in unsupported format\n");
    if (secwritten < 0)
	write_log ("Disk write: sector labels ignored\n");
}


static void drive_eject(drive *drv)
{
    if (!drive_empty(drv))
	zfile_close(drv->diskfile);

    drv->dskchange = 4;
    drv->dskchange_time = 20;
/*    printf("setting changed bit %d\n", drv-floppy);*/
    drv->diskfile = 0;
}

/* We use this function if we have no Kickstart ROM.
 * No error checking - we trust our luck. */
void DISK_ersatz_read (int tr, int sec, uaecptr dest)
{
    int i;
    uae_u8 *dptr = get_real_address(dest);
    fseek (floppy[0].diskfile, floppy[0].trackdata[tr].offs + sec*512, SEEK_SET);
    fread (dptr, 1, 512, floppy[0].diskfile);
}

void disk_eject(int num)
{
    gui_filename(num, "");
    drive_eject(floppy + num);
    *currprefs.df[num] = *changed_prefs.df[num] = 0;
}

void disk_insert(int num, const char *name)
{
    /* just to be sure */
    drive_eject(floppy + num);
    drive_insert(floppy + num, num, name);
}

void DISK_check_change (void)
{
    int i;
    static int count = 0;
    count++;

    for (i = 0; i < 4; i++) {
	if (strcmp (currprefs.df[i], changed_prefs.df[i]) != 0) {
	    if (currprefs.df[i][0] != '\0') {
		drive_eject(floppy + i);
		currprefs.df[i][0] = '\0';
		gui_filename(i, "");
	    } else if (floppy[i].dskchange == 1) {
		/* In theory, it should work without the dskchange test.
		 * In practice, it doesn't. */
		drive_insert (floppy + i, i, changed_prefs.df[i]);
	    } else if (floppy[i].dskchange > 1 && floppy[i].dskchange_time > 0) {
		/* Force the dskchange bit to go to 1 after a given timeout */
		if (--floppy[i].dskchange_time == 0) {
		    floppy[i].dskchange = 1;
		}
	    }
	}
    }
}

int disk_empty(int num)
{
    return drive_empty(floppy + num);
}

void DISK_init()
{
    int i;
    for (i = 0; i < 4; i++)
	if (!drive_insert (floppy+i, i, currprefs.df[i]))
	    disk_eject (i);

    if (disk_empty (0))
	write_log ("No disk in drive 0.\n");
}

static int ledstate[] = { 0,0,0,0 };

void DISK_select(uae_u8 data)
{
    int step_pulse;
    int dr;

    if (selected != ((data >> 3) & 15))
	dskready = 0;
    selected = (data >> 3) & 15;
    side = 1 - ((data >> 2) & 1);

    direction = (data >> 1) & 1;
    step_pulse = data & 1;
    if (step != step_pulse) {
	step = step_pulse;
	if (step == 0){
	    for (dr = 0; dr < 4; dr++){
		if (!(selected & (1 << dr))) {
		    drive_step(floppy + dr);
		}
	    }
	}
    }
    for (dr = 0; dr < 4; dr++){
	if (!(selected & (1<<dr))) {
	    drive_motor(floppy + dr, data >> 7);
	}
    }
    for (dr = 0; dr < 4; dr++) {
	int state = (!(selected & (1<<dr))) | !floppy[dr].motoroff;
	if (state == ledstate[dr])
	    continue;
	gui_led (dr + 1, state);
	ledstate[dr] = state;
	gui_ledstate &= ~(1 << (dr + 1));
	gui_ledstate |= state << (dr + 1);
    }
}

uae_u8 DISK_status()
{
    uae_u8 st = 0x3c;
    int dr;

    for (dr = 0; dr < 4; dr++){
	drive *drv = floppy + dr;
	if (!(selected & (1 << dr))) {
	    if (drive_running(drv)){
		if (dskready) st &= ~0x20;
		dskready = 1;
	    } else {
		st &= ~0x20; /* report drive ID */
	    }

	    if (drive_track0 (drv)) { st &= ~0x10; }
	    if (drive_writeprotected (drv)) { st &= ~8; }
	    if (drv->dskchange) {
		/*printf("changed bit set: %d\n",dr); */
		st &= ~0x4;
		if (drv->dskchange > 1)
		    drv->dskchange--;
	    }
	}
    }
    return st;
}

int DISK_GetData(uae_u16 *mfm,uae_u16 *byt)
{
    int dr;
    for (dr = 0; dr < 4; dr++){
	if (!(selected & (1<<dr))) {
	    return drive_get_data (floppy + dr, mfm, byt);
	}
    }
    return 0;
}

static uae_u16 mfm_read_buffer[0x4000];
static int mfm_read_length;

int DISK_PrepareReadMFM(int length, uae_u16 sync, int use_sync)
{
    int dr;

    mfm_read_length = 0;

    for (dr = 0; dr < 4; dr++) {
	if (!(selected & (1<<dr))) {
	    int time = 0;
	    uae_u16 *mfmp = mfm_read_buffer;
	    drive *drv = floppy + dr;
	    int offset,mfmpos;

	    drive_fill_bigbuf(drv);

	    offset = (cycles - drv->last_cycles) / FLOPPY_SPEED;
	    mfmpos = (drv->mfmpos + offset) % drv->tracklen;
	    drv->last_cycles += offset*FLOPPY_SPEED;
	    drv->mfmpos = mfmpos;
	    mfmpos++; mfmpos %= drv->tracklen;
	    while (drv->bigmfmbuf[mfmpos] != sync && mfmpos != drv->mfmpos && use_sync)
		mfmpos = (mfmpos + 1) % drv->tracklen, time++;

	    if (drv->bigmfmbuf[mfmpos] != sync && use_sync) {
		write_log ("warning: sync not found on disk read\n");
		return 0;
	    } else
		mfmpos++;

	    mfm_read_length = length;

	    while (length--) {
		*mfmp++ = drv->bigmfmbuf[mfmpos];
		mfmpos = (mfmpos + 1) % drv->tracklen;
		time++;
	    }

	    return time*FLOPPY_SPEED;
	}
    }
    return 0;
}

int DISK_ReadMFM(uaecptr target)
{
    int i;
    uae_u8 *mfmp = get_real_address(target);

    if (!chipmem_bank.check(target, mfm_read_length * 2)) {
	write_log ("warning: Bad disk DMA read pointer\n");
	return 0;
    }

    for (i = 0; i < mfm_read_length; i++) {
	*mfmp++ = mfm_read_buffer[i] >> 8;
	*mfmp++ = mfm_read_buffer[i];
    }
    return 1;
}

void DISK_InitWrite()
{
    mfmwrite = mfmwrbuffer;
}

void DISK_WriteData(int length)
{
    int dr;
    for (dr=0;dr<4;dr++){
	if (!(selected & (1<<dr))) {
	    drive_write_data(floppy + dr, mfmwrbuffer, length);
	}
    }
}
