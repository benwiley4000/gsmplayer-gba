#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>

#include "art.h"

extern const unsigned short leopard_Palette[256];
extern const unsigned char leopard_Bitmap[16384];

void initArt()
{
    // Copy sprite palette and tile data into the appropriate locations
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Palette, SPRITE_PALETTE, sizeof(leopard_Palette));
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Bitmap, SPRITE_GFX, sizeof(leopard_Bitmap));
    // wait for vblank so we can access OAM (and also make sure dma copy is done)
    do
    {
        VBlankIntrWait();
    } while (REG_DMA3CNT & DMA_ENABLE);

    // Disable all sprites
    for (u16 i = 0; i < 128; i++)
    {
        OAM[i].attr0 = ATTR0_DISABLED;
    }

    // Set attributes on first 16 sprites
    // (we won't use the rest)
    for (u16 i = 0; i < 16; i++)
    {
        OAM[i].attr0 |= OBJ_Y(8 + (32 * (i / 4))) | ATTR0_COLOR_256 | ATTR0_SQUARE;
        OAM[i].attr1 = OBJ_X(8 + (32 * (i % 4))) | ATTR1_SIZE_32;
    }
}

void drawArt(GsmPlaybackTracker *playback)
{
    // 4 * 4 * 2 (4 tile width, 4 tile height, x2 for d-tiles)
    // [because palette256 addresses are 8-bit instead of 4-bit]
    u16 tile_block_size = 32;

    for (u16 i = 0; i < 16; i++)
    {
        // eventually for multiple arts we could use an offset
        // index multiplied by the cur_song index.
        OAM[i].attr2 = (0x01ff & (tile_block_size * i));
        // enable sprite
        OAM[i].attr0 &= ~ATTR0_DISABLED;
    }
}
