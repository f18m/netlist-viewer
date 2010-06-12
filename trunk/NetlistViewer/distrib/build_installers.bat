@del *.exe
@set PATH=%PATH%;%ProgramFiles%\NSIS;%ProgramFiles(x86)%\NSIS
makensis.exe /V2 setup.nsi
@echo.
@pause
