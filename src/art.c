#include <gba_affine.h>
#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>
#include <gba_compression.h>
#include <stdlib.h>

#include "blend_defs.h"
#include "art.h"

// adapted from tonc
#define REG_BG_AFFINE		((BGAffineDest*)(REG_BASE+0x0000))	//!< Bg affine array

// These are basically the same tile data but one
// is arranged for use as a background and the other
// is arranged for use as a sprite
// (background is easier to blend to white)
// We probably could have just made both a background
// but I originally implement the foreground image as
// a sprite and don't want to mess around with it
// more right now.
extern const unsigned short leopard_Palette[256];
extern const unsigned char leopard_Bitmap[16384];
extern const unsigned short leopard_BG_Palette[256];
extern const unsigned char leopard_BG_Bitmap[16384];

extern const char reelTiles[384];

void bitunpack2(void *restrict dst, const void *restrict src, size_t len)
{
  BUP tilespec = {
      .SrcNum = len, .SrcBitNum = 1, .DestBitNum = 8, .DestOffset = 0, .DestOffset0_On = 0};
  BitUnPack(src, dst, &tilespec);
}

// 4 * 4 * 2 (4 tile width, 4 tile height, x2 for d-tiles)
const u16 tile_block_size = 32;

void initArt()
{
    // Draw solid background for blending with album background
    REG_BG2CNT = SCREEN_BASE(30)
        | CHAR_BASE(1)
        | BG_PRIORITY(3)
        | BG_256_COLOR
        | BG_SIZE_0;

    // Set up background tile map
    BGAffineSource bgAffineSrc;
    bgAffineSrc.theta = 0;
    // 2x display
    bgAffineSrc.sX = 1 << 7;
    bgAffineSrc.sY = 1 << 7;
    // Center on screen
    bgAffineSrc.tX = 120;
    bgAffineSrc.tY = 80;
    // Center image
    bgAffineSrc.x = 64 << 8;
    bgAffineSrc.y = 64 << 8;
    // Apply affine transform
    BgAffineSet(&bgAffineSrc, &(REG_BG_AFFINE[2]), 1);

    // Fill out screen entries
    u16* screenEntries = SCREEN_BASE_BLOCK(30);
    for (u16 i = 0; i < 256; i += 2) {
        // We have to write 2 bytes at a time
        // because VRAM can only be written in
        // 2 or 4 byte chunks
        screenEntries[i >> 1] = i | ((i + 1) << 8);
    }

    REG_BLDCNT = BLD_BUILD(
        BLD_BG2,
        0,
        2
    );
    REG_BLDY = BLDY_BUILD(12); // out of 15

    // Copy sprite palette and tile data into the appropriate locations
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Palette, SPRITE_PALETTE, sizeof(leopard_Palette));
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_Bitmap, SPRITE_GFX, sizeof(leopard_Bitmap));
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_BG_Palette, BG_PALETTE, sizeof(leopard_BG_Palette));
    while (REG_DMA3CNT & DMA_ENABLE)
        VBlankIntrWait();
    dmaCopy(leopard_BG_Bitmap, PATRAM8(1, 0), sizeof(leopard_BG_Bitmap));

    // setup palette and tile data for reel animation
    // (use last palette to potentially allow multiple art palettes before)
    SPRITE_PALETTE[0] = RGB5(31, 31, 31);
    SPRITE_PALETTE[1] = RGB5(12, 12, 12);
    bitunpack2(SPRITE_GFX + 512 * 16, reelTiles, sizeof(reelTiles));

    // wait for vblank so we can access OAM (and also make sure dma copy is done)
    do
    {
        VBlankIntrWait();
    } while (REG_DMA3CNT & DMA_ENABLE);

    // Disable all sprites
    for (u16 i = 0; i < 128; i++)
    {
        OAM[i].attr0 = ATTR0_DISABLED;
        OAM[i].attr1 = 0;
        OAM[i].attr2 = 0;
    }

    // Set attributes on first 16 sprites
    // (album art)
    for (u16 i = 0; i < 16; i++)
    {
        OAM[i].attr0 |= OBJ_Y(8 + (32 * (i / 4)))
            | ATTR0_COLOR_256
            | ATTR0_SQUARE;
        OAM[i].attr1 |= OBJ_X(8 + (32 * (i % 4)))
            | ATTR1_SIZE_32;
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

void drawArt(GsmPlaybackTracker *playback)
{
    // enable album art sprites
    for (u16 i = 0; i < 16; i++)
    {
        // eventually for multiple arts we could use an offset
        // index multiplied by the cur_song index.
        OAM[i].attr2 |= (0x01ff & (tile_block_size * i));
        // enable sprite
        OAM[i].attr0 &= ~ATTR0_DISABLED;
    }

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
