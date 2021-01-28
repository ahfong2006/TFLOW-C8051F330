#include <c8051F330.h>
#include "define.h"
#include "type.h"
#include "modbus.h"
#include "main.h"


bit readDHT;
unsigned char twoSec;
unsigned char qSec;
/******************************************************************************/
void PCA_isr() interrupt 11 using 2
{
  unsigned int pTime;
  
  if(CCF0){
    CCF0 = 0;
    pTime = PCA0L;
    pTime |= PCA0H<<8;
    pTime += 0x4FC0; //10mS    
    PCA0CPL0 = pTime;
    PCA0CPH0 = pTime>>8;
    if(++twoSec == 200){ 
      twoSec = 0;
      readDHT = 1;
    }
    if (!VD5){
      if ( ++qSec == 15) VD5 = 1; //LED GOOG REED DHT11
    } else qSec = 0;
      
  }
  
  if(CCF1){
    CCF1 =0;
  } 
}
/******************************************************************************/