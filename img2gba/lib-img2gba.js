
const quantize = require('./quantize');

const R_SHIFT = 0;
const G_SHIFT = 5;
const B_SHIFT = 10;

const TILE_WIDTH = 8;
const TILE_SIZE = TILE_WIDTH * TILE_WIDTH;

const PALETTE_SIZE = 256;
const RESERVED_PALETTE_SIZE = 16;
const MAX_COLORS = PALETTE_SIZE - RESERVED_PALETTE_SIZE;

/**
 * @param {number[]} data
 * @returns {{ palette: number[], data: number[] }}
 */
function quantizeData(data) {
    const arr = Array(data.length / 4);
    for (let i = 0; i < data.length / 4; i++) {
        const dataIndex = i * 4;
        arr[i] = [data[dataIndex + 0], data[dataIndex + 1], data[dataIndex + 2]];
    }

    const colorMap = quantize(arr, MAX_COLORS);
    const newData = Array(arr.length);
    const paletteMap = new Map([[0, 0]]);
    let paletteColorIndex = RESERVED_PALETTE_SIZE;
    for (let i = 0; i < arr.length; i++) {
        const [r, g, b] = colorMap.map(arr[i]);
        const [r_, g_, b_] = [r, g, b].map(c => c >> 3);
        let paletteColor = 0;
        paletteColor |= (r_ << R_SHIFT) & 0b11111;
        paletteColor |= (g_ << G_SHIFT) & 0b11111_00000;
        paletteColor |= (b_ << B_SHIFT) & 0b11111_00000_00000;
        let pIndex = paletteMap.get(paletteColor);
        if (typeof pIndex !== 'number') {
            pIndex = paletteColorIndex++;
            paletteMap.set(paletteColor, pIndex);
        }
        newData[i] = pIndex;
    }
    const palette = Array(PALETTE_SIZE).fill(0);
    for (const [color, index] of paletteMap) {
        palette[index] = color;
    }

    return { data: newData, palette };
}

/**
 * @param {number} bitmapIndex
 * @param {number} canvasWidth
 */
function bitmapToCanvasIndex(bitmapIndex, canvasWidth, blockWidth) {
    const blockSize = blockWidth * blockWidth;
    const tilesPerBlockRow = blockWidth / TILE_WIDTH;

    const tileBlocksPerCanvasRow = canvasWidth / blockWidth;

    const blockIndex = Math.floor(bitmapIndex / blockSize);
    const tileIndex = Math.floor((bitmapIndex % blockSize) / TILE_SIZE);
    const pixelY = Math.floor((bitmapIndex % TILE_SIZE) / TILE_WIDTH);
    const pixelX = Math.floor(bitmapIndex % TILE_WIDTH);

    const blockX = blockIndex % tileBlocksPerCanvasRow;
    const blockY = Math.floor(blockIndex / tileBlocksPerCanvasRow);
    const tileX = tileIndex % tilesPerBlockRow;
    const tileY = Math.floor(tileIndex / tilesPerBlockRow);

    const x = blockX * blockWidth + tileX * TILE_WIDTH + pixelX;
    const y = blockY * blockWidth + tileY * TILE_WIDTH + pixelY;

    return y * canvasWidth + x;
}

module.exports = {
    /**
     * @param {number[]} data
     * @param {number} width
     * @param {number} height
     * @param {number} blockSize
     * @returns {{ palette: number[], bitmap: number[] }}
     */
    img2Gba(data, width, height, blockSize) {
        if (width % blockSize || height % blockSize) {
            throw new Error('Image width and height must be multiples of block size');
        }

        const { palette, data: canvasData } = quantizeData(data);

        const tiledBitmap = Array(canvasData.length);
        for (let i = 0; i < tiledBitmap.length; i++) {
            tiledBitmap[i] = canvasData[bitmapToCanvasIndex(i, width, blockSize)];
        }

        const tiledBitmap4Bit = Array(tiledBitmap.length >> 1);
        for (let i = 0; i < tiledBitmap4Bit.length; i++) {
            const srcIndex = i << 1;
            const combined = (0b1111 & tiledBitmap[srcIndex])
                | ((0b1111 & tiledBitmap[srcIndex + 1]) << 4);
            tiledBitmap4Bit[i] = combined;
        }

        return {
            palette,
            // switch to 4 bit if we're using 16 color palettes
            bitmap: tiledBitmap,
        }
    },
}
