#define GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline int finit_module(int fd, const char *uargs, int flags)
{
	return syscall(__NR_finit_module, fd, uargs, flags);
}

int main(int argc, char **argv)
{
	long res;
	int fd;

	if(argc != 2)
	{
		printf("USAGE : lkrd_insmod [file path]\n");
		return -1;
	}
	
	fd = open(argv[1], O_RDONLY | O_CLOEXEC);
	if(fd < 0)
	{
		printf("[%s] open error\n", argv[1]);
		return -1;
	}

	res = finit_module(fd, "", 0);
	if(res)
	{
		printf("finit_module error : %ld\n", res);
	}

	close(fd);
	return 0;
}
