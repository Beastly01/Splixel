#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include "png.h"
#include "lights.h"

#include "fontfiles.h"

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int useRow;
int rainbow=0;	// makes the characters all rainbowey... best with white fonts..
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
int bytes_per_pixel=3;
unsigned char *font_data;
// input the ordinal slot, which represents the interpolation position of the color you want
// input max which represents the end of the "rainbow" position.
// let x = 5 and max = 10, you'd wand the middle of the rainbow color, which would return 00ff00
// let x =0  regardless of max, the returned color is ff0000 as this is at the start of the colour wheel
char *colours(int slot, int max)
{
   float mult = 1536/max;      // 256 possible colors per color, 3 colours. then 2x..256*2*3 = 1536
   int adj = slot * mult;       // If slot is scaled into the 0-1536 
   int fwd,bwd;
   static char cols[3];		// colours R G B in array, returned by fn
   adj = adj % 1536;     // in case ppl are stupid, and $x > $max;
   fwd = adj % 256; // this computes the forward-counting offset
   bwd = 255-fwd;  // this computes the backward-counting offset
   bwd = 255-fwd;  // this computes the backward-counting offset
   if (adj < 256)
   {
	  cols[0]=0xff;
	  cols[1]=fwd;
	  cols[2]=0;
   }
   else
   {
      if (adj < 512)
      {
		cols[0]=bwd;
		cols[1]=0xff;
		cols[2]=0x00;
      }         
      else      
      {         
         if (adj < 768)
         {         
			cols[0]=0;
			cols[1]=0xff;
			cols[2]=fwd;
         }              
         else           
         {              
            if (adj<1024)      
            {           
				cols[0]=0;
				cols[1]=bwd;
				cols[2]=0xff;
            }                   
            else                
            {                   
               if (adj < 1280)         
               {        
					cols[0]=fwd;
					cols[1]=0;  
					cols[2]=0xff;
               }                        
               else                     
               {                        
                  if (adj < 1536)              
                  {     
					cols[0]=0xff;
					cols[1]=0x0;
					cols[2]=bwd;				  
                  }                             
               }                        
            }                   
         }              
      }         
	}
	return(cols);
}   


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

        /* expand paletted colors into true rgb */
        if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
           png_set_expand(png_ptr);

        /* expand grayscale images to the full 8 bits */
        if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY &&
           info_ptr->bit_depth < 8)
           png_set_expand(png_ptr);

        /* expand images with transparency to full alpha channels */
        if (info_ptr->valid & PNG_INFO_tRNS)
           png_set_expand(png_ptr);

        //SET BACKGROUND
        png_color_16 my_background;
        png_color_16p image_background;

        my_background.red = 0;
        my_background.green = 0;
        my_background.blue = 0;

        if (png_get_bKGD(png_ptr, info_ptr, &image_background))
            png_set_background(png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
        else if (info_ptr)
            png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
        //END SET BACKGROUND

        /* tell libpng to strip 16 bit depth files down to 8 bits */
        if (info_ptr->bit_depth == 16)
           png_set_strip_16(png_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);

        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        printf("Orig Color type %d, New Color type %d, rowbytes %d\n",color_type,info_ptr->color_type,png_get_rowbytes(png_ptr,info_ptr));

        //if(info_ptr->color_type!=6){
        //	abort_("Unable to handle new color type after expansion");
        //}
        if(info_ptr->color_type==6){
        	bytes_per_pixel=4;
        }else if(info_ptr->color_type==2){
        	bytes_per_pixel=3;
        }

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        int y;
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}

void draw_buffer(unsigned char *char_data){
	int x,y;
	unsigned char r,g,b;
	static int updates=0;
	char buff[8][3];	// store the colours for this frame
	++updates;
#define frames_per_newcolour 3
// RAINBOW_SIZE is the number of colours in the rotation, eg 32 means a gradient of 8 out of 32 per update
#define RAINBOW_SIZE 32
	if (rainbow)
	{
		int base;
		base = updates/frames_per_newcolour;
		for (y=0;y<8;++y)
		{
			memcpy(&buff[y][0],colours(base+y,RAINBOW_SIZE),3);
		}
	}
		
	for(y=0;y<char_pixel_height;y++){
		for(x=0;x<char_pixel_width;x++){
			r = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)];
			g = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+1];
			b = char_data[(y*bytes_per_pixel*char_pixel_width)+(x*bytes_per_pixel)+2];
			if (rainbow && (r>100 || g>100 || b>100))
			{
				r=buff[y][0];
				g=buff[y][1];
				b=buff[y][2];
			}
			setpixel(x,y,r,g,b);
		}
	}
    refresh();
	usleep(50000);
}

void print_char_at_index(int cx, int cy, unsigned char *font_data){
	int offset = ((cy * chars_per_row) + cx) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
	draw_buffer(&font_data[offset]);
}

//ideally we'd just read in a text file with the same chars as in the font png
unsigned char *get_pointer_to_char_data(char wanted){
//		char *d[] = {"                                ",
//				    " !\"#$%&'()*+,-./0123456789:;<=>?",
//				    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\] _",
//				    " abcdefghijklmnopqrstuvwxyz{|}~ ",
//				    "  '   - ^                       ",
//				    "                                ",
//				    "                                ",
//				    "                                "};
	    char *d[] = {" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"};

		//find char in table.. yes this could be smarter ;p
		int x,y,found=0;

		//because we encode unknown chars with space, we have to handle space
		//specially.
		if(wanted==' '){
			found=1;
			x=0;y=0;
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
			x=0;y=0;
		}
                if(useRow!=0){
                        y=useRow;
                }
		int offset = ((y * chars_per_row) + x) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
		return &font_data[offset];
}

void print_string(char *string){
	int idx;
	int len=strlen(string);
	for(idx=0; idx<len;idx++){
		draw_buffer(get_pointer_to_char_data(string[idx]));
	}
}

void process_file(void)
{
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
	    		//printf("char %d,%d\n",currentCharRow,currentChar);
	    		int offset = ((currentCharRow * chars_per_row) + currentChar) * (bytes_per_pixel*char_pixel_height*char_pixel_width);
	    		for(pixelrow=0; pixelrow<8; pixelrow++){
	    			png_byte* row = row_pointers[(8*currentCharRow)+pixelrow];
	    			for(pixel=0; pixel<8; pixel++){
	    				png_byte* ptr = &(row[(currentChar*8*bytes_per_pixel)+(pixel*bytes_per_pixel)]);

	    				font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)] = ptr[0];
	    				font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)+1] = ptr[1];
	    				font_data[offset+(pixelrow*bytes_per_pixel*char_pixel_width)+(pixel*bytes_per_pixel)+2] = ptr[2];
	    				//printf("%02X%02X%02X ",ptr[0],ptr[1],ptr[2]);
	    			}
	    			//printf("\n");
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
	  draw_buffer(buffer);
  }
  free(buffer);
  draw_buffer(bdata);
}

void scroll_string(char *string, int step){
	int idx;
	int len = strlen(string);
	if(len<2){
		draw_buffer(&string[0]);
	}else{
		for(idx=0; idx<(len-1); idx++){
			scroll_two(string[idx],string[idx+1],step);
		}
	}
}
void usage()
{
	printf("Usage:\n  driver [-f:0-136[-s:#] [\"string to print\"]\n");
	printf("  -f is the font number.  Sucks to use numbers, I know.  So what?\n");
	printf("  -s is for subfont, if there are any.  Zero based. \n");
	printf("  -r is rainbow mode.  Best used with a white font\n");
	printf("	EG:driver -f113 -s2 \"hello\" uses super mario font, third subfont \n");
	exit(0);
}
int main(int argc, char **argv)
{
	int c;    
	int fontnumber=10;
	useRow=0;

	while ( (c = getopt(argc, argv, "f:s:r") ) != -1) 
	{
		switch (c) 
		{
		case 'f':
			fontnumber=atoi(optarg);
			break;
		case 's':
			useRow=atoi(optarg);
			break;
		case 'r':
			rainbow = 1;
			break;
		case '?':
			fprintf(stderr, "Unrecognized option (%c)!\n",c);
			break;
		}
	}
	if ( argc-optind > 1)
	{
		printf("You can only have one quoted string parameter..\n");
		usage();
	}
	char *fontImageName = fonts[fontnumber];
    printf("Using font %s\n",fontImageName);
       read_png_file(fontImageName);
       process_file();
    if (openserial("/dev/ttyAMA0")<0)
    {
        perror("opening /dev/ttyAMA0");
        exit(-1);
    }
	if (argc-optind==1)
	{
		scroll_string(argv[optind],1);
	}
	else
		scroll_string("  Spikenzielabs LED driver  ",1);
    return 0;
}
