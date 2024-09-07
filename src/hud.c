#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <gba_input.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>  // for memset

#include "album_info.h"
#include "hud.h"

extern const char _x16Tiles[2048];  // font

void hud_init(void);
void hud_frame(GsmPlaybackTracker* playback, unsigned int t);

void initHUD() {
    hud_init();
}

void drawHUDFrame(GsmPlaybackTracker* playback) {
    hud_frame(playback, playback->src_pos - playback->src_start_pos);
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
  BG_COLORS[0] = RGB5(31, 31, 31);
  BG_COLORS[1] = RGB5(0, 0, 0);
  bitunpack1(PATRAM4(0, 0), _x16Tiles, sizeof(_x16Tiles));
  REG_BG1CNT = SCREEN_BASE(31) | CHAR_BASE(0);
}

void showGSMPlayerCopyrightInfo() {
  hud_wline(1, artistName);
  hud_wline(2, albumName);
  hud_wline(3, "");
  hud_wline(4, "GSM Player for GBA");
  char copyrightLine[] = "C 2004, 2019 Damian Yerrick";
  copyrightLine[0] = 28; // this is where I put the copyright symbol
  hud_wline(5, copyrightLine);
  hud_wline(6, "and Toast contributors");
  hud_wline(7, "(See TOAST-COPYRIGHT.txt)");
  hud_wline(8, "");
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

void hud_show_instructions() {
  hud_wline(1, "                  Play: A/B\n");
  hud_wline(2, "                  Seek: L/R\n");
  hud_wline(3, "                  Skip: DPad\n");
  hud_wline(4, "                  Lock: Slct\n");
  hud_wline(5, "                  Info: Strt\n");
  hud_wline(6, "");
  hud_wline(7, "");
}

void hud_update_clock(unsigned int trackno)
{
  int upper;
  hud_clock.cycles = 0;

  for(upper = 0; upper < 4; upper++)
    hud_clock.clock[0] = 0;
  upper = trackno / 10;
  hud_clock.trackno[1] = trackno - upper * 10;

  trackno = upper;
  upper = trackno / 10;
  hud_clock.trackno[0] = trackno - upper * 10;
}

const unsigned int printable_max_len = 20 - 5;

void update_title_marquee(GsmPlaybackTracker *playback)
{
  if (!--playback->frames_until_marquee_update)
  {
    playback->marquee_offset++;
    if (playback->marquee_offset == playback->curr_song_name_len)
    {
      playback->marquee_offset = 0 - printable_max_len;
    }
    // wait 3 seconds if marquee has returned to starting
    // position, otherwise move at 3 fps (30 / 10 = 3)
    playback->frames_until_marquee_update =
        playback->marquee_offset == 0
            ? 90
            : 10;
  }
}

/**
 * @param t offset in bytes from start of sample
 * (at 18157 kHz, 33/160 bytes per sample)
 */
void hud_frame(GsmPlaybackTracker* playback, unsigned int t)
{
  char line[29];
  char time_bcd[4];

  hud_update_clock(playback->cur_song + 1);

  playback->reel_rotation_theta = t * 3;

  /* a fractional value for Seconds Per Byte
     1/33 frame/byte * 160 sample/frame * 924 cpu/sample / 2^24 sec/cpu
     * 2^32 fracunits = 1146880 sec/byte fracunits
   */

  t = fracumul(t, 1146880);
  if(t > 5999)
    t = 5999;
  decimal_time(time_bcd, t);

  line[0] = !playback->playing ? 16 : 17;
  line[1] = ' ';
  line[2] = hud_clock.trackno[0] + '0';
  line[3] = hud_clock.trackno[1] + '0';
  line[4] = ' ';
  char curr_char;
  int done_printing_name = 0;
  if (playback->curr_song_name_len > printable_max_len)
  {
    update_title_marquee(playback);
  }
  int offset = 0;
  for (int i = 0; i < printable_max_len; i++)
  {
    offset = i + playback->marquee_offset;
    curr_char = offset < 0 ? ' ' : playback->curr_song_name[offset];
    // a cheap way to avoid showing file extensions
    if (curr_char == '.') done_printing_name = 1;
    line[i + 5] = done_printing_name ? ' ' : curr_char;
  }
  line[20] = ' ';
  line[21] = playback->locked ? 12 : 13;
  line[22] = ' ';
  line[23] = time_bcd[0];
  line[24] = time_bcd[1];
  line[25] = ':';
  line[26] = time_bcd[2];
  line[27] = time_bcd[3];
  line[28] = '\0';
  
  hud_wline(9, line);
}
