#include <gba_affine.h>
#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <stdlib.h>

#include "art.h"

extern const unsigned short leopard_Palette[16];
extern const unsigned char leopard_Bitmap[8192];

extern const char reelTiles[256];

void bitunpack2(void *restrict dst, const void *restrict src, size_t len)
{
  BUP tilespec = {
      .SrcNum = len, .SrcBitNum = 1, .DestBitNum = 4, .DestOffset = 0, .DestOffset0_On = 0};
  BitUnPack(src, dst, &tilespec);
}

// 4 * 4 * 1 (4 tile width, 4 tile height, x1 for s-tiles)
const u16 tile_block_size = 16;

void initArt()
{
    // Copy sprite palette and tile data into the appropriate locations
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Palette, SPRITE_PALETTE, sizeof(leopard_Palette));
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Bitmap, SPRITE_GFX, sizeof(leopard_Bitmap));
    // setup palette and tile data for reel animation
    // (use last palette to potentially allow multiple art palettes before)
    SPRITE_PALETTE[240] = RGB5(31, 31, 31);
    SPRITE_PALETTE[241] = RGB5(0, 0, 0);
    bitunpack2(SPRITE_GFX + 512 * 8, reelTiles, sizeof(reelTiles));
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
    // (album art)
    for (u16 i = 0; i < 16; i++)
    {
        OAM[i].attr0 |= OBJ_Y(8 + (32 * (i / 4))) | ATTR0_COLOR_16 | ATTR0_SQUARE;
        OAM[i].attr1 = OBJ_X(8 + (32 * (i % 4))) | ATTR1_SIZE_32;
    }

    // Set attributes for reel animation sprites
    // Reel 1
    u16 reelAnimationY = 8 * 13;
    u16 reelAnimationX = 8 * 19 + 4;
    u16 reel_tile_offset = 16;
    OAM[64].attr0 |= OBJ_Y(reelAnimationY) | ATTR0_COLOR_16 | ATTR0_SQUARE | ATTR0_ROTSCALE;
    OAM[64].attr1 = OBJ_X(reelAnimationX) | ATTR1_SIZE_32;
    OAM[64].attr2 = ATTR2_PALETTE(15) | (0x01ff & ((0 + reel_tile_offset) * tile_block_size));
    // Reel 2
    OAM[65].attr0 |= OBJ_Y(reelAnimationY) | ATTR0_COLOR_16 | ATTR0_SQUARE | ATTR0_ROTSCALE;
    OAM[65].attr1 = OBJ_X(reelAnimationX + 36) | ATTR1_SIZE_32;
    OAM[65].attr2 = ATTR2_PALETTE(15) | (0x01ff & ((0 + reel_tile_offset) * tile_block_size));
    // Tape 1
    OAM[66].attr0 |= OBJ_Y(reelAnimationY) | ATTR0_COLOR_16 | ATTR0_SQUARE;
    OAM[66].attr1 = OBJ_X(reelAnimationX + 15) | ATTR1_SIZE_32;
    OAM[66].attr2 = ATTR2_PALETTE(15) | (0x01ff & ((1 + reel_tile_offset) * tile_block_size));
    // Tape 2 (overlaps)
    OAM[66].attr0 |= OBJ_Y(reelAnimationY) | ATTR0_COLOR_16 | ATTR0_SQUARE;
    OAM[66].attr1 = OBJ_X(reelAnimationX + 17) | ATTR1_SIZE_32;
    OAM[66].attr2 = ATTR2_PALETTE(15) | (0x01ff & ((1 + reel_tile_offset) * tile_block_size));
}

void drawArt(GsmPlaybackTracker *playback)
{
    for (u16 i = 0; i < 16; i++)
    {
        // eventually for multiple arts we could use an offset
        // index multiplied by the cur_song index.
        OAM[i].attr2 = ATTR2_PALETTE(0) | (0x01ff & (tile_block_size * i));
        // enable sprite
        OAM[i].attr0 &= ~ATTR0_DISABLED;
    }

    for (u16 i = 64; i < 68; i++)
    {
        // enable sprite
        OAM[i].attr0 &= ~ATTR0_DISABLED;
    }

    // Set affine transform for reels
    for (u16 i = 64; i < 66; i++) {
        ObjAffineSource affineSrc;
        affineSrc.sX = 0x100;
        affineSrc.sY = 0x100;
        affineSrc.theta = playback->reel_rotation_theta + (1 << 13) * (i - 64);
        ObjAffineSet(&affineSrc, &((OBJAFFINE*)OAM)[i].pa, 1, 8);
    }
}
