#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

//所有的后台进程pid
pid_t background_childs[256];
//所有的后台进程命令
char background_cmds[256][256];
//所有的后台进程个数
int n_background_childs = 0;

//shell执行的当前命令pid
pid_t current_pid = -1;

//Ctrl-C信号处理函数
void sigint_handle(int signo)
{
	int i;

	//当前shell在执行命令，直接kill
	if (current_pid != -1) {
		kill(current_pid, SIGINT);
	}
	//直接kill所有的后台进程
	for (i = 0; i < n_background_childs; i++) {
		kill(background_childs[i], SIGINT);
	}
}

//列出所有的后台进程和命令
void list_jobs()
{
	int i, ret;

	printf("List of backgrounded processes:\n");
	for (i = 0; i < n_background_childs; i++) {
		//waitpid判断进程是否存活，返回0代表RUNNING, 否则代表finished
		ret = waitpid(background_childs[i], NULL, WNOHANG);
		printf("Command %s with PID %d Status: ", background_cmds[i], background_childs[i]);
		if (ret == 0) {
			printf("RUNNING\n");
		} else {
			printf("FINISHED\n");
		}
	}
}

int main()
{
	char cmd[256];
	pid_t pid;

	//注册Ctrl-C信号处理函数
	signal(SIGINT, sigint_handle);
	while (1) {
		int background = 0;

		printf("minish> ");
		if (NULL == fgets(cmd, 256, stdin)) {
			printf("read error\n");
			continue;
		}
		//输入命令最后带有'\n',去掉
		if (cmd[strlen(cmd) - 1] == '\n') {
			cmd[strlen(cmd) - 1] = '\0';
		}
		//输入命令最后带有'&',代表后台进程
		if (cmd[strlen(cmd) - 1] == '&') {
			cmd[strlen(cmd) - 1] = '\0';
			background = 1;
		}
		if (strcmp(cmd, "listjobs") == 0) {
			list_jobs();
			continue;
		} else if (strcmp(cmd, "exit") == 0) {
			int i;

			//kill掉所有后台的进程，然后退出
			for (i = 0; i < n_background_childs; i++) {
				kill(background_childs[i], SIGKILL);
			}
			for (i = 0; i < n_background_childs; i++) {
				waitpid(background_childs[i], NULL, 0);
			}
			break;
		} else if (strcmp(cmd, "pwd") == 0) {
			char buf[128];

			//获取当前目录
			if (getcwd(buf, 128) == NULL) {
				printf("getcwd error\n");
			} else{
				printf("%s\n", buf);
			}
			continue;
		} else if (strncmp(cmd, "cd ", 3) == 0) {
			//改变当前目录
			chdir(&cmd[3]);
			continue;
		} else if (strncmp(cmd, "fg ", 3) == 0) {
			//等待后台进程的执行
			pid_t fg_pid = atoi(&cmd[3]);
			waitpid(fg_pid, NULL, 0);
			continue;
		} else if (strcmp(cmd, "") == 0) {
			//输入为空字符串时，不处理
			continue;
		}
		pid = fork();
		if (pid > 0) {
			//父进程
			if (background == 1) {
				background_childs[n_background_childs] = pid;
				strcpy(background_cmds[n_background_childs], cmd);
				n_background_childs++;
			} else {
				//非后台进程等待执行完毕
				current_pid = pid;
				waitpid(pid, NULL, 0);
				current_pid = -1;
			}
		} else if (pid == 0) {
			//子进程
			char *parts[128];
			int n_parts = 0, i;
			char *start = cmd;
			//按照空格分隔命令成多个字符串，保存在parts之中
			for (i = 0; i < strlen(cmd); i++) {
				if (cmd[i] == ' ') {
					cmd[i] = '\0';
					parts[n_parts] = start;
					n_parts++;
					start = &cmd[i + 1];
				}
			}
			parts[n_parts] = start;
			n_parts++;
			//parts最后一个元素以NULL结束
			parts[n_parts] = NULL;
			//使用execvp执行输入的所有命令
			execvp(parts[0], parts);
			return 0;
		} else {
			printf("fork error\n");
			break;
		}
	}
	return 0;
}
