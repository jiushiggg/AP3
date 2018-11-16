REM @ECHO OFF
if "%1"=="" (SET/P tmp=input version:) else (SET tmp=%1)
if "%2"=="" (
	SET/P bleflg=°üº¬À¶ÑÀbinÎÄ¼þ£¿y/n:	
) else (
	SET bleflg=%2
)


if "%bleflg%" == "y" ( 
	SET/P BLEBIN=Input bin:
	SET/P BLEPATH=Input path:
	SET/P tmp2=input ble version:
	mv 
) else (
	SET BLEBIN= 
	SET BLEPATH= 
	SET tmp2=
)


REM echo %tmp%

SET myPath=%~dp0Debug
SET UploadProgramPath=..\UploadProgram
SET FILENAME=AP3_Dongle.bin
SET FILENAMEZIP=AP3_Dongle.zip



REM rename
if not exist "%UploadProgramPath%\rfd-%tmp%.bin" (
	mv %myPath%\%FILENAME%  %myPath%\rfd-%tmp%.bin
) else (
	cp	%UploadProgramPath%\rfd-%tmp%.bin %myPath%\rfd-%tmp%.bin
)

if "%bleflg%"=="y" (
	if not exist "%UploadProgramPath%\ble%tmp2%.bin" (
		cp %BLEPATH%\%BLEBIN% 		%myPath%\ble%tmp2%.bin
	) else (
		cp	%UploadProgramPath%\ble%tmp2%.bin %myPath%\ble%tmp2%.bin
	)
) 

REM compress

if "%bleflg%"=="y" (
"D:\Program Files (x86)\Haozip"\HaoZipC a -tzip %myPath%\%FILENAMEZIP% %myPath%\rfd-%tmp%.bin %myPath%\ble%tmp2%.bin
) else (
"D:\Program Files (x86)\Haozip"\HaoZipC a -tzip %myPath%\%FILENAMEZIP% %myPath%\rfd-%tmp%.bin
)

REM delete *.bin
REM del /P %myPath%\rfd-%tmp%.bin

REM Create file
if not exist "%UploadProgramPath%"  md %UploadProgramPath%

REM Copy file
if not exist "%UploadProgramPath%\rfd-%tmp%.bin"  (
	mv %myPath%\rfd-%tmp%.bin 	%UploadProgramPath%\rfd-%tmp%.bin
) else (
	del /P %myPath%\rfd-%tmp%.bin 
)

if "%bleflg%"=="y" 	(
	if "%BLEPATH%\%BLEBIN%"=="%UploadProgramPath%\ble%tmp2%.bin" (
		del /P %myPath%\ble%tmp2%.bin
	) else (
		mv %myPath%\%BLEBIN% 	%UploadProgramPath%\ble%tmp2%.bin	
	)	
)

REM open filezilla
"C:\Program Files (x86)\FileZilla FTP Client"\filezilla ftp://upload:upload@192.168.5.95/rfd_3.0 --local=%myPath%

REM delete the file of xx.zip
del /P %myPath%\%FILENAMEZIP%

