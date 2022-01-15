/***************************************************************************
 * Dr. Evil's Insidious Bomb, Version 1.1
 * Copyright 2011, Dr. Evil Incorporated. All rights reserved.
 *
 * LICENSE:
 *
 * Dr. Evil Incorporated (the PERPETRATOR) hereby grants you (the
 * VICTIM) explicit permission to use this bomb (the BOMB).  This is a
 * time limited license, which expires on the death of the VICTIM.
 * The PERPETRATOR takes no responsibility for damage, frustration,
 * insanity, bug-eyes, carpal-tunnel syndrome, loss of sleep, or other
 * harm to the VICTIM.  Unless the PERPETRATOR wants to take credit,
 * that is.  The VICTIM may not distribute this bomb source code to
 * any enemies of the PERPETRATOR.  No VICTIM may debug,
 * reverse-engineer, run "strings" on, decompile, decrypt, or use any
 * other technique to gain knowledge of and defuse the BOMB.  BOMB
 * proof clothing may not be worn when handling this program.  The
 * PERPETRATOR will not apologize for the PERPETRATOR's poor sense of
 * humor.  This license is null and void where the BOMB is prohibited
 * by law.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
// #include "support.h"
// #include "phases.h"

/* 
 * Note to self: Remember to erase this file so my victims will have no
 * idea what is going on, and so they will all blow up in a
 * spectaculary fiendish explosion. -- Dr. Evil 
 */

FILE *infile;

int main(int argc, char *argv[])
{
    char *input;

    /* Note to self: remember to port this bomb to Windows and put a 
     * fantastic GUI on it. */

    /* When run with no arguments, the bomb reads its input lines 
     * from standard input. */
    if (argc == 1) {  
	infile = stdin;
    } 

    /* When run with one argument <file>, the bomb reads from <file> 
     * until EOF, and then switches to standard input. Thus, as you 
     * defuse each phase, you can add its defusing string to <file> and
     * avoid having to retype it. */
    else if (argc == 2) {
	if (!(infile = fopen(argv[1], "r"))) {
	    printf("%s: Error: Couldn't open %s\n", argv[0], argv[1]);
	    exit(8);
	}
    }

    /* You can't call the bomb with more than 1 command line argument. */
    else {
	printf("Usage: %s [<input_file>]\n", argv[0]);
	exit(8);
    }

    /* Do all sorts of secret stuff that makes the bomb harder to defuse. */
    initialize_bomb();

    printf("Welcome to my fiendish little bomb. You have 6 phases with\n");
    printf("which to blow yourself up. Have a nice day!\n");

    /* Hmm...  Six phases must be more secure than one phase! */
    input = read_line();             /* Get input                   */
    phase_1(input);                  /* Run the phase               */
    phase_defused();                 /* Drat!  They figured it out!
				      * Let me know how they did it. */
    printf("Phase 1 defused. How about the next one?\n");

    /**
     * phase 1:
     * print (char *) 0x402400
     * Answer -> Border relations with Canada have never been better.
    */

    /* The second phase is harder.  No one will ever figure out
     * how to defuse this... */
    input = read_line();
    phase_2(input);
    phase_defused();
    printf("That's number 2.  Keep going!\n");

    /**
     * phase 2:
     * func read_six_numbers: 读取输入字符串,调用sscanf()函数匹配字符串中的数字,匹配得到的数字存放在int数组中,数组长度大于5则返回true
     * 从第二位数字开始for循环遍历数组,如果是前一位数字的2倍,则继续循环,否则boom,任务失败
     * 完成for循环则表示任务成功,即输入字符串中的前六位数字是从1开始,2为公比的等比数列(输入字符串中不得有除数字外的其它字符)
     * Answer -> 1 2 4 8 16 32
     */

    /* I guess this is too easy so far.  Some more complex code will
     * confuse people. */
    input = read_line();
    phase_3(input);
    phase_defused();
    printf("Halfway there!\n");

    /**
     * phase 3:
     * mov $0x4025cf,%esi -> sscanf()函数参数format为"%d %d",str为输入字符串,返回匹配成功数字个数
     * 0x8(%rsp) -> 读取得到的第一个数字, 0xc(%rsp) -> 读取得到的第二个数字
     * cmpl $0x7,0x8(%rsp) (ja-无符号大于) -> 第一个数字必须小于等于7(无符号数大于等于0)
     * jmpq *0x402470(,%rax,8) 类似于一个switch-case
     * jmpq跳转到一个长度为8代码表,将第一个数字作为索引,跳转到不同的代码块
     * Answer -> 0 - 207, 1 - 311, 2 - 707, 3 - 256, 4 - 389, 5 - 206, 6 - 682, 7 - 327
     */

    /* Oh yeah?  Well, how good is your math?  Try on this saucy problem! */
    input = read_line();
    phase_4(input);
    phase_defused();
    printf("So you got that one.  Try this one.\n");

    /**
     * phase 4:
     * cmpl $0xe,0x8(%rsp) -> 第一个数字必须小于等于14（无符号数大于等于0)
     * Answer -> 第一个数:7, 3, 2, 1  第二个数: 0
     */ 
    
    /* Round and 'round in memory we go, where we stop, the bomb blows! */
    input = read_line();
    phase_5(input);
    phase_defused();
    printf("Good work!  On to the next...\n");

    /**
     * phase 5:
     * 
     */

    /* This phase will never be used, since no one will get past the
     * earlier ones.  But just in case, make this one extra hard. */
    input = read_line();
    phase_6(input);
    phase_defused();

    /* Wow, they got it!  But isn't something... missing?  Perhaps
     * something they overlooked?  Mua ha ha ha ha! */
    
    return 0;
}
