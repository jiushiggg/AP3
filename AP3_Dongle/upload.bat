@ECHO OFF
if "%1"=="" (SET/P tmp=input version:) else (SET tmp=%1)

REM echo %tmp%

SET myPatch=%~dp0Debug

REM rename
mv %myPatch%\AP3_Dongle.bin  %myPatch%\rfd-%tmp%.bin

REM compress
"D:\Program Files (x86)\Haozip"\HaoZipC a -tzip %myPatch%\AP3_Dongle.zip %myPatch%\rfd-%tmp%.bin

REM delete *.bin
del /P %myPatch%\rfd-%tmp%.bin

REM open filezilla
"C:\Program Files (x86)\FileZilla FTP Client"\filezilla ftp://upload:upload@192.168.5.95/rfd_3.0 --local=%myPatch%