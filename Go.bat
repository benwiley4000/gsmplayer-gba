@echo off
echo Converting .wav files to .gsm format...
for %%F in (wavs\*.wav) do sox "%%F" -r 18157 -c 1 "gsms\%%~nF.gsm"
echo Building ROM...
C:\devkitPro\tools\bin\gbfs.exe gsmsongs.gbfs gsms\*

make -j8
