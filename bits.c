/* 
 * CS:APP Data Lab 
 * 
 * <Jiacheng Sun jzs375>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
 int tem1 = ~x;            /* This apply the De Morgan's Law       */
 int tem2 = ~y;            /* A and B equals to not (NOTA or NOTB) */ 
 return ~(tem1|tem2);
}
/* 
 * allEvenBits - return 1 if all even-numbered bits in word set to 1
 *   Examples allEvenBits(0xFFFFFFFE) = 0, allEvenBits(0x55555555) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allEvenBits(int x) {
  int t1 = x & 0x55;        /* Check even-numbered bits by 8 bits once until 32 bits */
  int t2 = (x>>8) & 0x55;   /* have been checked. Combined each checking result into */
  int t3 = (x>>16) & 0x55;  /* 8 bits. If all even numbered, the combination should  */
  int t4 = (x>>24) & 0x55;  /* be 01010101 and should be overflow to 9th bit after adding 0xAB */
  return (((((t1 & t2) & t3) & t4) + 0xAB) >> 8);

}
/* 
 * bitMask - Generate a mask consisting of all 1's 
 *   lowbit and highbit
 *   Examples: bitMask(5,3) = 0x38
 *   Assume 0 <= lowbit <= 31, and 0 <= highbit <= 31
 *   If lowbit > highbit, then mask should be all 0's
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int bitMask(int highbit, int lowbit) {
  int t = ~ (0x0) ;                               /* Move the highbit to the right position and reverse it to */
  return ~((t << highbit) << 1) & (t << lowbit);  /* make outside-mask space opposite to lowbit's outside space */

}
/* 
 * replaceByte(x,n,c) - Replace byte n in x with c
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: replaceByte(0x12345678,1,0xab) = 0x1234ab78
 *   You can assume 0 <= n <= 3 and 0 <= c <= 255
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 3
 */
int replaceByte(int x, int n, int c) {
  int twobyte = 0xff;                      /* Create a 2 byte mask, and convert position to bits number */
  int shift = n << 3;                      /* shift the mask to right position and then put the replacement */
  twobyte = ~(twobyte << shift);           /* number on it */
  c = c << shift;
  return (x & twobyte)| c;
}
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5) = 0, bitParity(7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int bitParity(int x) {
  x = x ^ (x >> 16);  /* If 0's odd, 1's odd. fold the list and if 2 1's meet, they are cancelled to 0 */
  x = x ^ (x >> 8 );  /* repeat the process until only 1 element left. if 1 left it's odd, otherwise even */
  x = x ^ (x >> 4);
  x = x ^ (x >> 2);
  x = x ^ (x >> 1);
  return x&1;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1 << 31;  /* 0x80000000 */
}
/* 
 * isNegative - return 1 if x < 0, return 0 otherwise 
 *   Example: isNegative(-1) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNegative(int x) {
  return (x >> 31) & 1;  /* check the sign bit. 1 negtive, 0 positive. */
}
/* 
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int addOK(int x, int y) {
  int sumSign = ((x + y) >> 31) & 1;      /* check sign. Two number with opposite sign will never overflow */
  int xSign = (x >> 31) & 1;              /* the sum overflow when the sum has oppsite sign with the two addend */
  int ySign = (y >> 31) & 1;
  return !((~(xSign ^ ySign)) & (xSign ^ sumSign));
}
/* 
 * absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
int absVal(int x) {
 int signChecker = x >> 31;                          /* No need to change sign if positive, if negative, flip all bits using*/
 int result = (signChecker ^ x) + (signChecker & 1); /* the definition of right shift. all 1 if sign bit is 1, 0 otherwise*/
 return result;
}
/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
  int NaNExp = 0x7f800000;                         /* Overflow when exp are 1's */
  int fracCheck = 0x7fffff;                        /* Otherwise, flip the sign bit*/
  int frac = (fracCheck & uf);
  if (((uf & NaNExp) == NaNExp) && (frac != 0) ){
    return uf;
  }
  else{
    return uf ^ 0x80000000;
  } 
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  //int carry = !((uf & 3) ^ 3);                 /* carry or round is defined be last 2 bit, only 11 should carry */
  int NaNExp = 0x7f800000;                       /* special case: overflow, uf = 0, 0x80000000 (return themselves)*/
  int exp = uf & NaNExp;                         /* exp = 0: only need to right shift, rounding and combine sign  */
  int sign = uf & 0x80000000;                    /* exp = 0x00800000: the exp will go to fraction                 */
  int carry = 1;                                 /* common case: the exp will decrease by one bit, combine the new */
  if ((uf & 3) != 3){carry = 0;}                 /* exp with old sign and fraction*/
  if ((exp == NaNExp)) {return uf;}
  else if(uf == 0) {return 0;}
  else if(uf == 0x80000000) {return 0x80000000;}
  else if(exp == 0) {return  ((((uf & 0x007fffff) >> 1) + carry) | sign);}
  else if(exp == 0x00800000) {return ((((uf & 0x7fffffff)  >> 1) + carry) | sign); }
  else {return (exp - 0x800000) | (uf & 0x807fffff);}
}
