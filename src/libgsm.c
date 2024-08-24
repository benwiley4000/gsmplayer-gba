#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <gba_dma.h>
#include <gba_sound.h>
#include <gba_input.h>
#include <gba_timers.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset

#include "gsm.h"
#include "private.h" /* for sizeof(struct gsm_state) */
#include "gbfs.h"
#include "libgsm.h"

#if 0
#define PROFILE_WAIT_Y(y) \
  do                      \
  {                       \
  } while (REG_VCOUNT != (y))
#define PROFILE_COLOR(r, g, b) (BG_COLORS[0] = RGB5((r), (g), (b)))
#else
#define PROFILE_WAIT_Y(y) ((void)0)
#define PROFILE_COLOR(r, g, b) ((void)0)
#endif

// gsmplay.c ////////////////////////////////////////////////////////

static void dsound_switch_buffers(const void *src)
{
  REG_DMA1CNT = 0;

  /* no-op to let DMA registers catch up */
  asm volatile("eor r0, r0; eor r0, r0" ::: "r0");

  REG_DMA1SAD = (intptr_t)src;
  REG_DMA1DAD = (intptr_t)0x040000a0; /* write to FIFO A address */
  REG_DMA1CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 |
                DMA_SPECIAL | DMA_ENABLE | 1;
}

#define TIMER_16MHZ 0

void init_sound(void)
{
  // TM0CNT_L is count; TM0CNT_H is control
  REG_TM0CNT_H = 0;
  // turn on sound circuit
  SETSNDRES(1);
  SNDSTAT = SNDSTAT_ENABLE;
  DSOUNDCTRL = 0x0b0e;
  REG_TM0CNT_L = 0x10000 - (924 / 2);
  REG_TM0CNT_H = TIMER_16MHZ | TIMER_START;
}

/* gsm_init() **************
   This is to gsm_create() as placement new is to new.
*/
void gsm_init(gsm r)
{
  memset((char *)r, 0, sizeof(*r));
  r->nrp = 40;
}

void wait4vbl(void)
{
  asm volatile("mov r2, #0; swi 0x05" ::: "r0", "r1", "r2", "r3");
}

struct gsm_state decoder;
const GBFS_FILE *fs;
const unsigned char *src;
uint32_t src_len;

int initPlayback(GsmPlaybackTracker *playback)
{
  fs = find_first_gbfs_file(find_first_gbfs_file);
  if (!fs)
  {
    return 1;
  }
  init_sound();
  playback->src_pos = NULL;
  playback->src_end = NULL;
  playback->decode_pos = 160;
  playback->cur_buffer = 0;
  playback->last_joy = 0x3ff;
  playback->cur_song = (unsigned int)(-1);
  playback->last_sample = 0;
  playback->locked = 0;
  playback->nframes = 0;
  return 0;
}

signed short out_samples[160];
signed char double_buffers[2][608] __attribute__((aligned(4)));

#define CMD_START_SONG 0x0400

void reset_gba(void) __attribute__((long_call));

void advancePlayback(GsmPlaybackTracker *playback)
{

  unsigned short j = (REG_KEYINPUT & 0x3ff) ^ 0x3ff;
  unsigned short cmd = j & (~playback->last_joy | KEY_R | KEY_L);
  signed char *dst_pos = double_buffers[playback->cur_buffer];

  playback->last_joy = j;

  /*
        if((j & (KEY_A | KEY_B | KEY_SELECT | KEY_START))
           == (KEY_A | KEY_B | KEY_SELECT | KEY_START))
          reset_gba();
  */

  if (cmd & KEY_SELECT)
  {
    playback->locked ^= KEY_SELECT;
  }

  if (playback->locked & KEY_SELECT)
  {
    cmd = 0;
  }

  if (cmd & KEY_START)
  {
    playback->locked ^= KEY_START;
  }

  if (cmd & KEY_L)
  {
    playback->src_pos -= 33 * 50;
    if (playback->src_pos < src)
    {
      cmd |= KEY_LEFT;
    }
  }

  // R button: Skip forward
  if (cmd & KEY_R)
  {
    playback->src_pos += 33 * 50;
  }

  // At end of track, proceed to the next
  if (playback->src_pos >= playback->src_end)
  {
    cmd |= KEY_RIGHT;
  }

  if (cmd & KEY_RIGHT)
  {
    playback->cur_song++;
    if (playback->cur_song >= gbfs_count_objs(fs))
    {
      playback->cur_song = 0;
    }
    cmd |= CMD_START_SONG;
  }

  if (cmd & KEY_LEFT)
  {
    if (playback->cur_song == 0)
    {
      playback->cur_song = gbfs_count_objs(fs) - 1;
    }
    else
    {
      playback->cur_song--;
    }
    cmd |= CMD_START_SONG;
  }

  if (cmd & CMD_START_SONG)
  {
    gsm_init(&decoder);
    src = gbfs_get_nth_obj(fs, playback->cur_song, playback->curr_song_name, &src_len);
    // If reached by seek, go near end of the track.
    // Otherwise, go to the start.
    if (cmd & KEY_L)
    {
      playback->src_pos = src + src_len - 33 * 60;
    }
    else
    {
      playback->src_pos = src;
    }
    playback->src_end = src + src_len;
  }

  PROFILE_WAIT_Y(0);

  if (playback->locked & KEY_START)
  { /* if paused */
    for (j = 304 / 2; j > 0; j--)
    {
      *dst_pos++ = playback->last_sample >> 8;
      *dst_pos++ = playback->last_sample >> 8;
      *dst_pos++ = playback->last_sample >> 8;
      *dst_pos++ = playback->last_sample >> 8;
    }
  }
  else
  {
    for (j = 304 / 4; j > 0; j--)
    {
      int cur_sample;
      if (playback->decode_pos >= 160)
      {
        if (playback->src_pos < playback->src_end)
        {
          gsm_decode(&decoder, playback->src_pos, out_samples);
        }
        playback->src_pos += sizeof(gsm_frame);
        playback->decode_pos = 0;
      }

      /* 2:1 linear interpolation */
      cur_sample = out_samples[playback->decode_pos++];
      *dst_pos++ = (playback->last_sample + cur_sample) >> 9;
      *dst_pos++ = cur_sample >> 8;
      playback->last_sample = cur_sample;

      cur_sample = out_samples[playback->decode_pos++];
      *dst_pos++ = (playback->last_sample + cur_sample) >> 9;
      *dst_pos++ = cur_sample >> 8;
      playback->last_sample = cur_sample;

      cur_sample = out_samples[playback->decode_pos++];
      *dst_pos++ = (playback->last_sample + cur_sample) >> 9;
      *dst_pos++ = cur_sample >> 8;
      playback->last_sample = cur_sample;

      cur_sample = out_samples[playback->decode_pos++];
      *dst_pos++ = (playback->last_sample + cur_sample) >> 9;
      *dst_pos++ = cur_sample >> 8;
      playback->last_sample = cur_sample;
    }
  }

  PROFILE_COLOR(27, 27, 27);

  dsound_switch_buffers(double_buffers[playback->cur_buffer]);
  PROFILE_COLOR(27, 31, 27);

  playback->cur_buffer = !playback->cur_buffer;
}
