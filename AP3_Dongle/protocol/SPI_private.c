/*
 * SPI_private.c
 *
 *  Created on: 2018��8��13��
 *      Author: ggg
 *      private protocol
 *      SN(1Byte) | LEN(2Byte) | Data(0-512) | CRC(2Byte)
 *      SN���Ӱ�ɹ��������ݺ�SN�ż�1�����ظ�ĸ�壬ĸ����SN������һ�����ݡ�
 *      LEN:length�����һ�����ݳ������λ��1��0x8000����
 *      Data:data
 *      CRC: sn+len+data��CRC��
 *
 */



void SPI_PrivateDataInit(void);
{

}



