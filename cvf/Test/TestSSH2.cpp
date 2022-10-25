
#include"BFC/ssh.h"

int main()
{
	ff::SSHSession ssh;
	ssh.connect("192.168.73.130", "zf", "fanz");

	ssh.exec("cd /mnt/\npwd\nls ./", stdout);
	printf("#\n");
	ssh.exec("ls ~", stdout);
	int n = ssh.nchannels();

	return 0;
}

#include"BFC/ssh.cpp"
#include"BFC/src.cpp"
