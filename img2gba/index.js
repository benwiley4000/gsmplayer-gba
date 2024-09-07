#!/bin/node

const fs = require('fs');
const path = require('path');

const { img2Gba } = require('./lib-img2gba');
const jpegDecode = require('./jpeg-decoder');

const imgPath = process.argv[2];
const outDir = process.argv[3];

// make sure directory exists
fs.readdirSync(outDir);

const imgData = fs.readFileSync(imgPath);

const { data, width, height } = jpegDecode(imgData);

const lastSlashIndex = Math.max(imgPath.lastIndexOf('/'), imgPath.lastIndexOf('\\'));
const imgName = imgPath.slice(lastSlashIndex + 1).replace(/\.jpeg$/, '');

for (const [currentName, blockSize] of [[imgName, 32], [`${imgName}_BG`, 8]]) {
    const { palette, bitmap } = img2Gba(data, width, height, blockSize);

    const palettePath = path.join(outDir, `${currentName}.pal.c`);
    const bitmapPath = path.join(outDir, `${currentName}.raw.c`);
    
    fs.writeFileSync(palettePath, `
    extern const unsigned short ${currentName}_Palette[${palette.length}] = {
        ${palette.map(p => `0x${p.toString(16).padStart(4, '0')}`).join(', ')}
    };
    `);
    
    
    fs.writeFileSync(bitmapPath, `
    extern const unsigned char ${currentName}_Bitmap[${bitmap.length}] = {
        ${bitmap.map(b => `0x${b.toString(16).padStart(2, '0')}`).join(', ')}
    };
    `);
}
