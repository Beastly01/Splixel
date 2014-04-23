#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>			//Used for UART
#include <stdlib.h>
#include "lights.h"

int ser0fp = -1; // serial 0 file pointer
#define LEDS 64
// PIC serial timeout.. wait this long, and cmd buffer is reset
#define TIMEOUT 350000 
// send a command to light bar
// return 1 for success and 0 for failure
int 
sendcmd(int cmd,int a,int b,int c)
{
	int x;
	char buff[4];
	buff[0]=cmd;
	buff[1]=a;
	buff[2]=b;
	buff[3]=c;
	if (ser0fp != -1)
	{
		int count = write(ser0fp, buff, 4);
		if (count == 4) return(count);
		return(0);
	}
	else
		return(0);
	
}
// causes toy to send RAM to screen
void refresh()
{
	sendcmd(L_FUPDATE,0,0,0);
}
void
setall(int r, int g, int b)
{
	sendcmd(L_SETALL,r,g,b);
}

void clrscr() // clear the screen
{
	setall(0,0,0);
}
// pixels are zero based.  So, 0,0 is top left of screen.  7,0 is top right, 7,7 is bottom right.
void setpixel(int x,int y,int r,int g,int b)
{
	int pos;
	if (x<=7 && y<=7) 
	{
		pos = (y*8)+(x)+1;
		sendcmd(128+pos,r,g,b);
	}
}

int openserial(char *fn)
{
	ser0fp = open(fn, O_RDWR | O_NOCTTY );		//Open in blocking read/write mode
	if (ser0fp == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		return(-1);
	}
	struct termios options;
	tcgetattr(ser0fp, &options);
	options.c_cflag = B57600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(ser0fp, TCIFLUSH);
	tcsetattr(ser0fp, TCSANOW, &options);
	resetlink();	// ensure trash from opening the port is timed out
	return(ser0fp);
}

void resetlink()
{
	usleep(TIMEOUT); // ensure we're in command mode
}
#ifdef DEBUG
main()
{
	int x,y; // generic loop variables
	if (openserial("/dev/ttyAMA0")<0)
	{
		perror("Opening serial port");
		exit(-1);
	}
	resetlink(); 	// ensure we're in command mode, as sometimes a rogue character is sent accidentally to ctrlr
	
	//----- TX BYTES -----
	if (!sendcmd(L_SETALL,LEDS,0,0))	 // set number of LEDs to 8
	{
		printf("can't set LEDs to 8\n");
	}

	for (x=0;x<LEDS;++x)
	{
		for (y=0;y<LEDS;++y)
		{
			if(x==y) sendcmd(129+y,100,100,100);
			else sendcmd(y+129,0,0,0);
		}
		refresh();
		usleep(75000);
	}		
	for (x=LEDS-1;x>=0;--x)
	{
		for (y=0;y<LEDS;++y)
		{
			if(x==y) sendcmd(129+y,100,100,100);
			else sendcmd(y+129,0,0,0);
		}
		refresh();
		usleep(75000);
	}
}
#endif
