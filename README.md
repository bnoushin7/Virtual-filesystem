# Virtual filesystem

This project implements a pseudo-filesystem in Linux, giving to the user information about
the hierarchy of processes currently running on the system, and allowing to send a signal to these processes.

## Project description

Once mounted, each folder in this filesystem is named after the PID of the corresponding process.The directory tree
replicates the process hierarchy.


### Installing
User space test program:

The user space test program (testcase.c) having a very simple Makefile, registers SIGUSR1 and forks three times. Signal handler is implemented to prints "signal number %d received".
The "testcase" should be run before mounting the filesystem. It starts printing parent/child is running and then keeps just running, without printing anything. 
Filesystemshould be mounted after launching the "testcase" and the hierarchy of these processes is also shown in the hierarchy of processes.
In order to see the statistics about the processes, "cat /path/to/the/pid/pid.status" should be used.
In order to sendthe signal, "echo "10" > /path/to/the/pid/signal" should be used.

FileSystem program

The lwnfs.c is the filesystem module, which has a make file associated with it. 
These are the steps required for the module to be working.

0- These command in the makefile should be changed according to what the VM sets the IP to:
	INSTALL_TARGET=user@192.168.53.89:~
NOTE: The Makefile assumes that you are working in your home directory.
1- Make
2- Make install 
This command "scp $(MOD_NAME).ko $(INSTALL_TARGET)" in the Makefile, copies the "lwnfs.ko" module to the guest machin (in my case KVM)
3-"sudo insmod lwnfs.ko"
The lwnfs.ko module should be first copied and installed on the guest machine.
4- mkdir dir
"dir" is the directory for mounting the filesystem.
5- It should be mounted like "sudo mount -t lwnfs lwnfs dir/"
