#include <c8051F330.h>
#include "crc.h"
#include "define.h"
#include "type.h"
#include "flash.h"
#include "F330_FlashPrimitives.h"
#include "F330_FlashUtils.h"

sbit    r_t=P0^3; //прийом передача
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

//декларація атрибутів модбасу
modbus mb;

xdata union{
  unsigned char modbuf[modbufsize];
  struct{
    unsigned char address;
    unsigned char function;
    unsigned char exeption;        
  }exception;//повідомлення про виключну ситуацію
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
  }read_req;//читання Запит
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned char byte_count;        
  }read_resp;//читання відповідь
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int register_address;
    unsigned int register_value;
  }write_req;//запис одного регістра Запит
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int register_address;
    unsigned int register_value;
  }write_resp;//запис одного регістра відповідь
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
    unsigned char byte_count;
    unsigned int reg[24];
  }write_mult_req;//запис регістрів Запит
  
  struct{
    unsigned char address;
    unsigned char function;
    unsigned int start_address;
    unsigned int quantity;
  }write_mult_resp;//запис регістрів відповідь
  
}buf _at_ 0x00;

//регістри модбаса
//для читання
xdata union{
  unsigned int reg[9];
  SYS sys;
}in _at_ 0x80;//128 32-байти

//для запису
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
}out _at_ 0x80 + sizeof(in);//160 32-байти;

//таблиця регламенту запису у регістри
code w_a_v WAV[]={
  {&out.out.address,1,256},
  {&out.out.baud,0,6},
  {&out.out.corr,0,65535},
  {&out.out.ref,1000,3000},
  {&out.out.idac,5,32767},
  {&out.out.command,0,65535},
};
//організація вихідного потоку в два структурних елементи
struct{
  unsigned char tact;//0 дані з th[0], 1 дані з th[1], 2 crc_low, 3 crc_hi, 4 підготовка до прийому 
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
  time = br[(char)out.out.baud].time15;			//зарядити таймер на 1,5символа
  TH0 = time>>8; TL0 = time; 
  TR0 = 1;
  
  if(mb.ptr > modbufsize-1) mb.ptr = modbufsize-1;//обмежитись буфером прийому (буфер розміщено з 0 адреси.)
  *mb.ptr = SBUF0;
  mb.ptr++;
  mb.counter++;                   //інкреметувати кількість прийнятих байтів щоб виявити переповнення буфера
  
  crc(SBUF0);
}

void HandleTransmitInterrupt(void)
{
  unsigned char byte;
  
  switch(u0.tact){
  case 0: //відправка даних за поінтерами
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
    mb.counter = 0;         //Ініціалізація.
    mb.ptr = (unsigned char pdata *)&buf.modbuf;       
    mb.crcHi=0xff; 
    mb.crcLo=0xff;        
    byte = 0; while(--byte); r_t = 1;
    REN0 = 1;               //дозволити прийом
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
    //кінець тайауту в 3,5 симвла
    flag_time35 = 0;
    if(buf.modbuf[0] == 0){
      mb.ptr = 0;			//Помилка прийому! І Ініціалізація.
      mb.crcHi=0xff; 
      mb.crcLo=0xff;
      mb.counter = 0;
      i = 0; while(--i); r_t = 0;
      REN0 = 1;
      return; //нульова адреса відповідь не давати
    }
    r_t = 0; i = 0; while(--i);
    i = 0; while(--i); r_t = 0;
    TI0 = 1;
    return;    
  }
  
  /* перевірка контрольної суми */
  if((mb.crcHi | mb.crcLo)){
    mb.ptr = 0;			//Помилка прийому! І Ініціалізація.
    mb.crcHi=0xff; 
    mb.crcLo=0xff;
    mb.counter = 0;
    REN0 = 1;
    return;				
  }
  
  /* чибуло переповнення буфера і чи правильна адреса */
  if(mb.counter >= modbufsize && buf.modbuf[0] == *(((char*)&out.out.address)+1)){
    buf.exception.exeption = 4; //неможливо виконати 
    error();
    return;    
  }
  
  /* обробка */
  if(buf.modbuf[0] == 0 || buf.modbuf[0] == *(((char*)&out.out.address)+1)){//перевірка адреси 
    switch(buf.modbuf[1]){				//function code
    case 3:                     //читання одного або більне регістрів (всі регістри розміщені у зовнішній RAM)
      if(buf.read_req.quantity > 125){
        buf.exception.exeption = 3; //забагато регістрів
        error();
        return;
      }
      /* обмежитись зовнішньою памятю в перших 256 байтів 128 16-бітвих регістрів */
      if(buf.read_req.start_address>127 || buf.read_req.start_address*2 + buf.read_req.quantity*2-1 > 255){
        buf.exception.exeption = 2; //помилка адреси або діапазону
        error();
        return;
      }
      /* підготовка відповіді */
      u0.tact = 0;
      u0.th[0].count = 3;                         //заголовок
      u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
      u0.th[1].count = buf.read_resp.byte_count = buf.read_req.quantity * 2; //тіло
      u0.th[1].ptr = buf.read_req.start_address*2;
      break;
      
    case 6:                     //запис у регістр
      /* перевірка адреси */
      buf.write_req.register_address *= 2;//перетворити у внутрішню 8-бітну адресацію
      for(i=0;i<sizeof(WAV)/sizeof(w_a_v);i++){
        if(buf.write_req.register_address == WAV[i].address){
          if(buf.write_req.register_value >= WAV[i].min && buf.write_req.register_value <= WAV[i].max){
            
            /* запис в регістр */
            *((int xdata*)buf.write_req.register_address) = buf.write_req.register_value;
            /* читання регістра */
            buf.write_resp.register_value = *((int xdata*)buf.write_req.register_address);                            
            
            /* встановити ознаку запису в регістр, щоб в фоновій програмі перезаписати зміни у флеш */
            //перевірка чи треба писати у флеш
            if((buf.write_req.register_address >= (int)&out.out.address)&&
            (buf.write_req.register_address <= (int)&out.out.idac)){
                //тайми то перезаписати флеш
                renewFlash = 1;
            }
            
            buf.write_req.register_address = buf.write_req.register_address>>1;//перетворити назад у модбасовську 16-бітну адресацію
            /* підготовка відповіді */
            u0.tact = 0;
            u0.th[0].count = 4;                         //заголовок
            u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
            u0.th[1].count = 2; //тіло
            u0.th[1].ptr = (char pdata*)&buf.write_resp.register_value;
            break;
          }
          else{
            buf.exception.exeption = 3; //помилка діапазону
            error();
            return;
          }
        }
      }
      if(i == sizeof(WAV)/sizeof(w_a_v)){
        /*неправильна адреса*/
        buf.exception.exeption = 2;
        error();
        return;                
      }
      break;
      
    case 16:
      /* перевірка адреси */
      buf.write_mult_req.start_address *= 2;//перетворити у внутрішню 8-бітну адресацію
      // обмежитись областю 16-бітвих регістрів модбаса
      if(((buf.write_mult_req.start_address - (int)&out)/2 + buf.write_mult_req.quantity > sizeof(out.reg)/sizeof(int)-2) ||//-2 щоб уникнути запису адреси і швидкості
         (buf.write_mult_req.quantity*2 != buf.write_mult_req.byte_count)){
           buf.exception.exeption = 2; //помилка адреси або діапазону
           error();
           return;
         }
      offset = (buf.write_mult_req.start_address - (int)&out)/2;
      //ПЕРЕВІРКА діапазону значень
      for(i=0;i<buf.write_mult_req.quantity;i++){
        if(buf.write_mult_req.reg[i] < WAV[offset + i].min || buf.write_mult_req.reg[i] > WAV[offset + i].max){
          buf.exception.exeption = 3; //помилка діапазону
          error();
          return;
        }
      }
      
      //копіювання
      for(i=0;i<buf.write_mult_req.quantity;i++){
        out.reg[offset + i] = buf.write_mult_req.reg[i];
      }
      //перевірка чи треба писати у флеш
      if((buf.write_mult_req.start_address >= (int)&out.out.address)&&
         (buf.write_mult_req.start_address <= (int)&out.out.idac)){
           //тайми то перезаписати флеш
           renewFlash = 1;
         }
      
      buf.write_mult_resp.start_address = buf.write_mult_req.start_address>>1;//перетворити назад у модбасовську 16-бітну адресацію
      /* підготовка відповіді */
      u0.tact = 0;
      u0.th[0].count = 4;                         //заголовок
      u0.th[0].ptr = (unsigned char pdata *)buf.modbuf;
      u0.th[1].count = 2; //тіло
      u0.th[1].ptr = (char pdata*)&buf.write_mult_resp.quantity;				
      break;
      
      
      
    default:
      /* функція не підтримується */
      buf.exception.exeption = 1;
      error();
      break;
    }
    
    flag_time35 = 1;
    time = br[(char)out.out.baud].time35;			//зарядити таймер на 3,5символа
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
  
  u0.tact = 1;        //так як в пакeті один структурний елемет
  u0.th[1].count = 3; //адреса функція код помилки
  u0.th[1].ptr = (unsigned char pdata *)&buf.modbuf;
  
  mb.crcHi=0xff; 
  mb.crcLo=0xff;
  mb.counter = 0;
  
  flag_time35 = 1;
  time = br[(char)out.out.baud].time35;			//зарядити таймер на 3,5символа
  TH0 = time>>8; TL0 = time; 
  TR0 = 1;    
}