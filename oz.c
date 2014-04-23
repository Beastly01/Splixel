#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lights.h"

#define PNG_DEBUG 3
#include "png.h"

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

int chars_per_row;
int no_of_rows;
static int char_pixel_width=8;
static int char_pixel_height=8;
static int bytes_per_pixel=3;
unsigned char *font_data;


void read_png_file(char* file_name)
{
        unsigned char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);

        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        int y;
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}

void print_char(unsigned char *char_data)
{
	int x,y;
	unsigned char r,g,b;
	for(y=0;y<char_pixel_height;y++){
		for(x=0;x<char_pixel_width;x++)
		{
			r = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)];
			g = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+1];
			b = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+2];
			r = 255-r;
			g = 255-g;
			b = 255-b;
			setpixel(x,y,r,g,b);
		}
	}
	refresh();
	usleep(50000);
}

void print_char_at_index(int cx, int cy, unsigned char *font_data){
int offset = ((cy * chars_per_row) + cx) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
print_char(&font_data[offset]);
}

//ideally we'd just read in a text file with the same chars as in the font png
unsigned char *get_pointer_to_char_data(char wanted){
char *d[] = {"                                ",
   " !\"#$%&'()*+,-./0123456789:;<=>?",
   "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\] _",
   " abcdefghijklmnopqrstuvwxyz{|}~ ",
   "  '   - ^                       ",
   "                                ",
   "                                ",
   "                                "};

//find char in table.. yes this could be smarter ;p
int x,y,found=0;

//because we encode unknown chars with space, we have to handle space
//specially.
if(wanted==' '){
found=1;
x=0;y=1;
}else{
for(y=0;y<no_of_rows;y++){
char *row = d[y];
for(x=0;x<chars_per_row;x++){
if(row[x]==wanted){
found=1;
break;
}
}
if(found){
break;
}
}
}
if(found==0){
x=31;y=7;
}
int offset = ((y * chars_per_row) + x) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
return &font_data[offset];
}

void print_string(char *string){
int idx;
int len=strlen(string);
for(idx=0; idx<len;idx++){
print_char(get_pointer_to_char_data(string[idx]));
}
}

void process_file(void)
{
//we really should check the png format.. but since we only have the one png...;p
   chars_per_row=width/char_pixel_width;
   no_of_rows=height/char_pixel_height;

   printf("Processing image with %d,%d giving %d chars in %d rows\n",height,width,chars_per_row,no_of_rows);
        printf("Bit depth is set to %d\n",bit_depth);
   int pixelrow = 0;
   int pixel = 0;

   font_data = malloc(no_of_rows * chars_per_row * char_pixel_height * char_pixel_width * bytes_per_pixel);

   int currentCharRow = 0;
   int currentChar = 0;
   for(currentCharRow=0; currentCharRow<no_of_rows; currentCharRow++){
   	for(currentChar=0; currentChar<chars_per_row; currentChar++){
   	int offset = ((currentCharRow * chars_per_row) + currentChar) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
   	for(pixelrow=0; pixelrow<8; pixelrow++){
   	png_byte* row = row_pointers[(8*currentCharRow)+pixelrow];
   	for(pixel=0; pixel<bit_depth; pixel++){
   	png_byte* ptr = &(row[(currentChar*bit_depth)+pixel]);
   	if(bit_depth==4){
   	int actpixel=pixel*2;
   	unsigned char high=(
   	           (ptr[0]&0xF0)
   	           |
   	           (
   	             (
   	               (ptr[0]&0xF0)>>4
   	             )&0x0F
   	           )
   	          );
unsigned char low=(
(ptr[0]&0x0F)
|
(
 (
(ptr[0]&0x0F)<<4
 )&0xF0
)
  );
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)] = high;
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)+1] = high;
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)+2] = high;
   	actpixel++;
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)] = low;
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)+1] = low;
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(actpixel*bytes_per_pixel)+2] = low;
   	}else if(bit_depth==8){
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)] = ptr[0];
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)+1] = ptr[0];
   	font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)+2] = ptr[0];
   	}else{
   	printf("Unsupported bit-depth");
   	break;
   	}
   	}
   	}
   	}
   }
}

void scroll_two(char a, char b, int step){
  unsigned char *buffer = malloc(char_pixel_height * char_pixel_width * bytes_per_pixel);
  unsigned char *adata = get_pointer_to_char_data(a);
  unsigned char *bdata = get_pointer_to_char_data(b);

  int offset,x,y;
  for(offset=0;offset<char_pixel_width;offset+=step){
 memset(buffer,255,(char_pixel_height * char_pixel_width * bytes_per_pixel));
 for(y=0;y<char_pixel_height;y++){
 for(x=0;x<(char_pixel_width-offset);x++){
 buffer[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)]=adata[(y*bytes_per_pixel*char_pixel_width)+((x+offset)*bytes_per_pixel)];
 buffer[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+1]=adata[(y*bytes_per_pixel*char_pixel_width)+((x+offset)*bytes_per_pixel)+1];
 buffer[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+2]=adata[(y*bytes_per_pixel*char_pixel_width)+((x+offset)*bytes_per_pixel)+2];
 }
 for(x=0;x<offset;x++){
 //printf("%d,%d ",x,(x+(char_pixel_width-offset)));
 buffer[(y*bytes_per_pixel*char_pixel_width)+((x+(char_pixel_width-offset))*bytes_per_pixel)]=bdata[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)];
 buffer[(y*bytes_per_pixel*char_pixel_width)+((x+(char_pixel_width-offset))*bytes_per_pixel)+1]=bdata[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+1];
 buffer[(y*bytes_per_pixel*char_pixel_width)+((x+(char_pixel_width-offset))*bytes_per_pixel)+2]=bdata[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+2];
 }
 //printf("\n");
 }
 print_char(buffer);
  }
  free(buffer);
  //print_char(bdata);
}

void scroll_string(char *string, int step){
int idx;
int len = strlen(string);
if(len<2){
print_char((char) string[0]);
}else{
for(idx=0; idx<(len-1); idx++){
scroll_two(string[idx],string[idx+1],step);
}
}
}
int main(int argc, char **argv)
{
   char *fontImageName = "ZxSpectrum_AsciiLatin.png";
        read_png_file(fontImageName);
        process_file();
	if (openserial("/dev/ttyAMA0")<0)
	{
		perror("opening /dev/ttyAMA0");
		exit(-1);
	}
   //print_string("Coffee!");
        //scroll_two('a','b',1);
    scroll_string("  Eric and Ozzy For Spikenzielabs   ",1);
    return 0;
}


