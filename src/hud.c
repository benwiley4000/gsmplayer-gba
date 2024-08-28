#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <gba_input.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>  // for memset

#include "hud.h"

extern const char _x16Tiles[2048];  // font

void hud_init(void);
void hud_new_song(const char *name, unsigned int trackno);
void hud_frame(int locked, unsigned int t);

void initHUD() {
    hud_init();
}

void drawHUDFrame(GsmPlaybackTracker* playback, unsigned int nframes) {
    REG_BG0HOFS = nframes;
    hud_frame(playback->locked, nframes);
}

void dma_memset16(void *dst, unsigned int c16, size_t n) {
  volatile unsigned short src = c16;
  DMA_Copy(3, &src, dst, DMA_SRC_FIXED | DMA16 | (n>>1));
}

/**
 * does (uint64_t)x * frac >> 32
 */
uint32_t fracumul(uint32_t x, uint32_t frac) __attribute__((long_call));

void bitunpack1(void *restrict dst, const void *restrict src, size_t len) {
  // Load tiles
  BUP bgtilespec = {
    .SrcNum=len, .SrcBitNum=1, .DestBitNum=4, 
    .DestOffset=0, .DestOffset0_On=0
  };
  BitUnPack(src, dst, &bgtilespec);
}

/**
 * Writes a string to the HUD.
 */
static void hud_wline(unsigned int y, const char *s)
{
  unsigned short *dst = MAP[31][y * 2] + 1;
  unsigned int wid_left;

  // Write first 28 characters of text line
  for(wid_left = 28; wid_left > 0 && *s; wid_left--) {
    unsigned char c0 = *s++;

    dst[0] = c0 << 1;
    dst[32] = (c0 << 1) | 1;
    ++dst;
  }
  // Clear to end of line
  for(; wid_left > 0; wid_left--, dst++) {
    dst[0] = ' ' << 1;
    dst[32] = (' ' << 1) | 1;
  }
}

void hud_init(void)
{
  BG_COLORS[0] = RGB5(27, 31, 27);
  BG_COLORS[1] = RGB5(0, 16, 0);
  bitunpack1(PATRAM4(0, 0), _x16Tiles, sizeof(_x16Tiles));
  REG_DISPCNT = 0;
  REG_BG2CNT = SCREEN_BASE(31) | CHAR_BASE(0);

  hud_cls();
  hud_wline(1, "GSM Player for GBA");
  hud_wline(2, "Copr. 2004, 2019");
  hud_wline(3, "Damian Yerrick");
  hud_wline(4, "and Toast contributors");
  hud_wline(5, "(See TOAST-COPYRIGHT.txt)");

  VBlankIntrWait();
  REG_DISPCNT = 0 | BG2_ON;
}

/* base 10, 10, 6, 10 conversion */
static unsigned int hud_bcd[] =
{
  600, 60, 10, 1  
};


#undef BCD_LOOP
#define BCD_LOOP(b) if(src >= fac << b) { src -= fac << b; c += 1 << b; }

static void decimal_time(char *dst, unsigned int src)
{
  unsigned int i;

  for(i = 0; i < 4; i++)
    {
      unsigned int fac = hud_bcd[i];
      char c = '0';

      BCD_LOOP(3);
      BCD_LOOP(2);
      BCD_LOOP(1);
      BCD_LOOP(0);
      *dst++ = c;
    }
}

struct HUD_CLOCK
{
  unsigned int cycles;
  unsigned char trackno[2];
  unsigned char clock[4];
} hud_clock;

/**
 * @param locked 1 for locked, 0 for not locked
 * @param t offset in bytes from start of sample
 * (at 18157 kHz, 33/160 bytes per sample)
 */
void hud_frame(int locked, unsigned int t)
{
  char line[16];
  char time_bcd[4];

  /* a fractional value for Seconds Per Byte
     1/33 frame/byte * 160 sample/frame * 924 cpu/sample / 2^24 sec/cpu
     * 2^32 fracunits = 1146880 sec/byte fracunits
   */

  t = fracumul(t, 1146880);
  if(t > 5999)
    t = 5999;
  decimal_time(time_bcd, t);

  line[0] = (locked & KEY_SELECT) ? 12 : ' ';
  line[1] = (locked & KEY_START) ? 16 : ' ';
  line[2] = ' ';
  line[3] = hud_clock.trackno[0] + '0';
  line[4] = hud_clock.trackno[1] + '0';
  line[5] = ' ';
  line[6] = ' ';
  line[7] = time_bcd[0];
  line[8] = time_bcd[1];
  line[9] = ':';
  line[10] = time_bcd[2];
  line[11] = time_bcd[3];
  line[12] = '\0';
  hud_wline(9, line);
}

void hud_new_song(const char *name, unsigned int trackno)
{
  int upper;

  hud_wline(5, "Playing");
  hud_wline(6, name);
  hud_clock.cycles = 0;

  for(upper = 0; upper < 4; upper++)
    hud_clock.clock[0] = 0;
  upper = trackno / 10;
  hud_clock.trackno[1] = trackno - upper * 10;

  trackno = upper;
  upper = trackno / 10;
  hud_clock.trackno[0] = trackno - upper * 10;
}
