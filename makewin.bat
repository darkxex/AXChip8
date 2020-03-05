@RD /S /Q "C:\respaldo2017\RomsXShop\build"
xcopy "C:\respaldo2017\C++\Chip8\test\*.cpp" "C:\respaldo2017\Chip8\source\*.cpp" /y
xcopy "C:\respaldo2017\C++\Chip8\test\*.h" "C:\respaldo2017\Chip8\source\*.h" /y
make
pause
nxlink -s "out/AXChip8.nro" 