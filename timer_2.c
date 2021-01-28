#include <c8051F330.h>
#include "define.h"
#include "type.h"
#include "modbus.h"
#include "main.h"
#include "PCA.h"

/******************************************************************************/
sbit DHT = P0^6; //DHT11 Line

bit startD; //initiations of read DHT11
bit Err;
bit ok50; // start next one is ok
unsigned char stg; //stage of read sensor
unsigned char dhtD[5];
unsigned char bitN;//bit counter
unsigned char byteN;//byte counter
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
//timer
void Timer2 (void) interrupt 5 using 1
{
  TF2H = 0;
  TR2 = 0;//TMR2CN &= 0x04; disable timer 2  
  
  if (startD) {
    startD = 0; //reset
    stg = 0;
    DHT = 1;// 20mS push down end
    TMR2H = 0; TMR2L = 0;
    EX0 = 1; //waiting for falling edge
    TR2 = 1;//run timer
  }   

}
/******************************************************************************/
void int0_isr() interrupt 0 using 2 //falling edge int
{
  bit Bt;  
  switch (stg){
    default:
      if (TMR2H ==0 && TMR2L < 5) break; //prohibit small time NOT NECECERY  
      TR2 = 0; //timer stop
      if ( ok50){
        ok50 = 0;
        if (TMR2H ==0){ 
          if (TMR2L > 45){ //>22uS 
            if ( TMR2L < 61) Bt = 0;// <30us
            else if ( TMR2L < 153) Bt = 1;// <75us
            if (Bt) dhtD[byteN] |= (unsigned char)Bt << bitN;
            if( --bitN == 0xff){ 
              bitN = 7;
              if ( --byteN == 0xff) stg = 4;//end
            }
            TMR2H = 0; TMR2L = 0;
            TR2 = 1;//run timer
          }else Err = 1;
        }else Err = 1;        
      }
      break;
    case 0:
      if (TMR2H ==0 && TMR2L < 5) break; //prohibit small time  
      TR2 = 0; //timer stop
      if (TMR2H ==0 && TMR2L > 31 && TMR2L < 92){//>15uS  <45uS
        //EX0 = 0;//disable falling
        stg = 1;
        TMR2H = 0; TMR2L = 0;
        EX1 = 1; //waiting for rising edge
        TR2 = 1;//run timer
      } else Err = 1;
      break;
    case 2:
      if (TMR2H ==0 && TMR2L < 5) break; //prohibit small time NOT NECECERY
      TR2 = 0; //timer stop
      if (TMR2H ==0 && TMR2L > 143 && TMR2L < 184){//>70uS  <90uS
        stg = 3;
        ok50 = 0;
        TMR2H = 0; TMR2L = 0;
        TR2 = 1;//run timer
      } else Err = 1;      
      break;  
    case 4:
      break;  
  }
}
/******************************************************************************/
void int1_isr() interrupt 2 using 2 //rasing edge int
{
  switch (stg){
    default: 
      if (TMR2H ==0 && TMR2L < 5) break; //prohibit small time NOT NECECERY
      TR2 = 0; //timer stop
      if (TMR2H ==0 &&  TMR2L < 112){//  <55uS
        ok50 = 1;
        TMR2H = 0; TMR2L = 0;
        if (stg == 4) {stg = 5; break;}//final stop bit
      } else Err = 1;
      TR2 = 1;//run timer    
      break;
    case 1:
    if (TMR2H ==0 && TMR2L < 5) break; //prohibit small time  
    TR2 = 0; //timer stop
      if (TMR2H ==0 && TMR2L > 143 && TMR2L < 184){//>70uS  <90uS
        stg = 2;
        TMR2H = 0; TMR2L = 0;
        TR2 = 1;//run timer
      } else Err = 1;      
      break;
  }
}
/******************************************************************************/
// timer 3 used for ADC 
void timer3 (void) interrupt 14 using 3
{
  //TMR3CN    = 0x0;//reset TF3H and stoping timer 
  //TMR3CN    = 0x04; //reset TF3H
  
}
/******************************************************************************/
/******************************************************************************/
void DHT11 (void)
{
  unsigned char i;
  unsigned char cs;
  
  if (readDHT){
    readDHT = 0;
    for (i=0;i<5;i++) dhtD[i] = 0;
    TMR2H = 0x60;
    TMR2L = 0x7E;//20mS
    Err = 0;
    bitN = 7; // high bit first
    byteN = 4;// higest 
    startD = 1;
    EX0 = 0;//disable ex.int0 falling edge
    EX1 = 0;//disable ex.int1 rising edge
    DHT = 0; //begin request to sensor drope line down
    TR2 = 1;//TMR2CN = 0x04; enable timer 2
  }
  
  if (stg == 5){
    stg = 0;
    //end of transmition
    cs = 0;
    for(i=4;i>0;i--){
      cs += dhtD[i];
    }
    if (cs == dhtD[0]){
      // check summ is right
      in.sys.rH = dhtD[4];
      //in.sys.rH |= dhtD[3];
      in.sys.tC = dhtD[2]*10+dhtD[1];
      VD5 = 0; //LED GOOG REED DHT11
    }
    
  }
   
}  
/******************************************************************************/
/******************************************************************************/



