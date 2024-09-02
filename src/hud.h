#ifndef HUD_H
#define HUD_H
#include <gba_video.h>
#include <gba_dma.h>
#include <stdlib.h>

#include "libgsm.h"

void initHUD();
void showGSMPlayerCopyrightInfo();
void hud_show_instructions();
void drawHUDFrame(GsmPlaybackTracker* playback);

void dma_memset16(void *dst, unsigned int c16, size_t n);

static void hud_cls(void)
{
  dma_memset16(MAP[31], ' ' << 1, 32*20*2);
}

#endif
