
//
//  compile.h
//  PL0
//
//  Created by Cardinal on 2018/11/15.
//  Copyright © 2018年 Cardinal. All rights reserved.
//

#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/* 符号 */
enum symbol {
    nul,         ident,     number,     plus,      minus,
    times,       slash,     eql,        neq,       lss,
    leq,         gtr,       geq,        lparen,    rparen,
    semicolon,   period,    becomes,    ifsym,     elsesym,
    whilesym,    writesym,  readsym,    range,     callsym,
    intsym,      funcsym,   forsym,     insym,     lbrace,
    rbrace,      mod,       add,        sub,       constsym,
    oddsym,      repeatsym, charsym,
};


/* 符号表中的类型 */
enum object {
    constant,
    variable,
    function,
};

/* 虚拟机代码指令 */
enum fct {
    lit,     opr,     lod,
    sto,     cal,     ini,
    jmp,     jpc,     jeq,
};

void compile();
void error(int n);
void getsym();
void getch();
void init();
void gen(enum fct x, int y, int z);
void test(bool* s1, bool* s2, int n);
int inset(int e, bool* s);
void addset(bool* sr, bool* s1, bool* s2, int n);
void subset(bool* sr, bool* s1, bool* s2, int n);
void mulset(bool* sr, bool* s1, bool* s2, int n);
void block(int lev, bool* fsys);
void interpret();
void factor(int lev, bool* fsys);
void term(int lev, bool* fsys);
void condition(int lev, bool* fsys);
void expression(int lev, bool* fsys);
void statement(int lev, bool* fsys);
void statement_item(int lev, bool* fsys);
void statement_single_item(int lev, bool* fsys);
void listall();
int position();
void enter(enum object k, int lev, int* pdx);
int base(int l, int* s, int b);
void vardeclaration(int lev, int* pdx, symbol last_sym);
#endif

