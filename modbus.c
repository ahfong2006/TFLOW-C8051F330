#include <c8051F330.h>
#include "crc.h"
#include "define.h"
#include "type.h"
#include "flash.h"
#include "F330_FlashPrimitives.h"
#include "F330_FlashUtils.h"

sbit    r_t=P0^3; //������ ��������
bit renewFlash;
/******************************************************************************
1.0					VARIABLES
******************************************************************************/
code baudrate br[8]={
  {0x21,0x00,0x96,0xf38a,0xe2ec},	//9600        osc/12
  {0x21,0x01,0x2c,0xe714,0xc5d9},	//14400       osc/4
  {0x21,0x01,0x61,0xed4f,0xd463},	//19200       osc/4
//  {0x21,0x01,0x96,0xed4f,0xd463},	//28800       osc/4
  {0x21,0x01,0xb1,0xed4f,0xd463},	//38400       osc/4
  {0x21,0x08,0x26,0xf9c5,0xf176},	//56000       osc/12
  {0x21,0x08,0x2c,0xf9c5,0xf176},	//57600       osc/12
  {0x21,0x08,0x96,0xf9c5,0xf176}	//115200      osc/12
};

//���������� �������� �������
modbus mb;

xdata union{
  unsigned char modbuf[modbufsize];
  struct{
    unsigned char address;
    unsigned char function;
    unsigned char exeption;        
  }exception;//����������� ��� �������� ��������
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
  }read_req;//������� �����
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned char byte_count;        
  }read_resp;//������� �������
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int register_address;
    unsigned int register_value;
  }write_req;//����� ������ ������� �����
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int register_address;
    unsigned int register_value;
  }write_resp;//����� ������ ������� �������
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
    unsigned char byte_count;
    unsigned int reg[24];
  }write_mult_req;//����� ������� �����
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
  }write_mult_resp;//����� ������� �������
  
}buf _at_ 0x00;

//������� �������
//��� �������
xdata union{
  unsigned int reg[9];
  SYS sys;
}in _at_ 0x80;//128 32-�����

//��� ������
xdata union{
  unsigned int reg[8];
  struct{
    unsigned int address;
    unsigned int baud;
    unsigned int corr;
    unsigned int ref;// voltage of reference in milivolts 2453 coresponds 2.453V
    unsigned int idac;// microamphere x 10 i.m. 5000 coresponds 0.5mA 
    unsigned int command;
  }out;
}out _at_ 0x80 + sizeof(in);//160 32-�����;

//������� ���������� ������ � �������
code w_a_v WAV[]={
  {&out.out.address,1,256},
  {&out.out.baud,0,6},
  {&out.out.corr,0,65535},
  {&out.out.ref,1000,3000},
  {&out.out.idac,5,32767},
  {&out.out.command,0,65535},
};
//���������� ��������� ������ � ��� ����������� ��������
struct{
  unsigned char tact;//0 ��� � th[0], 1 ��� � th[1], 2 crc_low, 3 crc_hi, 4 ��������� �� ������� 
  thread th[2];    
}u0;

/******************************************************************************
2.0					Predeclared functions
******************************************************************************/
void HandleReceiveInterrupt(void);
void HandleTransmitInterrupt(void);
void error();

/******************************************************************************
3.0					Functions itself
******************************************************************************/

void modbus_init()
{
  out.out.address = *((int code*)0x1a00);
  out.out.baud = (*((unsigned int code*)0x1a02)<=6)? *((unsigned int code*)0x1a02) : 0;
  out.out.corr = *((int code*)0x1a04);
  out.out.ref = *((int code*)0x1a06);
  out.out.idac = *((int code*)0x1a08);
  TMOD = br[(char)out.out.baud].tmod;
  CKCON = br[(char)out.out.baud].ckcon;
  TH1 = br[(char)out.out.baud].th1;
  mb.crcHi=0xff; 
  mb.crcLo=0xff;
  REN0 = 1;
}

void crc(unsigned char next_one)
{
  unsigned char crc_index;
  
  crc_index = mb.crcLo ^ next_one;
  mb.crcLo = CRC_hi [crc_index] ^ mb.crcHi;
  mb.crcHi = CRC_lo [crc_index];
}

void uart_isr (void) interrupt 4 using 1
{
  if(TI0){
    TI0 = 0;
    HandleTransmitInterrupt();
  }
  if(RI0){
    RI0 = 0;
    HandleReceiveInterrupt();
  }
}

/******************************************************************************
4.0					Functions itself predeclared in section 2.0
******************************************************************************/

void HandleReceiveInterrupt(void)
{
  unsigned int time;
  
  TR0 = 0;
  time = br[(char)out.out.baud].time15;			//�������� ������ �� 1,5�������
  TH0 = time>>8; TL0 = time; 
  TR0 = 1;
  
  if(mb.ptr > modbufsize-1) mb.ptr = modbufsize-1;//���������� ������� ������� (����� �������� � 0 ������.)
  *mb.ptr = SBUF0;
  mb.ptr++;
  mb.counter++;                   //������������� ������� ��������� ����� ��� ������� ������������ ������
  
  crc(SBUF0);
}

void HandleTransmitInterrupt(void)
{
  unsigned char byte;
  
  switch(u0.tact){
  case 0: //�������� ����� �� ���������
  case 1:
    byte = *u0.th[u0.tact].ptr;
    SBUF0 = byte;
    crc(byte);
    u0.th[u0.tact].ptr++;
    if(--u0.th[u0.tact].count == 0)u0.tact++;
    break;
    
  case 2:
    SBUF0 = mb.crcLo;
    u0.tact++;
    break;
    
  case 3:
    SBUF0 = mb.crcHi;
    u0.tact++;
    break;
    
  case 4:
    mb.counter = 0;         //�����������.
    mb.ptr = (unsigned char pdata *)&buf.modbuf;       
    mb.crcHi=0xff; 
    mb.crcLo=0xff;        
    byte = 0; while(--byte); r_t = 1;
    REN0 = 1;               //��������� ������
    break;
  }
}

bit flag_time35;
void timer_0 (void) interrupt 1 using 3
{ 
  unsigned int time;
  unsigned char i,offset;
  
  TF0 = 0; TR0 = 0; REN0 = 0;
  
  if(flag_time35){
    //����� ������� � 3,5 ������
    flag_time35 = 0;
    if(buf.modbuf[0] == 0){
      mb.ptr = 0;			//������� �������! � �����������.
      mb.crcHi=0xff; 
      mb.crcLo=0xff;
      mb.counter = 0;
      i = 0; while(--i); r_t = 0;
      REN0 = 1;
      return; //������� ������ ������� �� ������
    }
    r_t = 0; i = 0; while(--i);
    i = 0; while(--i); r_t = 0;
    TI0 = 1;
    return;    
  }
  
  /* �������� ���������� ���� */
  if((mb.crcHi | mb.crcLo)){
    mb.ptr = 0;			//������� �������! � �����������.
    mb.crcHi=0xff; 
    mb.crcLo=0xff;
    mb.counter = 0;
    REN0 = 1;
    return;				
  }
  
  /* ������ ������������ ������ � �� ��������� ������ */
  if(mb.counter >= modbufsize && buf.modbuf[0] == *(((char*)&out.out.address)+1)){
    buf.exception.exeption = 4; //��������� �������� 
    error();
    return;    
  }
  
  /* ������� */
  if(buf.modbuf[0] == 0 || buf.modbuf[0] == *(((char*)&out.out.address)+1)){//�������� ������ 
    switch(buf.modbuf[1]){				//function code
    case 3:                     //������� ������ ��� ����� ������� (�� ������� ������� � ������� RAM)
      if(buf.read_req.quantity > 125){
        buf.exception.exeption = 3; //�������� �������
        error();
        return;
      }
      /* ���������� ��������� ������ � ������ 256 ����� 128 16-����� ������� */
      if(buf.read_req.start_address>127 || buf.read_req.start_address*2 + buf.read_req.quantity*2-1 > 255){
        buf.exception.exeption = 2; //������� ������ ��� ��������
        error();
        return;
      }
      /* ��������� ������ */
      u0.tact = 0;
      u0.th[0].count = 3;                         //���������
      u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
      u0.th[1].count = buf.read_resp.byte_count = buf.read_req.quantity * 2; //���
      u0.th[1].ptr = buf.read_req.start_address*2;
      break;
      
    case 6:                     //����� � ������
      /* �������� ������ */
      buf.write_req.register_address *= 2;//����������� � �������� 8-���� ���������
      for(i=0;i<sizeof(WAV)/sizeof(w_a_v);i++){
        if(buf.write_req.register_address == WAV[i].address){
          if(buf.write_req.register_value >= WAV[i].min && buf.write_req.register_value <= WAV[i].max){
            
            /* ����� � ������ */
            *((int xdata*)buf.write_req.register_address) = buf.write_req.register_value;
            /* ������� ������� */
            buf.write_resp.register_value = *((int xdata*)buf.write_req.register_address);                            
            
            /* ���������� ������ ������ � ������, ��� � ������ ������� ������������ ���� � ���� */
            //�������� �� ����� ������ � ����
            if((buf.write_req.register_address >= (int)&out.out.address)&&
            (buf.write_req.register_address <= (int)&out.out.idac)){
                //����� �� ������������ ����
                renewFlash = 1;
            }
            
            buf.write_req.register_address = buf.write_req.register_address>>1;//����������� ����� � ������������ 16-���� ���������
            /* ��������� ������ */
            u0.tact = 0;
            u0.th[0].count = 4;                         //���������
            u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
            u0.th[1].count = 2; //���
            u0.th[1].ptr = (char pdata*)&buf.write_resp.register_value;
            break;
          }
          else{
            buf.exception.exeption = 3; //������� ��������
            error();
            return;
          }
        }
      }
      if(i == sizeof(WAV)/sizeof(w_a_v)){
        /*����������� ������*/
        buf.exception.exeption = 2;
        error();
        return;                
      }
      break;
      
    case 16:
      /* �������� ������ */
      buf.write_mult_req.start_address *= 2;//����������� � �������� 8-���� ���������
      // ���������� ������� 16-����� ������� �������
      if(((buf.write_mult_req.start_address - (int)&out)/2 + buf.write_mult_req.quantity > sizeof(out.reg)/sizeof(int)-2) ||//-2 ��� �������� ������ ������ � ��������
         (buf.write_mult_req.quantity*2 != buf.write_mult_req.byte_count)){
           buf.exception.exeption = 2; //������� ������ ��� ��������
           error();
           return;
         }
      offset = (buf.write_mult_req.start_address - (int)&out)/2;
      //����²��� �������� �������
      for(i=0;i<buf.write_mult_req.quantity;i++){
        if(buf.write_mult_req.reg[i] < WAV[offset + i].min || buf.write_mult_req.reg[i] > WAV[offset + i].max){
          buf.exception.exeption = 3; //������� ��������
          error();
          return;
        }
      }
      
      //���������
      for(i=0;i<buf.write_mult_req.quantity;i++){
        out.reg[offset + i] = buf.write_mult_req.reg[i];
      }
      //�������� �� ����� ������ � ����
      if((buf.write_mult_req.start_address >= (int)&out.out.address)&&
         (buf.write_mult_req.start_address <= (int)&out.out.idac)){
           //����� �� ������������ ����
           renewFlash = 1;
         }
      
      buf.write_mult_resp.start_address = buf.write_mult_req.start_address>>1;//����������� ����� � ������������ 16-���� ���������
      /* ��������� ������ */
      u0.tact = 0;
      u0.th[0].count = 4;                         //���������
      u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
      u0.th[1].count = 2; //���
      u0.th[1].ptr = (char pdata*)&buf.write_mult_resp.quantity;				
      break;
      
      
      
    default:
      /* ������� �� ����������� */
      buf.exception.exeption = 1;
      error();
      break;
    }
    
    flag_time35 = 1;
    time = br[(char)out.out.baud].time35;			//�������� ������ �� 3,5�������
    TH0 = time>>8; TL0 = time; 
    TR0 = 1;
    
    mb.crcHi=0xff; 
    mb.crcLo=0xff;
    
  }
  else{
    mb.ptr = 0;			
    mb.crcHi=0xff; 
    mb.crcLo=0xff;
    mb.counter = 0;
    REN0 = 1;
  }
  
}

void error()
{
  unsigned int time;
  
  mb.ptr = 0;			
  buf.exception.function |= 0x80;       
  
  u0.tact = 1;        //��� �� � ���e� ���� ����������� ������
  u0.th[1].count = 3; //������ ������� ��� �������
  u0.th[1].ptr = (unsigned char pdata *)&buf.modbuf;
  
  mb.crcHi=0xff; 
  mb.crcLo=0xff;
  mb.counter = 0;
  
  flag_time35 = 1;
  time = br[(char)out.out.baud].time35;			//�������� ������ �� 3,5�������
  TH0 = time>>8; TL0 = time; 
  TR0 = 1;    
}