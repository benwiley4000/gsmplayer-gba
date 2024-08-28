#ifndef LIBGSM_H
#define LIBGSM_H

typedef struct GsmPlaybackTracker
{
  const unsigned char *src_start_pos;
  const unsigned char *src_pos;
  const unsigned char *src_end;
  unsigned int decode_pos;
  unsigned int cur_buffer;
  unsigned short last_joy;
  unsigned int cur_song;
  int last_sample;
  int playing;
  int locked;

  char curr_song_name[25];
} GsmPlaybackTracker;

typedef struct GsmPlaybackInputMapping
{
  int TOGGLE_PLAY_PAUSE;
  int PREV_TRACK;
  int NEXT_TRACK;
  int SEEK_BACK;
  int SEEK_FORWARD;
  int TOGGLE_LOCK;
} GsmPlaybackInputMapping;

extern struct GsmPlaybackInputMapping DEFAULT_PLAYBACK_INPUT_MAPPING;

int initPlayback(GsmPlaybackTracker *playback);

void advancePlayback(GsmPlaybackTracker *playback, GsmPlaybackInputMapping *mapping);

void writeFromPlaybackBuffer(GsmPlaybackTracker *playback);

#endif
