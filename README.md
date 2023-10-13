# MyShell
简易UNIX shell。A simple UNIX shell.

## myshell的功能概述
1、可以运行内部和外部命令，如ls、echo、cd、mkdir等  
2、可以运行不带参命令和一部分带参命令(参数中不含空格即可运行)  
3、可以运行绝对路径、相对路径下的命令  
4、容许输入冗余的空格，容许错误输入、空输入  
5、可使用exit命令结束myshell程序  
6、支持单独重定向标准输入或标准输出、可以用管道连接两个命令  
7、对于错误输入，有一定的报错提醒功能  

## 功能实现用到的系统调用
•	进程管理：fork()，waitpid()  
•	IO重定向：close()，dup2()，open()  
•	管道：pipe()  
•	支持cd：chdir()  
•	执行命令：execvp()  

## 功能实现流程伪代码：
```
while (not exit) {
	fgets(input);
	parseInput(); // 预处理输入，判断是否有特殊符号
	run();
}

run() {
    if (contains(">>") || contains(">")) {
        创建子进程
        重定向输出并运行
    }
    else if (contains("<")) {
        创建子进程
        重定向输入并运行
    }
    else if (contains("|")) {
        创建管道和子进程
        重定向并运行
    } 
    else if (contains("cd")) {
        调用chdir()
    }
    else  {
        创建子进程并运行
    }
}
```
