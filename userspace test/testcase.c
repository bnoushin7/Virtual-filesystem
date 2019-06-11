#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
void handler(int signum)
{
	printf("signal %d received. \n", signum);
}

int main()

{
	signal(SIGUSR1, handler);
	pid_t pid_arr[3];
	int i;
	for(i=0; i<3; i++)
	{
		pid_arr[i] = fork();
		if(pid_arr[i]==0){
				printf("child process :: %d\n", getpid());
			while(1){
				sleep(1);
			}


		}
		else{
			printf("parent process :: %d\n", getpid());
			sleep(1);
		}
	}
		wait(NULL);
}
