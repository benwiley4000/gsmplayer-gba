#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include "libgsm.h"
#include "hud.h"

int main(void) {
  // Enable vblank IRQ for VBlankIntrWait()
  irqInit();
  irqEnable(IRQ_VBLANK);

  initHUD();

  for (unsigned int i = 180; i > 0; --i) {
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
