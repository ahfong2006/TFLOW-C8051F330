#include <c8051F330.h>
#include <intrins.h>
#include "define.h"
#include "type.h"
#include "modbus.h"
#include "timer_2.h"
#include "flash.h"
#include "F330_FlashPrimitives.h"
#include "F330_FlashUtils.h"
#include "ADC.h"

/******************************************************************************/
//службові лінії для регістрів В/В
sbit    latch=P1^3;
//sbit    r_t=P0^3; //прийом передача
sbit    miso=P1^1;

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
void Init_Device(void);
void in_out_init(void);
/******************************************************************************/
void main()
{
  
  Init_Device();  
  modbus_init();
  
  for(;;){        
    
    resistence ();
    temperature ();
    DHT11 ();

    PCA0CPH2=0xFF;//reset watchdog
   
    //flash workout
    if(renewFlash){
      renewFlash = 0;        
      //якщо address або baudrate чи тайми то перезаписати флеш
      PCA0MD &= ~(1<<6);//disable WD
      FLASH_Update (0x1a00, (char xdata*)&out.out.address, 10);
      PCA0CPH2=0xFF;//reset watchdog
      PCA0MD |= 1<<6;//enable WD
      //read back from flash
      out.out.address = *((int code*)0x1a00);
      out.out.baud = (*((unsigned int code*)0x1a02)<=4)? *((unsigned int code*)0x1a02) : 0;
      out.out.corr = *((int code*)0x1a04);
      out.out.ref = *((int code*)0x1a06);
      out.out.idac = *((int code*)0x1a08);  
      
      /* перевірка зміни швикості обміну */
      if(buf.write_req.register_address == (int)&out.out.baud){
        TMOD = br[(char)out.out.baud].tmod;
        CKCON = br[(char)out.out.baud].ckcon;
        TH1 = br[(char)out.out.baud].th1;
      }            
    }
  }
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
