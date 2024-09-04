@echo off

echo Resizing images...
@REM Made-up convention, but inputs are .jpg and resized are .jpeg
for %%F in (art\*.jpg) do "C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe" convert -resize 128x128 "art\%%~nF.jpg" "art\%%~nF.jpeg"

echo Creating 256 color tiles...
for %%F in (art\*.jpeg) do node .\img2gba "art\%%~nF.jpeg" .\src 

echo Converting .wav files to .gsm format...
for %%F in (wavs\*.wav) do sox "%%F" -r 18157 -c 1 "gsms\%%~nF.gsm"

echo Compiling .gsm files into a .gbfs archive...
powershell .\GoGBFS.ps1

echo Building ROM...
make -j8
