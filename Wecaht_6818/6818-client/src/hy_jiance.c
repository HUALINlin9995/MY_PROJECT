#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "pro.h"

bool is_tupian = false;

//读取ADC值的函数
int read_adc_value()
{
	FILE* fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", "r");
	if(fp==NULL)
	{
		perror("Failed to open ADC file");
		return -1;
	}
	int adc_value;
	fscanf(fp, "%d", &adc_value);
	fclose(fp);
	return adc_value;
}
int jc_main()
{
	int adc_value;
	while(1)
	{
		adc_value = read_adc_value();
		if(adc_value==-1)
			exit(1);
		//printf("检测到ADC的值：d\n", adc_value);
		if (adc_value < 330)
		{
			printf("检测到火焰\n");
			bj_main(1);
			http_send(SERVER_IP, PORT, "error");
			wallpaper_main("/ceshi/tubiao/huoyan.bmp");
			is_tupian = true;
		}
		else
		{
			bj_main(0);
			if (is_tupian)
			{
				wallpaper_main(WALLPAPER_NAME);
				tubiao_main();
				is_tupian = false;
			}
		}
		sleep(1);
	}
	return 0;
}