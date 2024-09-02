@echo off
echo converting art files...

echo Creating 16 color bitmaps..
for %%F in (art\*.jpg) do "C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe" convert -resize 128x128 -depth 4 -compress None -colors 15 -type palette "art\%%~nF.jpg" "art\%%~nF_16.bmp"

echo Creating 256 color bitmaps..
for %%F in (art\*.jpg) do "C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe" convert -resize 128x128 -depth 8 -compress None -colors 256 -type palette "art\%%~nF.jpg" "art\%%~nF_256.bmp"

echo Creating 16 color tiles..
for %%F in (art\*_16.bmp) do gfx2gba -x -osrc -fsrc -c16 -t8 -T32 -p"%%~nF.pal" "art\%%~nF.bmp"
echo Creating 256 color tiles..
for %%F in (art\*_256.bmp) do gfx2gba -x -osrc -fsrc -c256 -t8 -T32 -p"%%~nF.pal" "art\%%~nF.bmp"

echo Converting .wav files to .gsm format...
for %%F in (wavs\*.wav) do sox "%%F" -r 18157 -c 1 "gsms\%%~nF.gsm"
echo Building ROM...
C:\devkitPro\tools\bin\gbfs.exe gsmsongs.gbfs gsms\*

make -j8
