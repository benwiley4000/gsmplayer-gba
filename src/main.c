#include <gba_video.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>

#include "libgsm.h"
#include "hud.h"
#include "art.h"

struct GsmPlaybackInputMapping InputMapping = {
    .TOGGLE_PLAY_PAUSE = KEY_A | KEY_B,
    .PREV_TRACK = KEY_LEFT,
    .NEXT_TRACK = KEY_RIGHT,
    .SEEK_BACK = KEY_L,
    .SEEK_FORWARD = KEY_R,
    .TOGGLE_LOCK = KEY_SELECT,
};

int TOGGLE_INFO = KEY_START;

int main(void)
{
  // Enable vblank IRQ for VBlankIntrWait()
  irqInit();
  irqEnable(IRQ_VBLANK);

  // Hide everything
  REG_DISPCNT = 0;

  // Unpack font data and setup BG2 for HUD
  initHUD();

  // Prepare art
  initArt();
  // (we can enable because all sprites are still hidden)
  REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;

  // Display BG2 (HUD) on next VBlank
  VBlankIntrWait();
  REG_DISPCNT |= BG2_ON;

  GsmPlaybackTracker playback;

  if (initPlayback(&playback) != 0)
  {
    return 1;
  }

  drawArt(&playback);
  hud_cls();
  while (true)
  {
    advancePlayback(&playback, &InputMapping);
    VBlankIntrWait();
    writeFromPlaybackBuffer(&playback);
    drawHUDFrame(&playback);
    if (!(REG_KEYINPUT & TOGGLE_INFO))
    {
      REG_DISPCNT &= ~OBJ_ON;
      showGSMPlayerCopyrightInfo();
    }
    else
    {
      REG_DISPCNT |= OBJ_ON;
      hud_show_instructions();
    }
  }
}
