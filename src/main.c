#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include "libgsm.h"
#include "hud.h"

int main(void) {
  // Enable vblank IRQ for VBlankIntrWait()
  irqInit();
  irqEnable(IRQ_VBLANK);

  // Hide everything
  REG_DISPCNT = 0;

  // Unpack font data and setup BG2 for HUD
  initHUD();

  // Flash copyright info on screen
  showGSMPlayerCopyrightInfo();

  // Display BG2 (HUD) on next VBlank
  VBlankIntrWait();
  REG_DISPCNT |= BG2_ON;

  // Wait for 6 seconds before starting player
  for (unsigned int i = 30 * 6; i > 0; --i) {
    VBlankIntrWait();
  }

  GsmPlaybackTracker playback;

  if (initPlayback(&playback) != 0) {
    return 1;
  }

  hud_cls();
  while (true) {
    advancePlayback(&playback, &DEFAULT_PLAYBACK_INPUT_MAPPING);
    VBlankIntrWait();
    writeFromPlaybackBuffer(&playback);
    drawHUDFrame(&playback);
  }
}
