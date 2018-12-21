# 开发记录
[12.20]
1.支持语法a = b = 3;
方法：在不修改LL(0)语法的情况下支持一个字符的回溯操作，设计recover_store和recover_load函数来保存状态和恢复状态
[12.21]
1.修改getch的定义，修复feof判断失败的bug

