typedef struct GsmPlaybackTracker
{
  const unsigned char *src_pos;
  const unsigned char *src_end;
  unsigned int decode_pos;
  unsigned int cur_buffer;
  unsigned short last_joy;
  unsigned int cur_song;
  int last_sample;
  unsigned int locked;

  unsigned int nframes;

  char curr_song_name[25];
} GsmPlaybackTracker;

int initPlayback(GsmPlaybackTracker *playback);

void advancePlayback(GsmPlaybackTracker *playback);
