#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
using namespace std;

void dopipe(execution exec);//execute pipe
void read_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type only read
void read_write_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type both read and write
void write_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type only write




void dopipe(execution exec)
{
	vector<string> p;
	vector<string> str;
	int pipe_count = 0;
	
	int pipefd[50][2];
	
	for(int i = 0; i < exec.argv.size(); i++){//compute how many pipe we need
		if(exec.argv[i] == "|")
			pipe_count++;
	}
	
	for(int i = 0; i < pipe_count; i++){//build pipe
		pipe(pipefd[i]);
	}
	
	str.push_back(exec.cmd);
	for(int i = 0; i < exec.argv.size(); i++)
		str.push_back(exec.argv[i]);
	
	for(int j = 0, thepipe = 0; j < str.size(); j++){//ex: A | B | C
		p.push_back(str[j]);
		if(str[j] == "|" || (j + 1) == str.size()){
			if(p[p.size() - 1] == "|")
				p.pop_back();
			if(thepipe == 0){//stdin -> A -> pipe
				if(fork() == 0){
					write_pipe(p, pipefd, thepipe);
				}
				wait(NULL);
			}
			else{
				if(ispipe(exec, j - 1)){
					if(fork() == 0){//pipe -> B -> pipe
						read_write_pipe(p, pipefd, thepipe);
					}
					close(pipefd[thepipe - 1][0]);
					close(pipefd[thepipe - 1][1]);
					wait(NULL);
				}
				else{
					if(fork() == 0){//pipe -> C -> stdout
						read_pipe(p, pipefd, thepipe);
					}
					close(pipefd[thepipe - 1][0]);
					close(pipefd[thepipe - 1][1]);
					wait(NULL);
				}
			}
			thepipe++;
			p.clear();
		}
	}
	for(int i = 0; i < pipe_count; i++){//close pipe
		close(pipefd[i][0]);
		close(pipefd[i][1]);
	}
	
}

void read_pipe(vector<string> p, int fd[][2], int pipe_num)
{
	char* buf[100];
	int i = 0;
	for(; i < p.size(); i++){
		buf[i] = (char*)p[i].c_str();
	}
	buf[i] = NULL;

  // input from stdin (already done)
  // output to pipe1
  dup2(fd[pipe_num - 1][0], 0);

  // close fds
  close(fd[pipe_num - 1][0]);
  close(fd[pipe_num - 1][1]);

  // exec
  execvp((char*)p[0].c_str(), buf);
  // exec didn't work, exit
  perror("bad exec");
  exit(0);
}

void read_write_pipe(vector<string> p, int fd[][2], int pipe_num)
{
	char* buf[100];
	int i = 0;
	for(; i < p.size(); i++){
		buf[i] = (char*)p[i].c_str();
	}
	buf[i] = NULL;

  // input from stdin (already done)
  // output to pipe1
  dup2(fd[pipe_num - 1][0], 0);
  dup2(fd[pipe_num][1], 1);
  // close fds
  close(fd[pipe_num - 1][0]);
  close(fd[pipe_num - 1][1]);
  close(fd[pipe_num][0]);
  close(fd[pipe_num][1]);
  // exec
  execvp((char*)p[0].c_str(), buf);

  // exec didn't work, exit
  perror("bad exec");
  exit(0);
}

void write_pipe(vector<string> p, int fd[][2], int pipe_num)
{
	char* buf[100];
	int i = 0;
	for(; i < p.size(); i++){
		buf[i] = (char*)p[i].c_str();
	}
	buf[i] = NULL;

  // input from stdin (already done)
  // output to pipe1
  dup2(fd[pipe_num][1], 1);
  // close fds
  close(fd[pipe_num][0]);
  close(fd[pipe_num][1]);
  // exec
  execvp((char*)p[0].c_str(), buf);

  // exec didn't work, exit
  perror("bad exec");
  exit(1);
}