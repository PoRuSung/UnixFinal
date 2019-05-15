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

struct execution {
	string cmd;
	vector<string> argv;
};

void dopipe(execution exec);//execute pipe
void read_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type only read
void read_write_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type both read and write
void write_pipe(vector<string> p, int fd[][2], int pipe_num);//pipe type only write
void dord(execution exec);//execute redirect
void rd_out(vector<string> p, execution exec);

int main()
{
	bool count = true;//check CMD
	bool first = false;
	char buf[2000];
	string orign_dir;//directory
	string str1;//input string

	execution exec;//save CMD and ARGV
	stringstream ss;
	string value;
	char* input, shell_prompt[100];
	
	getcwd(buf,sizeof(buf) - 1);//get directory
	
	ss << buf;
	while(getline(ss, value, '/')){
		if(first){
			orign_dir.push_back('/');
			orign_dir += value;
		}
		else first = true;
	}
	ss.clear();

	while (true)
	{
		rl_bind_key('\t', rl_complete);
		snprintf(shell_prompt, sizeof(shell_prompt), "\033[01;32m%s\033[0m@\033[01;34m%s\033[0m$ ", getenv("USER"), getcwd(buf, sizeof(buf) - 1));
		
		//printf("\033[01;32m%s\033[0m@", getenv("USER"));//print username

		//printf("\033[01;34m%s\033[0m> ", getcwd(buf, sizeof(buf) - 1));// print directory

		
		count = true;
		input = readline(shell_prompt);
		//getline(cin, str1, '\n'); //input
		
		//input = readline((char*)str1.c_str());
		
		str1 = (string)input;
		//cout << "str1: " << str1 << endl;
		//if (!input) break;
		if(str1.size() == 0) continue;
      		  // Add input to history.
       		 add_history(input);

	        // Do stuff...

        	// Free input.
        	free(input);
		ss << str1;
		while (getline(ss, value, ' ')) {//deal with input
			if (count){
				exec.cmd = value;
				count = false;
			}
			else {
				exec.argv.push_back(value);
			}
		}
		ss.clear();
		if (exec.cmd == "exit") {// when CMD = exit
			cout << "ByeBye!" << endl;
			break;
		}
		else {
			if (exec.cmd == "cd" && exec.argv.size() > 0) {// when CMD = cd
				string change_index = exec.argv[0];
				if(change_index == ".."){
					if(orign_dir.size() > 0)
						getdir(change_index, orign_dir, 1);
				}
				else if (change_index[0] == '/') 
				{
					getdir(change_index, orign_dir, 2);
					
				}
				else
					getdir(change_index, orign_dir, 3);
			}
			else if(isrd(exec)){//when exit < or >
				dord(exec);
			}
			else {
				if(exec.cmd != "cd"){
					if(ispipe(exec, 0))//when exit |
						dopipe(exec);
					else
						print(exec);
				}
			}
		}
		eclear(exec);
	}
}


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

void dord(execution exec)
{
	int rdtype;
	vector<string> p;
	p.push_back(exec.cmd);
	for(int i = 0; i < exec.argv.size(); i++){
		if(exec.argv[i] == ">") rdtype = 1;
		else if(exec.argv[i] == "<") rdtype = 2;	
		p.push_back(exec.argv[i]);
	}
	rd_out(p, exec);//deal with cmd (we don't know there are pipe or not)
}

void rd_out(vector<string> p, execution exec)// do redirect
{
	int i, j, fd, defout, thepipe = 0, pipe_count = 0, defin;
	int pipefd[50][2];
	string file = "";
	vector<string> str;
	char* buf[100];
	for( i = 0; i < p.size(); i++)
		if(p[i] == ">" || p[i] == "<" || p[i] == ">>") break;
	j = i;
	string two_type = p[i];
	for( i++; i < p.size(); i++)
		file = file + p[i];
	p.resize(j);

	for(int i = 0; i < exec.argv.size(); i++){
		if(exec.argv[i] == "|")
			pipe_count++;
	}
	
	if((defout = dup(1)) < 0){
		cerr << "can't dup(1)" << endl;
		exit(0);
	}
	defin = dup(0);
	if(two_type == ">"){
		if((fd = open((char*)file.c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR)) < 0){//create a file no matter it is exist or not
			cerr << "can't open the file" << endl;
			exit(0);
		}
		dup2(fd, 1);
		close(fd);
	}
	else if(two_type == ">>"){
		if((fd = open((char*)file.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0){//create a file no matter it is exist or not
			cerr << "can't open the file" << endl;
			exit(0);
		}
		dup2(fd, 1);
		close(fd);
	}
	else{
		fd = open((char*)file.c_str(), O_RDWR, S_IRUSR | S_IWUSR);//open an exist file
		dup2(fd, 0);
		close(fd);
	}
	if(p.size() > 0){
		if(ispipe(exec, 0)){
			for(int i = 0; i < pipe_count; i++){
				pipe(pipefd[i]);
			}
			for(i = 0; i < p.size(); i++){
				str.push_back(p[i]);
				if(p[i] == "|" || (i + 1) == p.size()){
					if(p[i] == "|") str.pop_back();
					if(thepipe == 0){//write
						if(fork() == 0){
							write_pipe(str, pipefd, thepipe);
						}
						wait(NULL);
					}
					else if(ispipe(exec, i - 1)){//read and write
						if(fork() == 0){
							read_write_pipe(str, pipefd, thepipe);
						}
						close(pipefd[thepipe - 1][0]);
						close(pipefd[thepipe - 1][1]);
						wait(NULL);
					}
					else{// read
						if(fork() == 0){
							read_pipe(str, pipefd, thepipe);
						}
						close(pipefd[thepipe - 1][0]);
						close(pipefd[thepipe - 1][1]);
						wait(NULL);

					}
					thepipe++;
					str.clear();
				}
			}
		}
		else{
			for(i = 0; i < p.size(); i++){
				buf[i] = (char*)p[i].c_str();
			}
			buf[i] = NULL;
			if(fork() == 0){
				execvp(buf[0], buf);
			}
		
			wait(NULL);
		}
	}
	if(two_type == ">"){//recover to the default stdout
		fflush(stdout);
		dup2(defout,1);
		close(defout);
	}
	else if(two_type == ">>"){
		fflush(stdout);
		dup2(defout,1);
		close(defout);
	}
	else{// recover to the default stdin
		fflush(stdin);
		dup2(defin, 0);
		close(defin);
	}
}