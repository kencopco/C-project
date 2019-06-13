//Name: Jiacheng Sun
//Class: Cmpsc311
//Instructor: Yanling Wang
#include "float_i2f.h"
#include <stdio.h>
float_bits float_i2f(int i) {
  unsigned int sign = i >> 31;
  unsigned int exp;
  unsigned int overf = 0x00FFFFFF;
  unsigned int frac = i;
  //special case handling
  if (i == 0x80000000){
    exp = 0x9E;
    frac = 0x0;
  } 
  //negative number transformation 
  else{
    if (sign == 0xFFFFFFFF){
      i = ~i+1;
    }  
    int j;
    int checkDig;
    //loop for check the first "1" in positive number
    for(j = 31;j > -1; j--){
      int leadingCheck;
      leadingCheck = i >> j;
      if (leadingCheck == 1){
        checkDig = j;
        break;
      }
    }
    // "0" case
    if (j == -1){
      exp = 0;
      frac = 0;
    }
    //common cases
    else {
      exp = checkDig + 127;
      unsigned int fracMove = 31 - checkDig;	//the digits that used to move 24-bit fraction to left most.
      unsigned int lastEight = (i << ((31 - (checkDig - 24))) >> 24) & 0x000000FF;	//residue on right most.
      unsigned int fracWOR = ((i << (fracMove)) >> 8) & 0x00FFFFFF;	//24-bit fraction without rounding on right most.
      unsigned int fracEnd = fracWOR << 31;	//the last digit of the fraction on right most.
      //check the situation that input integer is more than 23 bits.
      if (checkDig > 23){
        //residue is on "half" situation.
        if (lastEight == 0x80){
          //check the last digit of fraction.
          if (fracEnd == 0x80000000){
            //handle the overflow condition.
            if (fracWOR == overf){
              exp = exp + 1;
              frac = 0;
            }
            //rounding up
            else {frac = fracWOR + 1 ;}
          }
          //rounding down
          else {frac = fracWOR;}
        }
        //residue is more than "half".
        else if (lastEight > 0x80){
          //handling overflow.
          if (fracWOR == overf){
            exp = exp + 1;
            frac = 0;
          }
          //rounding up
          else {frac = fracWOR + 1;}
        } 
        //rounding down condition with "less than half"
        else {frac = fracWOR;}  
      }
      //less than 23 bits condition.
      else{ 
        frac = fracWOR;
      }
    }  
  }   
 return (sign << 31) | (exp << 23) | (frac & 0x7FFFFF);
}
