#include "pro.h"
int deploy_main()
{
	printf("正在配置ip...\n");
	int let=system("ifconfig eth0 192.168.1.101 netmask 255.255.255.0");
	if (let != 0)
	{
		printf("部署失败！\n");
		return -1;
	}
	else
	{
		printf("部署成功！\n");
		return 0;
	}
}