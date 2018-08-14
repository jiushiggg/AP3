/*
 * SPI_private.c
 *
 *  Created on: 2018年8月13日
 *      Author: ggg
 *      private protocol
 *      SN(1Byte) | LEN(2Byte) | Data(0-512) | CRC(2Byte)
 *      SN：子板成功接收数据后，SN号加1。返回给母板，母板用SN号组下一包数据。
 *      LEN:length。最后一包数据长度最高位置1（0x8000）。
 *      Data:data
 *      CRC: sn+len+data的CRC。
 *
 */



void SPI_PrivateDataInit(void);
{

}



