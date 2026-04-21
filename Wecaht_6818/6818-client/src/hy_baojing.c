#include "pro.h"

int fd = -1;
int beep_open()
{
	fd = open("/dev/beep",O_RDWR);
	if (fd < 0)
		return fd;
	return 0;
}

void beep_write(int i)
{
	if (i == 1)
	{
		ioctl(fd, 0, 1);
	}
	else if (i == 0)
		ioctl(fd,1,1);
	return;
}

void beep_close()
{
	if (fd > 0)
		close(fd);
}

int bj_main(int i)
{
	beep_open();
	beep_write(i);
	beep_close();
	return 0;
}