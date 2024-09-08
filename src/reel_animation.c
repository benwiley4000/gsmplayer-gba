#include <gba_affine.h>
#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <stdlib.h>

#include "reel_animation.h"

extern const char reelTiles[384];

void bitunpack2(void *restrict dst, const void *restrict src, size_t len)
{
  BUP tilespec = {
      .SrcNum = len, .SrcBitNum = 1, .DestBitNum = 8, .DestOffset = 0, .DestOffset0_On = 0};
  BitUnPack(src, dst, &tilespec);
}

// 4 * 4 * 2 (4 tile width, 4 tile height, x2 for d-tiles)
const u16 tile_block_size = 32;

void initReelAnimation()
{
    // setup palette and tile data for reel animation
    SPRITE_PALETTE[0] = RGB5(31, 31, 31);
    SPRITE_PALETTE[1] = RGB5(12, 12, 12);
    bitunpack2(SPRITE_GFX + 512 * 16, reelTiles, sizeof(reelTiles));

    // wait for vblank so we can access OAM (and also make sure dma copy is done)
    VBlankIntrWait();

    // Disable all sprites
    for (u16 i = 0; i < 128; i++)
    {
        OAM[i].attr0 = ATTR0_DISABLED;
        OAM[i].attr1 = 0;
        OAM[i].attr2 = 0;
    }

    // Set attributes for reel animation sprites
    // Reel 1
    u16 reelAnimationY = 8 * 12 + 4;
    u16 reelAnimationX = 8 * 19 + 5;
    u16 reel_tile_offset = 16;
    OAM[64].attr0 |= OBJ_Y(reelAnimationY)
        | ATTR0_COLOR_256
        | ATTR0_SQUARE
        | ATTR0_ROTSCALE
        | ATTR0_TYPE_BLENDED;
    OAM[64].attr1 |= OBJ_X(reelAnimationX)
        | ATTR1_SIZE_32
        | ATTR1_ROTDATA(64);
    OAM[64].attr2 |= (0x3FF & ((0 + reel_tile_offset) * tile_block_size))
        | ATTR2_PRIORITY(1);
    // Reel 2
    OAM[65].attr0 |= OBJ_Y(reelAnimationY)
        | ATTR0_COLOR_256
        | ATTR0_SQUARE
        | ATTR0_ROTSCALE
        | ATTR0_TYPE_BLENDED;
    OAM[65].attr1 |= OBJ_X(reelAnimationX + 32)
        | ATTR1_SIZE_32
        | ATTR1_ROTDATA(65);
    OAM[65].attr2 |= (0x3FF & ((0 + reel_tile_offset) * tile_block_size))
        | ATTR2_PRIORITY(1);
    // Tape
    OAM[66].attr0 |= OBJ_Y(reelAnimationY + 32)
        | ATTR0_COLOR_256
        | ATTR0_SQUARE;
    OAM[66].attr1 |= OBJ_X(reelAnimationX + 16)
        | ATTR1_SIZE_32;
    OAM[66].attr2 |= (0x3FF & ((2 + reel_tile_offset) * tile_block_size))
        | ATTR2_PRIORITY(0);
}

void drawReelAnimation(GsmPlaybackTracker *playback)
{
    // Set affine transform for reels
    for (u16 i = 64; i < 66; i++) {
        ObjAffineSource affineSrc;
        affineSrc.sX = 0x100;
        affineSrc.sY = 0x100;
        affineSrc.theta = playback->reel_rotation_theta + (64 - i) * (1 << 13);
        ObjAffineSet(&affineSrc, &((OBJAFFINE*)OAM)[i].pa, 1, 8);
    }

    // enable reel sprites (including tape)
    for (u16 i = 64; i < 67; i++)
    {
        // enable sprites
        OAM[i].attr0 &= ~ATTR0_DISABLED;
    }
}
