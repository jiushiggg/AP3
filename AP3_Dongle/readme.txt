需要的调试工具：
putty:使用telnet或串口打印log，SSH端口登录，
tftpd32.exe：建立一个tftp服务器，用于母板与PC端的文件传输。


使用的TI库
simplelink_cc2640r2_sdk_2_20_00_49

把编译后的子板bin文件放到如下目录中：
/home/elinker/bin/rf.bin
/home/elinker/bin/ble.bin

linux端常用指令：
mv
tftp -g -r xxx IPaddress
chmod


linux登录账户：root，密码：hanshow-imx6


cp2tftp.bat批处理文件：
删除tftp目录的下的bin文件，把自己目录下的文件复制到tftp目录下。
