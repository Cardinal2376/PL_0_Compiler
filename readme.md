# 开发记录
[12.20]  
1.支持语法a = b = 3;  
方法：在不修改LL(0)语法的情况下支持一个字符的回溯操作，设计recover_store和recover_load函数来保存状态和恢复状态  
[12.21]  
1.修改getch的定义，修复feof判断失败的bug  
2.修改一维度数组的实现，从词法分析阶段移到语义分析阶段，支持中括号里带表达式  
3.增加三元式stoa, loda，按照偏移量变址取数和变址存数  
4.为一维数组添加连等特性，添加三元是cpy，复制当前栈顶元素  
5.添加odd条件判断  
6.添加求余运算符号%  
7.添加xor运算  
8.添加自增自减运算符  
9.添加对常量的支持  
10.添加对多维数组的支持[partly, defination first]  
11.添加对char的运算和处理[to do]  
