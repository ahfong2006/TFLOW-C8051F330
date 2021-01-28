#include <c8051F330.h>
#include "define.h"
#include "type.h"
#include "modbus.h"
#include "ntc10k3950.h"
#include "sfr16.h"

/******************************************************************************/
unsigned int adc64; //adc accumulator
unsigned char adcN = 64; //counter of adc samples
unsigned int accNew; //adc accumulator copy
bit adcNew; //end of 20ms acquisition cycle
bit reIDAC; //flag for renew IDAC
bit Rnew; // new sample of resistance came
unsigned int DAC16, DAC10; // 16bit and 10bit
/******************************************************************************/
/******************************************************************************/
void ADC_INT (void) interrupt 10 
{  
  adc64 += ADC0;
  if (--adcN == 0) {
    adcNew = 1;
    adcN = 64;
    accNew = adc64; // copy of new mesurment result
    adc64 = 0; // clear accumulator

    if (reIDAC) {
      IDAC0 = DAC16; // in case the renge is change
      reIDAC = 0;
    }   
  }    
  AD0INT = 0;//reset interrupt
}
/******************************************************************************/
//#define REF 2454
//#define Iref 5017
xdata unsigned long Rbuf[32];

char Rplace; // poiter in resistance buffer
/******************************************************************************/
void resistence (void)
{
  unsigned long j,k;
  unsigned int q;
  unsigned int codeADC;
  if (adcNew) {
    adcNew = 0;
    codeADC = accNew >> 6; //reset overprecision
    in.sys.ADC = codeADC;
    j = (unsigned long)codeADC * out.out.ref;//REF; 
    q = j >> 10; // milivoltage
    in.sys.U = q;    
    //--power begin --
    j = (unsigned long)q * out.out.idac;//Iref;
    j = j / 10000;// uW
    in.sys.power = j;//power in uW 
    if (j > 600){
      if (DAC10 < 5) DAC10 = 5; //5<<6 0x140
      j = 600000 / j;
      j = j * DAC10;
      DAC10 = j / 1000;
      if (DAC10 > 1023) DAC10 = 1023;
      if (DAC10 < 5) DAC10 = 5; 
      reIDAC = 1; //request for renew  the DAC
    } else {
      if (j > 500){
        DAC10--;
        reIDAC = 1; //request for renew  the DAC
      } else {
        if ( DAC10 < 0x3ff){//j < 490 && codeADC < 0x380 
          DAC10++;
          reIDAC = 1; //request for renew  the DAC
        }     
      }
    }
    //--power end--
    k = (unsigned long)DAC16 * out.out.idac;//Iref;//old DAC
    k >>= 10; // idac offset
    if ((k & 0x3f) >= 31) k = (k >>6) + 1;
    else k >>= 6;
    //j = (unsigned long)(q) * 10000; //insted of 1000 becose Iref multiplied by 10 of microampere - Iref 0.5 mA 
    j = (unsigned long)accNew * out.out.ref; // overprecision 64
    j >>= 10; // 10 from 16
    j *= 10000;
    j = j / k; // resistance in Ohm 
    if ((j & 0x3f) >= 31) j = (j >>6) + 1;
    else j >>= 6;
    Rplace = Rplace & 0x1F;
    Rbuf [Rplace] = j;
    Rplace++;
    if (out.out.command){
      if (out.out.command > 0x3ff) out.out.command = 0x3ff;
      DAC10 = out.out.command; // command increase speed change of dac
      reIDAC = 1;
      out.out.command = 0;
    }
    DAC16 = DAC10 << 6;
    in.sys.dac = DAC10;
    Rnew = 1; // new resistans came
  }
}
/******************************************************************************/
void temperature (void) 
{
	unsigned char p;
  unsigned long Ac = 0;
  unsigned long R;
  
  if (Rnew){
    Rnew = 0;
    for (p=0;p<32;p++) Ac += Rbuf [p];
    R = Ac >>5;// divide by 32;
    in.sys.R[0] = R / 1000;
    in.sys.R[1] = R % 1000;    
    for ( p=1; p<sizeof(NTC10K3950)/sizeof(NTC); p++ ) {
      if ( R >= NTC10K3950[p].R ) {
        Ac = R - NTC10K3950[p].R; // delta resistance
        Ac *= 50; //multiply by 5.0 degrees celsius step of table
        Ac /= NTC10K3950[p-1].R - NTC10K3950[p].R; // divide by range of resistence 
        in.sys.T = NTC10K3950[p].t*10 - Ac; // temperature offset
        break;
      }
    }
  }
}
/******************************************************************************/




