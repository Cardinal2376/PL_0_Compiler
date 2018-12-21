//
//  my.cpp
//  PL0
//
//  Created by Cardinal on 2018/11/15.
//  Copyright © 2018年 Cardinal. All rights reserved.
//

#include "compile.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cctype>
#include <vector>
#include <map>


#define norw 13       /* 保留字个数 */
#define nmax 14       /* 数字的最大位数 */
#define addrmax 2048  /* 地址上界*/
#define levmax 3      /* 最大允许过程嵌套声明层数*/
#define cxmax 200     /* 最多的虚拟机代码数 */
#define stacksize 500 /* 运行时数据栈元素最多为500个 */
#define symnum 60
#define fctnum 9

const int max_length = 10;  /* max length of ident */
const int table_max = 100;  /* capacity of the symbol table */
const int maxerr = 30;
char ch;                    /* used by function "getch", to read and store a character */
enum symbol sym;            /* current symbol */
int current_char, line_length;  /* used by function "getch" */
int linenum;
int num;                    /* current number value */
char line[81];              /* line buffer for reading */
int cx;                     /* pointer for the virtual machine */

std::string current_ident;
std::map<std::string, symbol> wsym; /* the sym of key word */
std::map<int, symbol> ssym;         /* the sym of single char */

/* used by recover */
char rec_ch;
enum symbol rec_sym;
int rec_current_char, rec_line_length;
int rec_linenum;
int rec_num;
char rec_line[81];
int rec_cx;
std::string rec_current_ident;


/* Table Structure */
struct tableStruct {
    std::string name;
    enum object kind;       /* const, variable or procedure */
    int val;                /* only for const */
    int level;
    int addr;
    int size;
    int type;               /* int or char */
};

std::vector<tableStruct> table;
/* 虚拟机代码结构 */
struct instruction {
    enum fct f; /* 虚拟机代码指令 */
    int l;      /* 引用层与声明层的层次差 */
    int a;      /* 根据f的不同而不同 */
};
char mnemonic[fctnum][5];   /* 虚拟机代码指令名称 */

struct instruction code[cxmax]; /* 存放虚拟机代码的数组 */


bool declbegsys[symnum];    /* 表示声明开始的符号集合 */
bool statbegsys[symnum];    /* 表示语句开始的符号集合 */
bool facbegsys[symnum];     /* 表示因子开始的符号集合 */
bool termsys[symnum];     /* 表示term开始的符号集合 */

FILE* fin;      /* 输入源文件 */ 
FILE* ftable;
FILE* fdebug;
FILE* ferrors;
FILE* fresult;

/* On Developing */
std::vector<std::string> symbol_word {
    "nul",      "ident",    "number",   "plus",     "minus",
    "times",    "slash",    "eql",      "neq",      "lss",
    "leq",      "gtr",      "geq",      "lparen",   "rparen",
    "semicolon","period",   "becomes",  "ifsym",    "elsesym",
    "whilesym", "writesym", "readsym",  "range",    "callsym",
    "intsym",   "funcsym",  "forsym",   "insym",    "lbrace",
    "rbrace",   "mod",      "add",      "sub",      "constsym",
    "oddsym",   "repeatsym", "charsym", "lbracket", "rbracket",
    "arrsym",   "xorsym"
};
int arr_num;
int err;        /* ERROR Counter */
char errorinfo[100][100];


void init() {
    /* 设置单字符符号 */
    ssym.clear();
    ssym['%'] = mod;
    ssym['+'] = plus;
    ssym['-'] = minus;
    ssym['*'] = times;
    ssym['/'] = slash;
    ssym['('] = lparen;
    ssym[')'] = rparen;
    ssym['{'] = lbrace;
    ssym['}'] = rbrace;
    ssym['='] = becomes;
    ssym['$'] = period;
    ssym[';'] = semicolon;
    ssym['['] = lbracket;
    ssym[']'] = rbracket;
    
    /* initialize the key word symbol map */
    wsym["call"] = callsym;
    wsym["const"] = constsym;
    wsym["else"] = elsesym;
    wsym["for"] = forsym;
    wsym["func"] = funcsym;
    wsym["if"] = ifsym;
    wsym["in"] = insym;
    wsym["odd"] = oddsym;
    wsym["xor"] = xorsym;
    wsym["write"] = writesym;
    wsym["read"] = readsym;
    wsym["repeat"] = repeatsym;
    wsym["int"] = intsym;
    wsym["while"] = whilesym;
    wsym["char"] = charsym;
    
    /* 设置指令名称 */
    strcpy(mnemonic[lit], "lit");
    strcpy(mnemonic[opr], "opr");
    strcpy(mnemonic[lod], "lod");
    strcpy(mnemonic[sto], "sto");
    strcpy(mnemonic[cal], "cal");
    strcpy(mnemonic[ini], "int");
    strcpy(mnemonic[jmp], "jmp");
    strcpy(mnemonic[jpc], "jpc");
    strcpy(mnemonic[jeq], "jeq");
    strcpy(mnemonic[stoa], "stoa");
    strcpy(mnemonic[loda], "loda");
    strcpy(mnemonic[cpy], "cpy");
    
    /* 设置符号集 */
    memset(declbegsys, false, sizeof declbegsys);
    memset(statbegsys, false, sizeof statbegsys);
    memset(facbegsys, false, sizeof facbegsys);
    memset(termsys, false, sizeof facbegsys);
    
    /* 设置声明开始符号集 */
    declbegsys[intsym] = true;
    declbegsys[charsym] = true;
    
    /* 设置语句开始符号集 */
    statbegsys[callsym] = true;
    statbegsys[ifsym] = true;
    statbegsys[whilesym] = true;
    statbegsys[readsym] = true;
    statbegsys[writesym] = true;
    statbegsys[ident] = true;
    statbegsys[arrsym] = true;
    statbegsys[forsym] = true;
    statbegsys[repeatsym] = true;
    
    /* 设置因子开始符号集 */
    facbegsys[ident] = true;
    facbegsys[arrsym] = true;
    facbegsys[number] = true;
    facbegsys[lparen] = true;

    /* 设置term开始的符号集合 */
    termsys[mod] = true;
    termsys[slash] = true;
    termsys[xorsym] = true;
    termsys[times] = true;

    strcpy(errorinfo[0], "声明符号后面缺少标识符");
    strcpy(errorinfo[1], "缺少';'");
    strcpy(errorinfo[2], "语句或者函数声明开始符号错误");
    strcpy(errorinfo[3], "缺少语句开始符号");
    strcpy(errorinfo[4], "缺少语句结束符号");
    strcpy(errorinfo[5], "编译未完成");
    strcpy(errorinfo[6], "标识符未声明");
    strcpy(errorinfo[7], "赋值语句等号左侧不是变量");
    strcpy(errorinfo[8], "缺少赋值符号");
    strcpy(errorinfo[9], "缺少分号");
    strcpy(errorinfo[10], "语句结束符号错误");
    strcpy(errorinfo[11], "缺少关系运算符");
    strcpy(errorinfo[12], "标识符不能是函数");
    strcpy(errorinfo[13], "因子开始符号错误");
    strcpy(errorinfo[14], "数字位数超出范围");
    strcpy(errorinfo[15], "数字大小超出范围");
    strcpy(errorinfo[16], "缺少')'");
    strcpy(errorinfo[17], "缺少'('");
    strcpy(errorinfo[18], "变量未声明");
    strcpy(errorinfo[19], "function后面缺少'()'");
    strcpy(errorinfo[20], "缺少'{'");
    strcpy(errorinfo[21], "缺少'}'");
    strcpy(errorinfo[22], "缺少'in'");
    strcpy(errorinfo[23], "for循环缺少左边界");
    strcpy(errorinfo[24], "for循环缺少右边界");
    strcpy(errorinfo[25], "for循环缺少'...'");
    strcpy(errorinfo[30], "函数嵌套层数过多");
    strcpy(errorinfo[31], "call声明后面缺少过程名");
    strcpy(errorinfo[32], "常量声明中的=后应是数字");
    strcpy(errorinfo[33], "repeat后缺少while");
    strcpy(errorinfo[34], "不能识别的符号");
    strcpy(errorinfo[35], "缺少']'");
    strcpy(errorinfo[36], "定义出错");
    strcpy(errorinfo[37], "数组下标越界");
    strcpy(errorinfo[38], "数组定义出错");
    strcpy(errorinfo[39], "数组大小超过限制");
    strcpy(errorinfo[40], "缺少'['");

    err = 0;
    current_char = line_length = 0;
    ch = ' ';
    linenum = 0;
    table.clear();
}


void compile() {
    //fin = fopen("C:\\Users\\10152130143\\Desktop\\sw_compiler-master\\sw_lcm.txt", "r");
    //fin = stdin;
    /*
    fin = fopen("/Users/cardinal/PL0_Compiler/test_lcm.txt", "r");
    ftable = fopen("/Users/cardinal/PL0_Compiler/ftable.txt", "w");
    fdebug = fopen("/Users/cardinal/PL0_Compiler/fdebug.txt", "w");
    ferrors = fopen("/Users/cardinal/PL0_Compiler/ferrors.txt", "w");
    fresult = fopen("/Users/cardinal/PL0_Compiler/fresult.txt", "w");
    */
    
    fin = fopen("test_lcm.txt", "r");
    ftable = fopen("ftable.txt", "w");
    fdebug = fopen("fdebug.txt", "w");
    ferrors = fopen("ferrors.txt", "w");
    fresult = fopen("fresult.txt", "w");
    
    init();        /* 初始化 */
    
    bool nxtlev[symnum];
    addset(nxtlev, declbegsys, statbegsys, symnum);
    nxtlev[period] = true;
    
    getsym();
    block(0, nxtlev);
    
    if (sym != period) {
        error(5);
    }
    
    if (err == 0) {
        printf("\n===编译成功!===\n");
    } else {
        printf("\n本代码共有 %d 个错误!\n",err);
    }
    
    listall();
    interpret();
    
    fclose(fin);
    fclose(ftable);
    fclose(fdebug);
    fclose(ferrors);
    fclose(fresult);
}

/*
 * 用数组实现集合的集合运算
 */
int inset(int e, bool* s) {
    return s[e];
}

void addset(bool* sr, bool* s1, bool* s2, int n) {
    for (int i = 0; i < n; i++) {
        sr[i] = s1[i] || s2[i];
    }
}

void subset(bool* sr, bool* s1, bool* s2, int n) {
    for (int i = 0; i < n; i++) {
        sr[i] = s1[i] && (!s2[i]);
    }
}

void mulset(bool* sr, bool* s1, bool* s2, int n) {
    for (int i = 0; i < n; i++) {
        sr[i] = s1[i] && s2[i];
    }
}

/*
 *  出错处理，打印出错位置和错误编码
 */
void error(int n) {
    printf("第 %d 行 错误: %s, 当前sym = %s\n", linenum, errorinfo[n], symbol_word[sym].c_str());
    fprintf(ferrors, "第 %d 行 错误: %s\n", linenum, errorinfo[n]);
    err = err + 1;
    if (err > maxerr) {
        exit(1);
    }
}

/*
 word analysis
 */
void getsym() {
    printf("sym = ");
    std::cout << symbol_word[sym] << std::endl;
    //remove blanks
    while(ch == ' ' || ch == '\n' || ch == '\t') {
        getch();
    }
    if(ch == '$') sym = period;
    else if(isalpha(ch)) {
        //recognize symbol or ident
        std::string ident_in = "";
        //这里长度报错怎么办
        while(isalnum(ch) && ident_in.length() < max_length) {
            ident_in.push_back(ch);
            getch();
        }
        current_ident = ident_in;
        //judge if be the key word
        if(wsym.find(ident_in) != wsym.end()) {
            sym = wsym[ident_in];
        } else {
            sym = ident;
        }
    } else if(isdigit(ch)){
        num = 0;
        int k = 0;
        sym = number;
        do {
            num = 10 * num + ch - '0';
            k++;
            getch();
        } while(isdigit(ch));
        if(k > nmax) {
            error(14);
        }
    } else {
        if(ch == '<') {
            getch();
            if(ch == '=') {
                sym = leq;
                getch();
            } else {
                sym = lss;
            }
        } else if(ch == '>') {
            getch();
            if(ch == '=') {
                sym = geq;
                getch();
            } else {
                sym = gtr;
            }
        } else if(ch == '=') {
            getch();
            if(ch == '=') {
                sym = eql;
                getch();
            } else
                sym = becomes;
        } else if(ch == '!') {
            getch();
            if(ch == '=') {
                sym = neq;
                getch();
            } else
                sym = nul;
        } else if(ch == '/') {
            getch();
            if(ch == '/') {    //行注释
                current_char = line_length;
                getch();
                getsym();
            } else if(ch == '*') { //块注释
                int flag = 0;
                while(ch != '$') {
                    getch();
                    if(ch == '*') flag = 1;
                    else if(ch == '/' && flag == 1) {
                        getch();
                        getsym();
                        break;
                    } else {
                        flag = 0;
                    }
                }
                if(ch == '$') sym = period;
            } else {    //除号
                sym = slash;
                getch();
            }
        } else if(ch == '+') {
            getch();
            if(ch == '+') {
                sym = add;
                getch();
            } else {
                sym = plus;
            }
        } else if(ch == '-') {
            getch();
            if(ch == '-') {
                sym = sub;
                getch();
            } else {
                sym = minus;
            }
        } else {
            //符号开头，不在表里怎么办，应该报一个error
            if(ssym.find(ch) != ssym.end()) {
                sym = ssym[ch];
                if(sym != period) {
                    getch();
                }
            } else {
                error(34);
                sym = nul;
                getch();
            }
        }
    }
}
/*
 to get a single character
 */

void getch() {
    if(current_char == line_length) {
        //if char remain in buffer, o.w. get another char
        //printf("getch()\n");
        ch = getc(fin);
        if(feof(fin)) {
            //printf("feof\n");
            ch = '$';
            return;
        }
        ungetc(ch, fin);
        line_length = 0;
        current_char = 0;
        linenum++;
        ch = 'p';
        while(ch != '\n') {
            if(fscanf(fin, "%c", &ch) == EOF) {
                //printf("not feof:%c %d\n", ch, ch);
                line[line_length] = 0;
                break;
            }
            line[line_length] = ch;
            line_length++;
        }
    }
    ch = line[current_char];
    current_char++;
}

/*
 * 生成虚拟机代码
 *
 * x: instruction.f;
 * y: instruction.l;
 * z: instruction.a;
 */
void gen(enum fct x, int y, int z ) {
    if (cx >= cxmax) {
        printf("生成的虚拟机代码程序过长!\n");  /* 生成的虚拟机代码程序过长 */
        exit(1);
    }
    if ( z >= addrmax) {
        printf("地址偏移越界!\n");    /* 地址偏移越界 */
        exit(1);
    }
    code[cx].f = x;
    code[cx].l = y;
    code[cx].a = z;
    cx++;
}


/*
 * 测试当前符号是否合法
 *
 * 在语法分析程序的入口和出口处调用测试函数test，
 * 检查当前单词进入和退出该语法单位的合法性
 *
 * s1:  需要的单词集合
 * s2:  如果不是需要的单词，在某一出错状态时，
 *      可恢复语法分析继续正常工作的补充单词符号集合
 * n:   错误号
 */
void test(bool* s1, bool* s2, int n) {
    if (!inset(sym, s1)) {
        error(n);
        /* 当检测不通过时，不停获取符号，直到它属于需要的集合或补救的集合 */
        while ((!inset(sym,s1)) && (!inset(sym,s2))) {
            getsym();
        }
    }
}

/*
 syntax analysis
 lev: current level
 tx:  tail of the table
 */
void block(int lev, bool* fsys) {
    int dx;
    int tx0 = (int)table.size(); /* preserve original tx */
    int cx0; /* preserve original cx */
    
    bool nxtlev[symnum];
    dx = 3; /* leave 3 space for Static Link, Dynamic Link and Return Address */
    tableStruct item;
    item.kind = function;
    item.addr = cx;
    table.push_back(item);
    gen(jmp, 0, 0); /* generate jmp instruction but left for filled in later */
    
    if(lev > 1) { /* too much level */
        error(30);
    }
    if(sym == nul) sym = period;
    do {
        while(sym == intsym || sym == charsym) { /* to handle declaration statement */
            symbol last_sym = sym;
            getsym();
            vardeclaration(lev, &dx, last_sym);    
            
            if (sym == semicolon) {
                getsym();
            } else {
                error(9); /* 漏掉了分号 */
            }
            //printf("processing declaration\n");
        }        
        if (sym == period) break;
        //memcpy(nxtlev, statbegsys, sizeof(bool) * symnum);
        //nxtlev[ident] = true;
        test(statbegsys, declbegsys, 7);
    } while (inset(sym, declbegsys));    /* 直到没有声明符号 */
    
    code[table[tx0].addr].a = cx;    /* 把前面生成的跳转语句的跳转位置改成当前位置 */
    table[tx0].addr = cx;            /* 记录当前过程代码地址 */
    table[tx0].size = dx;           /* 声明部分中每增加一条声明都会给dx增加1，声明部分已经结束，dx就是当前过程数据的size */
    cx0 = cx;
    gen(ini, 0, dx);                /* 生成指令，此指令执行时在数据栈中为被调用的过程开辟dx个单元的数据区 */
    
    //printf("tail index = %d\n", (int)table.size());
    for (int i = 0; i < table.size(); i++) {
        switch (table[i].kind) {
            case constant:
                fprintf(ftable, "%d const %s ", i, table[i].name.c_str());
                fprintf(ftable, "val=%d\n", table[i].val);
                break;
            case variable:
                fprintf(ftable, "%d var   %s ", i, table[i].name.c_str());
                fprintf(ftable, "lev=%d addr=%d type=%d \n", table[i].level, table[i].addr, table[i].type);
                break;
            case function:
                fprintf(ftable,"%d func  %s ", i, table[i].name.c_str());
                fprintf(ftable,"lev=%d addr=%d size=%d\n", table[i].level, table[i].addr, table[i].size);
                break;
        }
    }
    fprintf(ftable,"\n");
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);    /* 每个后继符号集合都包含上层后继符号集合，以便补救 */
    nxtlev[semicolon] = true;
    statement(lev, nxtlev);
    gen(opr, 0, 0);                     /* 每个过程出口都要使用的释放数据段指令 */
    memset(nxtlev, 0, sizeof(bool) * symnum);   /* 分程序没有补救集合 */
    test(fsys, nxtlev, 8);              /* 检测后继符号正确性 */
}


/*
 * 在符号表中加入一项
 *
 * k:      标识符的种类为const，var或procedure
 * ptx:    符号表尾指针的指针，为了可以改变符号表尾指针的值
 * lev:    标识符所在的层次
 * pdx:    dx为当前应分配的变量的相对地址，分配后要增加1
 *
 */
void enter(enum object k, int lev, int* pdx, symbol last_sym) {
    tableStruct table_item;
    table_item.name = current_ident; /* 符号表的name域记录标识符的名字 */
    table_item.kind = k;
    switch (k) {
        case constant:  /* 常量 */
            if (num > addrmax) {
                error(15);  /* 常数越界 */
                num = 0;
            }
            table_item.val = num; /* 登记常数的值 */
            break;
        case variable:  /* 变量 */
            table_item.level = lev;
            table_item.addr = (*pdx);
            table_item.type = last_sym == intsym ? 0 : 1;
            (*pdx)++;
            break;
        case function:  /* 过程 */
            table_item.level = lev;
            break;
        case array:
            table_item.level = lev;
            table_item.addr = (*pdx);
            table_item.type = last_sym == intsym ? 0 : 1;
            table_item.size = arr_num;
            (*pdx) += arr_num;
            break;
    }
    table.push_back(table_item);
}



/*
 * 查找标识符在符号表中的位置，从tx开始倒序查找标识符
 * 找到则返回在符号表中的位置，否则返回0
 *
 * id:    要查找的名字
 * tx:    当前符号表尾指针
 */
int position() {
    for(int i = 0; i < table.size(); i++) {
        if(table[i].name == current_ident) return i;
    }
    return -1;
}


/**
 * [to handle variable declaration and arry declaration]
 */
void vardeclaration(int lev, int* pdx, symbol last_sym) {
    if(sym == ident) {
        recover_store();
        getsym();
        if(sym == lbracket) {
            getsym();
            if(sym == number) {
                arr_num = num;
                if(num >= stacksize) {
                    error(39);
                }
                getsym();
                if(sym == rbracket) {
                    getsym();
                    enter(array, lev, pdx, last_sym); // enter info into the table
                } else {
                    error(35);
                }

            } else {
                error(38);
            }
        } else {
            recover_load();
            enter(variable, lev, pdx, last_sym); // enter info into the table
            getsym();
        }
    } else {
        error(0); // ident should be behind int & var symbol
    }
}

/*
 * statement process
 */
void statement(int lev, bool* fsys) {    
    while(inset(sym, statbegsys)) {
        statement_item(lev, fsys);
    }
}

void statement_item(int lev, bool* fsys) {
    int cx1, cx2;
    bool nxtlev[symnum];
    if(sym == ifsym) {
        getsym();
        if(sym == lparen) {
            getsym();    //缺少左括号
        } else {
            error(17);
        }
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[lbrace] = true;
        condition(lev, nxtlev); /* 调用条件处理 */
        if(sym == rparen) {
            getsym();    //缺少左括号
        } else {
            error(16);
        }
        cx1 = cx;   /* 保存当前指令地址 */
        gen(jpc, 0, 0); /* 生成条件跳转指令，跳转地址未知，暂时写0 */
        if (sym == lbrace) {
            getsym();
        } else {
            error(20);
        }
        nxtlev[rbrace] = true;
        nxtlev[elsesym] = true;
        statement(lev, nxtlev);  /* 处理then后的语句 */
        cx2 = cx;
        gen(jmp, 0, 0);
        code[cx1].a = cx;   /* 经statement处理后，cx为then后语句执行完的位置，它正是前面未定的跳转地址，此时进行回填 */
        if (sym == rbrace) getsym();
        else error(21);
        if (sym == elsesym) {
            getsym();
            if (sym == lbrace) getsym();
            else error(20);
            
            memcpy(nxtlev, fsys, sizeof nxtlev);
            nxtlev[rbrace] = true;
            statement(lev, nxtlev);
            
            if (sym == rbrace) getsym();
            else error(21);
        }
        code[cx2].a = cx;
    } else if (sym == whilesym) {  /* 准备按照while语句处理 */

        cx1 = cx;   /* 保存判断条件操作的位置 */
        getsym();
        if(sym == lparen) {
            getsym();    //缺少左括号
        } else {
            error(17);
        }
        
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[lbrace] = true;  /* 后继符号为do */
        condition(lev, nxtlev);    /* 调用条件处理 */
        cx2 = cx;   /* 保存循环体的结束的下一个位置 */
        
        gen(jpc, 0, 0); /* 生成条件跳转，但跳出循环的地址未知，标记为0等待回填 */
        if(sym == rparen) {
            getsym();
        } else {
            error(16); //缺少右括号
        }
        if (sym == lbrace) {
            getsym();
        } else {
            error(20);  /* 缺少do */
        }
        memcpy(nxtlev, fsys, sizeof nxtlev);
        nxtlev[rbrace] = true;
        statement(lev, nxtlev);  /* 循环体 */
        if (sym == rbrace) getsym();
        else error(21);
        gen(jmp, 0, cx1);   /* 生成条件跳转指令，跳转到前面判断条件操作的位置 */
        code[cx2].a = cx;   /* 回填跳出循环的地址 */
    } else if(sym == forsym) {
        getsym();
        if(sym == lparen) {
            getsym();    //缺少左括号
        } else {
            error(17);
        }
        memcpy(nxtlev, fsys, sizeof nxtlev);
        nxtlev[semicolon] = true;
        statement_single_item(lev, fsys);
        if(sym == semicolon) {
            getsym();
        } else {
            error(9);
        }
        cx1 = cx;   /* 保存判断条件操作的位置 */
        condition(lev, nxtlev);    /* 调用条件处理 */
        cx2 = cx;   /* 保存循环体的结束的下一个位置 */
        gen(jpc, 0, 0); /* 生成条件跳转，但跳出循环的地址未知，标记为0等待回填 */
        gen(jmp, 0, 0); /* jmp 到内部语句开始的位置，回填 */
        //statement(lev, nxtlev);
        if(sym == semicolon) {
            getsym();
        } else {
            error(9);
        }
        //int cx3 = cx;
        statement_single_item(lev, nxtlev);
        gen(jmp, 0, cx1);
        if(sym == rparen) {
            getsym();
        } else {
            error(16); //缺少右括号
        }
                        
        if (sym == lbrace) {
            getsym();
        } else {
            error(20);
        }
        code[cx2+1].a = cx;
        memcpy(nxtlev, fsys, sizeof nxtlev);
        nxtlev[rbrace] = true;
        statement(lev, nxtlev);
        
        if (sym == rbrace) {
            getsym();
        } else {
            error(21);
        }
        gen(jmp, 0, cx2+2);
        code[cx2].a = cx;
    } else {
        statement_single_item(lev, fsys);
        if(sym == semicolon) {
            getsym();
        } else {
            error(1);
        }
    }
}

void statement_single_item(int lev, bool* fsys) {
    int cx1, cx2;
    bool nxtlev[symnum];
    if(sym == ident) {          //treated as assignment statement
        int pos = position();
        if(pos == -1) {
            error(6);         //none declare ident
        } else {
            tableStruct item = table[pos];
            if(item.kind == array) {
                getsym();
                if(sym == lbracket) {
                    getsym();
                } else {
                    error(40);
                }
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[rbracket] = true;
                expression(lev, fsys);
                if(sym == rbracket) {
                    getsym();
                } else {
                    error(35);
                }
            
                if(sym == becomes) {
                    getsym();
                } else {
                    error(8); //do not have assignment symbol
                }
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                expression_expand(lev, nxtlev); //to handle expression right to the assignment symbol
                gen(stoa, lev - item.level, item.addr);
            } else if(item.kind == variable){
                getsym();
                if(sym == add) {
                    gen(lod, lev - item.level, item.addr);
                    gen(lit, 0, 1);
                    gen(opr, 0, 2);
                    gen(sto, lev - item.level, item.addr);
                    getsym();
                } else if(sym == sub) {
                    gen(lod, lev - item.level, item.addr);
                    gen(lit, 0, 1);
                    gen(opr, 0, 3);
                    gen(sto, lev - item.level, item.addr);
                    getsym();
                } else {
                    if(sym == becomes) {
                        getsym();
                    } else {
                        error(8); //do not have assignment symbol
                    }
                    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                    expression_expand(lev, nxtlev); //to handle expression right to the assignment symbol
                    
                    gen(sto, lev - item.level, item.addr);
                    
                }
            } else {
                error(7);    //For assignement, ident must be on the left size
            }
        }
        
    } else if(sym == readsym) {
        getsym();
        int pos;
        int index = 0;
        if (sym == ident || sym == arrsym) {
            pos = position(); /* 查找要读的变量 */
        }
        if(pos == -1 || (sym != ident && sym != arrsym)) {
            error(18);  /* read语句括号中的标识符应该是声明过的变量 */
        } else {
            if(sym == arrsym) {
                index += arr_num;
                if(index >= table[pos].size) {
                    error(37);
                }
            }
            gen(opr, 0, 16);    /* 生成输入指令，读取值到栈顶 */
            gen(sto, lev - table[pos].level, table[pos].addr + arr_num); /* 将栈顶内容送入变量单元中 */
            getsym();
        }
        while (!inset(sym, fsys)) { // 出错补救，直到遇到上层函数的后继符号，对这段代码保留意见
            getsym();
        }
        
    } else if(sym == writesym) {
        getsym();
        int pos;
        int index = 0;
        if(sym == arrsym) {
            index += arr_num;
        }
        if (sym == ident || sym == arrsym) {
            pos = position();
        }
        if(pos == -1){
            error(18);
        } else {
            memcpy(nxtlev, fsys, sizeof nxtlev);
            expression(lev, nxtlev);
            gen(opr, 0, 14);
            gen(opr, 0, 15);
        }
    } else if(sym == callsym) {
        getsym();
        if (sym != ident) {
            error(0);   /* call后应为标识符 */
        } else {
            int pos  = position();
            if (pos == -1) {
                error(6);   /* 过程名未找到 */
            } else {
                tableStruct item = table[pos];
                if (item.kind == function) {
                    gen(cal, lev-item.level, item.addr); /* 生成call指令 */
                } else {
                    error(31);  /* call后标识符类型应为过程 */
                }
            }
            getsym();
            if (sym == lparen) {
                getsym();
            } else {
                error(19);
            }
            if (sym == rparen) {
                getsym();
            } else {
                error(19);
            }
        }
    }
}

void recover_store() {
    rec_ch = ch;
    rec_sym = sym;
    rec_current_char = current_char;
    rec_line_length = line_length;
    rec_linenum = linenum;
    rec_num = num;
    memcpy(rec_line, line, sizeof(line));
    rec_cx = cx;
    rec_current_ident = current_ident;
}

void recover_load() {
    ch = rec_ch;
    sym = rec_sym;
    current_char = rec_current_char;
    line_length = rec_line_length;
    linenum = rec_linenum;
    num = rec_num;
    memcpy(line, rec_line, sizeof(line));
    cx = rec_cx;
    current_ident = rec_current_ident;
}

/*
 *表达式,支持连等
 */
void expression_expand(int lev, bool* fsys) {
    enum symbol addop; /* 用于保存正负号 */
    bool nxtlev[symnum];

    if(sym == ident) {
        int pos = position();
        if(pos == -1) {
            error(6);
        } else {
            tableStruct item = table[pos];
            recover_store();
            getsym();
            if(item.kind == array) {
                if(sym == lbracket) {
                    getsym();
                } else {
                    error(40);
                }
                expression(lev, fsys);
                if(sym == rbracket) {
                    getsym();
                } else {
                    error(35);
                }
                if(sym != becomes) {
                    recover_load();
                    expression(lev, fsys);
                } else {
                    getsym();
                    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                    gen(cpy, 0, 0);
                    expression_expand(lev, nxtlev);                
                    gen(stoa, lev - item.level, item.addr);
                    gen(loda, lev - item.level, item.addr);
                }
            } else if(item.kind == variable) {
                if(sym != becomes) {
                    recover_load();
                    expression(lev, fsys);
                } else {
                    getsym();
                    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                    expression_expand(lev, nxtlev);                
                    gen(sto, lev - item.level, item.addr);
                    gen(lod, lev - item.level, item.addr);
                }
            } else {
                error(7);
            }
        }
    } else {
        expression(lev, fsys);
    }   
}

/*
 *表达式
 */
void expression(int lev, bool* fsys) {
    enum symbol addop; /* 用于保存正负号 */
    bool nxtlev[symnum];
    
    if(sym == plus || sym == minus) {   /* 表达式开头有正负号，此时当前表达式被看作一个正的或负的项 */
        addop = sym;    /* 保存开头的正负号 */
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(lev, nxtlev); /* 处理项 */
        if (addop == minus) {
            gen(opr,0,1);   /* 如果开头为负号生成取负指令 */
        }
    } else {    /* 此时表达式被看作项的加减 */
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(lev, nxtlev); /* 处理项 */
    }
    while (sym == plus || sym == minus) {
        addop = sym;
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(lev, nxtlev); /* 处理项 */
        if (addop == plus) {
            gen(opr, 0, 2); /* 生成加法指令 */
        } else {
            gen(opr, 0, 3); /* 生成减法指令 */
        }
    }
}

/*
 * 项处理
 */
void term(int lev, bool* fsys) {
    enum symbol mulop;  /* 用于保存乘除法符号 */
    bool nxtlev[symnum];
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[times] = true;
    nxtlev[slash] = true;
    nxtlev[mod] = true;
    
    factor(lev, nxtlev);   /* 处理因子 */
    while(inset(sym, termsys)) {
        mulop = sym;
        getsym();
        factor(lev, nxtlev);
        if(mulop == times) {
            gen(opr, 0, 4); /* 生成乘法指令 */
        } else if (mulop == slash) {
            gen(opr, 0, 5); /* 生成除法指令 */
        } else if(mulop == xorsym){ 
            gen(opr, 0, 18); //亦或运算
        } else if(mulop == mod){
            gen(opr, 0, 17); //求余运算
        }
    }
}

/*
 * 因子处理
 */
void factor(int lev, bool* fsys) {
    bool nxtlev[symnum];
    test(facbegsys, fsys, 11);  /* 检测因子的开始符号 */
    while(inset(sym, facbegsys)) {  /* 循环处理因子 */
        
        if(sym == ident) {  /* 因子为常量或变量 */
            int pos = position(); /* 查找标识符在符号表中的位置 */
            if (pos == -1) {
                error(6);   /* 标识符未声明 */
            } else {
                tableStruct item = table[pos];
                if(item.kind == array) {
                    getsym();
                    if(sym == lbracket) {
                        getsym();
                    } else {
                        error(40);
                    }
                    expression(lev, fsys);
                    if(sym == rbracket) {
                        getsym();
                    } else {
                        error(35);
                    }
                    gen(loda, lev-item.level, item.addr);
                } else if(item.kind == variable) {
                    gen(lod, lev-item.level, item.addr); /* 找到变量地址并将其值入栈 */
                    getsym();
                } else {
                    error(12); //不能是函数
                }
            }
        } else {
            if(sym == number) { /* 因子为数 */
                if (num > addrmax) {
                    error(15); /* 数越界 */
                    num = 0;
                }
                gen(lit, 0, num);
                getsym();
            } else {
                if (sym == lparen) {    /* 因子为表达式 */
                    getsym();
                    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                    nxtlev[rparen] = true;
                    expression(lev, nxtlev);
                    if (sym == rparen) {
                        getsym();
                    } else {
                        error(16);  /* 缺少右括号 */
                    }
                }
            }
        }
    }
}


/*
 * 条件处理
 */
void condition(int lev, bool* fsys) {
    //printf("condition is called, current_char sym = %s\n", symbol_word[sym].c_str());
    enum symbol relop;
    bool nxtlev[symnum];
    if(sym == oddsym) { /* 准备按照odd运算处理 */
        getsym();
        expression(lev, fsys);
        gen(opr, 0, 6); /* 生成odd指令 */
        return;
    }
    /* 逻辑表达式处理 */
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[eql] = true;
    nxtlev[neq] = true;
    nxtlev[lss] = true;
    nxtlev[leq] = true;
    nxtlev[gtr] = true;
    nxtlev[geq] = true;
    expression(lev, nxtlev);
    if (sym != eql && sym != neq && sym != lss && sym != leq && sym != gtr && sym != geq) {
        error(11); /* 应该为关系运算符 */
    } else {
        relop = sym;
        getsym();
        expression(lev, fsys);
        switch (relop) {
            case eql:
                gen(opr, 0, 8);
                break;
            case neq:
                gen(opr, 0, 9);
                break;
            case lss:
                gen(opr, 0, 10);
                break;
            case geq:
                gen(opr, 0, 11);
                break;
            case gtr:
                gen(opr, 0, 12);
                break;
            case leq:
                gen(opr, 0, 13);
                break;
        }
    }
}

/*
 * 输出所有目标代码
 */
void listall() {
    int i;
    for (i = 0; i < cx; i++) {
        printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
    }
}


/*
 * 解释程序
 */
void interpret() {
    int p = 0; /* 指令指针 */
    int b = 1; /* 指令基址 */
    int t = 0; /* 栈顶指针 */
    struct instruction i;   /* 存放当前指令 */
    int s[stacksize];   /* 栈 */
    
    s[0] = 0; /* s[0]不用 */
    s[1] = 0; /* 主程序的三个联系单元均置为0 */
    s[2] = 0;
    s[3] = 0;
    
    do {
        if (cx == 0) break;
        i = code[p];    /* 读当前指令 */
        fprintf(fdebug, "#\n");
        fprintf(fdebug, "%d\n", p);
        p = p + 1;
        switch (i.f) {
            case lit:   /* 将常量a的值取到栈顶 */
                t = t + 1;
                s[t] = i.a;
                break;
            case opr:   /* 数学、逻辑运算 */
                switch (i.a) {
                    case 0:  /* 函数调用结束后返回 */
                        t = b - 1;
                        p = s[t + 3];
                        b = s[t + 2];
                        break;
                    case 1: /* 栈顶元素取反 */
                        s[t] = - s[t];
                        break;
                    case 2: /* 次栈顶项加上栈顶项，退两个栈元素，相加值进栈 */
                        t = t - 1;
                        s[t] = s[t] + s[t + 1];
                        break;
                    case 3:/* 次栈顶项减去栈顶项 */
                        t = t - 1;
                        s[t] = s[t] - s[t + 1];
                        break;
                    case 4:/* 次栈顶项乘以栈顶项 */
                        t = t - 1;
                        s[t] = s[t] * s[t + 1];
                        break;
                    case 5:/* 次栈顶项除以栈顶项 */
                        t = t - 1;
                        s[t] = s[t] / s[t + 1];
                        break;
                    case 6:/* 栈顶元素的奇偶判断 */
                        s[t] = s[t] % 2;
                        break;
                    case 8:/* 次栈顶项与栈顶项是否相等 */
                        t = t - 1;
                        s[t] = (s[t] == s[t + 1]);
                        break;
                    case 9:/* 次栈顶项与栈顶项是否不等 */
                        t = t - 1;
                        s[t] = (s[t] != s[t + 1]);
                        break;
                    case 10:/* 次栈顶项是否小于栈顶项 */
                        t = t - 1;
                        s[t] = (s[t] < s[t + 1]);
                        break;
                    case 11:/* 次栈顶项是否大于等于栈顶项 */
                        t = t - 1;
                        s[t] = (s[t] >= s[t + 1]);
                        break;
                    case 12:/* 次栈顶项是否大于栈顶项 */
                        t = t - 1;
                        s[t] = (s[t] > s[t + 1]);
                        break;
                    case 13: /* 次栈顶项是否小于等于栈顶项 */
                        t = t - 1;
                        s[t] = (s[t] <= s[t + 1]);
                        break;
                    case 14:/* 栈顶值输出 */
                        printf("%d", s[t]);
                        fprintf(fresult, "%d", s[t]);
                        break;
                    case 15:/* 输出换行符 */
                        printf("\n");
                        fprintf(fresult,"\n");
                        break;
                    case 16:/* 读入一个输入置于栈顶 */
                        t = t + 1;
                        scanf("%d", &(s[t]));
                        break;
                    case 17: //求余
                        t = t - 1;
                        s[t] = s[t] % s[t + 1];
                        break;
                    case 18: //XOR
                        t = t - 1;
                        s[t] = s[t] ^ s[t + 1];
                }
                break;
            case lod:   /* 取相对当前过程的数据基地址为a的内存的值到栈顶 */
                t = t + 1;
                s[t] = s[base(i.l,s,b) + i.a];
                break;
            case sto:   /* 栈顶的值存到相对当前过程的数据基地址为a的内存 */
                s[base(i.l, s, b) + i.a] = s[t];
                t = t - 1;
                break;
            case cal:   /* 调用子过程 */
                s[t + 1] = base(i.l, s, b); /* 将父过程基地址入栈，即建立静态链 */
                s[t + 2] = b;   /* 将本过程基地址入栈，即建立动态链 */
                s[t + 3] = p;   /* 将当前指令指针入栈，即保存返回地址 */
                b = t + 1;  /* 改变基地址指针值为新过程的基地址 */
                p = i.a;    /* 跳转 */
                break;
            case ini:   /* 在数据栈中为被调用的过程开辟a个单元的数据区 */
                t = t + i.a;
                break;
            case jmp:   /* 直接跳转 */
                p = i.a;
                break;
            case jpc:   /* 条件跳转 */
                if (s[t] == 0)
                    p = i.a;
                t = t - 1;
                break;
            case jeq:   /* 条件跳转 */
                if (s[t] != 0)
                    p = i.a;
                t = t - 1;
                break;
            case loda: /* 以栈顶元素为偏移量，取内存的值到栈顶 */
                s[t] = s[base(i.l, s, b) + i.a + s[t]];
                break;
            case stoa: /* 以次栈顶元素为偏移量，把栈顶的值存到内存位置 */
                s[base(i.l, s, b) + i.a + s[t - 1]] = s[t];
                t = t - 2;
                break;
            case cpy: //复制当前栈顶元素的值到栈顶 
                t = t + 1;
                s[t] = s[t - 1];
                break;
        }
        for (int i = 1; i <= t; ++i) {
            fprintf(fdebug, "%d\n", s[i]);
        }
    } while (p != 0);
}

/* 通过过程基址求上l层过程的基址 */
int base(int l, int* s, int b) {
    int b1;
    b1 = b;
    while (l > 0) {
        b1 = s[b1];
        l--;
    }
    return b1;
}



