#ifndef __light_h__
#define __light_h__

// set all colours to same value  L_SETALL,R,G,B
#define L_SETALL 65
// set max number of LEDS (1-64)  L_MAXLED,12,DC,DC
#define L_MAXLED 66
// Force update L_FUPDATE,DC,DC,DC
#define L_FUPDATE 79
// Blink ALL LEDS 	L_BLINK,{delay},0,0
#define L_BLINK 69 

// send a command in the first parm, and the rest of the parms as mandatory parms
int sendcmd(int ,int,int,int);
// forced a redraw for delayed video and pixel setting stuff
void refresh(void);
// clear the screen (just calls setall(0,0,0)
void clrscr(void);
// sets a given pixel from 0-7 and 0-7
void setpixel(int x,int y,int r,int g,int b);
// set all pixels to r g b - clear screen to any colour? :)
void setall(int r, int g, int b);
// pass the name of the serial port, and it opens it, sets the baud and terminal options or returns -1.
int openserial(char *);	

void resetlink();

#endif