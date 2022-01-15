/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
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
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


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

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

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
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
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
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {

  return (~(x & y)) & (~((~x) & (~y)));
  // 摩尔根定理：~(~x & ~y) == x | y

}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {

  return 1<<31; 
  // TMin32 <==> 符号位为1，其余为0

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {

  return !(x + 1 + x + 1) & !!(x+1);
  // INT_MIN = -INT_MAX-1 <==> INT_MIN + INT_MAX + 1 = 0
  // x+1是针对x=-1的情况

}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {

  int temp = 0xAA;
  temp = (temp << 8) + 0xAA;
  temp = (temp << 8) + 0xAA;
  temp = (temp << 8) + 0xAA;
  return !((x & temp) ^ temp);
  // A是一个奇数位都是1，偶数位都是0的数字（位从0记起）
  // 因此只需将x与0xAAAAAAAA取交集即可判断是否奇数位全为1
  // 由于数字大小的限制，取temp=0xAA，通过三次移位得到0xAAAAAAAA
  // 若x奇数位全为1，则x&temp=0xAAAAAAAA=temp，取异或为0，取反为1

}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {

  return (~x) + 1;
  // 补码加法: x + (~x) + 1 = 0 -> -x = (~x) + 1
  // 补码加法溢出的位会被截断

}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {

  // x30 <= x <= 0x39 把前四位和后四位分开处理
  // 前四位判断 x>>4 == 3 后四位判断 0 <= x <= 9

  int a = !((x >> 4) ^ 0x3); // 判断前四位是否等于3
  int b = x & 0xf; // 提取x的后四位
  int c = ~0xA + 1; // ～OxA+1 = 0xFFFFFFF6(0110)
  // int d = 0x80 << 8; // 0x8000 ? 
  int e = !!((b+c)&0x80); // 若x的后四位大于9,则b+c必会溢出且等于0,则(b+c)&d为0,e=0,否则为1
  return a & e;
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {

  // x != 0 时返回y, x == 0 时返回x
  // 即x!=0时,表达式中含z的部分为0,即z+(~z)+1=0, 而x==0时,有y+(~y)+1=0
  // 从而需要使x&(~y+1)表达式中x在x!=0时为0x0,在x==0时为0xFFFFFFFF,对于z同理
  
  // return y + z + ((~(!x) + 1) & ((~y) + 1)) + ((~(!!x) + 1) & ((~z) + 1)) ;

  int a = !!(x^0x0); // x==0 -> a=0  x!=0 -> a=1
  int b = ~a+1; // x==0 -> b=0  x!=0 -> b=0xFFFFFFFF
  int c = ~(y&~b)+1; // x==0 -> ~b=0 -> ~(y&~b)=0xFFFFFFFF -> +1=0
  int d = ~(z&b)+1; // x==0 -> b=0xFFFFFFFF -> ~(z&b)=~z -> +1=~z+1
  return y+z+c+d;
  
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {

  int a = x>>31&0x1; // 获取x的符号位
  int b = y>>31&0x1; // 获取y的符号位
  int c1 = (a&~b); // x<0,y>0时,c1=1
  int c2 = (~a&b); // x>0,y<0时,c2=1
  int c = y + (~x) + 1; // y-x
  int flag = c>>31; // y-x的符号位
  // 需要考虑极端情况下的溢出情况
  // 当x<0,y>0(INT_MAX)时,y+(-x)<0,此时c1=1,c2=0,flag=1,满足x<=y
  // 当x>0,y<0(INT_MIN)时,y+(-x)>0,此时c1=0,c2=1,flag=0,不满足x<=y
  return c1 | (!c2 & !flag) ;

}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {

  // 要点在于如何判断x!=0
  // x=0->～x+1=0  x!=0 -> ~x+1!=0
  // x | ~x+1 -> x=0时为0x00000000  x!=0时为0xffffffff
  // >>31 -> x=0时为0  x!=0时为-1
  return ((x | (~x+1)) >> 31) + 1;

}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {

  // 思路:从右往左找到最左边1的位置,加上一位符号位即可
  // 1.获取x符号位判断x>=0或x<0(x为负不方便右移计算)
  // 2.若x>=0则保持不变,x<0则按位取反
  // 3.判断高16位是否为0(x>>16),若不为0则b16=16(至少有17位)
  // 4.高16位不为0,则x右移16位,左边补0,x>>=b16
  // 5.判断高8位是否为0(x>>8)......以此类推,最后全部相加再加1(符号位)
  
  int b16, b8, b4, b2, b1, b0;
  int flag = x>>31;
  x = (~x&flag)^(~flag&x);
  b16 = !!(x>>16) << 4;
  x >>= b16;

  b8 = !!(x>>8) << 3;
  x >>= b8;

  b4 = !!(x>>4) << 2;
  x >>= b4;

  b2 = !!(x>>2) << 1;
  x >>= b2;

  b1 = !!(x>>1);
  x >>= b1;
  b0 = x;

  return b16+b8+b4+b2+b1+b0+1;

}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {

  int exp = (uf&0x7f800000) >> 23;
  int sign = uf>>31&0x1;
  int frac = uf&0x7fffff;
  int result;

  if(!(exp ^ 0xff)){ // exp域全为1,frac为0时是无穷大,frac!=0时是NaN,均返回argument

    result = uf;
    
  } else if(!exp){ // exp域全为0,非规格化值

    frac <<= 1;
    result = sign<<31 | exp<<23 | frac;

  } else { // exp不全为0且不全为1,规格化值
    
    exp++;
    result = sign<<31 | exp<<23 | frac;

  }
  return result;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  return 2;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
    return 2;
}
