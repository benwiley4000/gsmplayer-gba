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
  // And set to Mode 1 (BG 1+2 regular, BG 3 affine)
  REG_DISPCNT = MODE_1;

  // NOTE: art should be initialized before HUD,
  // because art will fill BG and sprite palettes,
  // and HUD will overwrite beginning of BG palette
  // afterward.

  // Prepare art
  initArt();
  // (we can enable because all sprites are still hidden)
  REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;

  // Unpack font data and setup BG1 for HUD
  initHUD();

  // Display BG1 (HUD) and BG2 (album art background) on next VBlank
  VBlankIntrWait();
  REG_DISPCNT |= BG1_ON | BG2_ON;

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
      drawArt(&playback);
    }
  }
}
