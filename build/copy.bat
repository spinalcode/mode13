@echo off

set VAR=Destination
for /f "skip=1" %%L in ('wmic logicaldisk where volumename^="CRP DISABLD" Get Caption') do @call :SetVar %%L

echo Copying data from %Source% to %Destination%

del %Destination%\firmware.bin
copy testcode.bin %Destination%\firmware.bin

goto :EOF

:SetVar
set Label=%1
if NOT [%Label%]==[] set %VAR%=%Label%
goto :EOF

