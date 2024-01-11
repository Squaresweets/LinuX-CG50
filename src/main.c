#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/file.h>


#define IMAGE_PATH "\\\\fls0\\Image.txt"
int ram_amt = 32*1024;
char* ram_image = 0;
struct MiniRV32IMAState * core;

static void Print(char* buf);
static int keyPressed(int basic_keycode);

static uint32_t HandleControlStore( uint32_t addy, uint32_t val );
static uint32_t HandleControlLoad( uint32_t addy );
static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value );
static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno );

//actually include it
#define MINIRV32WARN( x... ) PrintWarning( x );
#define MINIRV32_DECORATE  static
#define MINI_RV32_RAM_SIZE ram_amt
#define MINIRV32_IMPLEMENTATION

#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( HandleControlStore( addy, val ) ) return val;
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = HandleControlLoad( addy );
#define MINIRV32_OTHERCSR_WRITE( csrno, value ) HandleOtherCSRWrite( image, csrno, value );
#define MINIRV32_OTHERCSR_READ( csrno, value ) value = HandleOtherCSRRead( image, csrno );

#include "mini-rv32ima.h"
#include "default64mbdtc.h"

void main(void)
{
    Bdisp_EnableColor(0);
    Bdisp_AllClr_VRAM();

    //Allocate memory for ram
	ram_image = malloc( ram_amt ); //allocate the ram memory
	memset( ram_image, 0, ram_amt ); //clear it

    int dtb_ptr = 0;
restart:
    {
        //Now we need to load our image
        unsigned short pFile[sizeof(IMAGE_PATH)*2]; // Make buffer for name
        Bfile_StrToName_ncpy(pFile, (unsigned char*)IMAGE_PATH, sizeof(IMAGE_PATH)); 
        int hFile;
        hFile = Bfile_OpenFile_OS(pFile, 0, 0); // Get handle, opening it in read mode
        Bfile_ReadFile_OS(hFile, ram_image, Bfile_GetFileSize_OS(hFile), 0);
        Bfile_CloseFile_OS(hFile);

        //Now we load the default dtb
        dtb_ptr = ram_amt - sizeof(default64mbdtb) - sizeof( struct MiniRV32IMAState );
        memcpy( ram_image + dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
    }

	// The core lives at the end of RAM.
	core = (struct MiniRV32IMAState *)(ram_image + ram_amt - sizeof( struct MiniRV32IMAState ));
	core->pc = MINIRV32_RAM_IMAGE_OFFSET;
	core->regs[10] = 0x00; //hart ID
	core->regs[11] = dtb_ptr?(dtb_ptr+MINIRV32_RAM_IMAGE_OFFSET):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	core->extraflags |= 3; // Machine-mode.

    // Update system ram size in DTB (but if and only if we're using the default DTB)
    // Warning - this will need to be updated if the skeleton DTB is ever modified.
    uint32_t * dtb = (uint32_t*)(ram_image + dtb_ptr);
    //if( dtb[0x13c/4] == 0x00c0ff03 )
    {
        uint32_t validram = dtb_ptr;
        dtb[0x13c/4] = (validram>>24) | ((( validram >> 16 ) & 0xff) << 8 ) | (((validram>>8) & 0xff ) << 16 ) | ( ( validram & 0xff) << 24 );
    }
	// Image is loaded.
	int rt;
	long long instct = -1;
	int instrs_per_flip = 128;
    
    char buf[12];


	unsigned int i = 1;
	char *c = (char*)&i;
	if (*c)    
		Print("Little endian");
	else
		Print("Big endian");






	for( rt = 0; rt < instct+1 || instct < 0; rt+=instrs_per_flip)
	{
        //just making time up for now
		int ret = MiniRV32IMAStep( core, ram_image, 0, 2, instrs_per_flip ); // Execute upto 1024 cycles before breaking out.
        itoa(rt, buf);
        locate_OS(1,1);
        Print_OS(buf,0,0);

		switch( ret )
		{
			case 0: break;
			case 1: break; //i cba
			case 3: instct = 0; break;
			case 0x7777: goto restart;	//syscon code for restart
			case 0x5555: return; //syscon code for power-off
			default: break;
		}


        //Render the screen but let you exit
        if(keyPressed(KEY_PRGM_MENU)){
            int key;
            GetKey(&key);
        }
        Bdisp_PutDisp_DD();
	}
}

//MY IO STUFF
static int IsKBHit()
{
    unsigned char buffer[12];
    PRGM_GetKey_OS( buffer );
    return buffer[1] == 0 && buffer[2] == 0;
}
static int ReadKBByte()
{
    int key;
    GetKey(&key);
    return key;
}
static void Print(char* buf)
{
    locate_OS(1,4);
    Print_OS(buf,0,0);
}
static void PrintWarning(const char *format, ...)
{
    char str[80];
    sprinf(str, format);
    Print(str);
}


//STUFF THE EMULATOR CALLS
static uint32_t HandleControlStore( uint32_t addy, uint32_t val )
{
    Print("1");
	if( addy == 0x10000000 ) //UART 8250 / 16550 Data Buffer
	{
		//printf( "%c", val );
		//fflush( stdout );
	}
	return 0;
}

static uint32_t HandleControlLoad( uint32_t addy )
{
    Print("2");
	// Emulating a 8250 / 16550 UART
	if( addy == 0x10000005 )
		return 0x60 | IsKBHit();
	else if( addy == 0x10000000 && IsKBHit() )
		return ReadKBByte();
	return 0;
}


static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value )
{
    Print("3");
	if( csrno == 0x136 )
	{
		//printf( "%d", value ); fflush( stdout );
	}
	if( csrno == 0x137 )
	{
		//printf( "%08x", value ); fflush( stdout );
	}
	else if( csrno == 0x138 )
	{
		//Print "string"
		uint32_t ptrstart = value - MINIRV32_RAM_IMAGE_OFFSET;
		uint32_t ptrend = ptrstart;
		//if( ptrstart >= ram_amt )
		//	printf( "DEBUG PASSED INVALID PTR (%08x)\n", value );
		while( ptrend < ram_amt )
		{
			if( image[ptrend] == 0 ) break;
			ptrend++;
		}
		if( ptrend == ptrstart ) return;

        char buf[ptrend-ptrstart];
        memcpy(buf, (image+ptrstart), ptrend-ptrstart);
        locate_OS(1,1);
        Print_OS(buf,0,0);
		//fwrite( image + ptrstart, ptrend - ptrstart, 1, stdout );
	}
	else if( csrno == 0x139 )
	{
		//putchar( value ); fflush( stdout );
	}
}

static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno )
{
    Print("4");
	if( csrno == 0x140 )
	{
		if( !IsKBHit() ) return -1;
		return ReadKBByte();
	}
	return 0;
}

//used to let you exit
static int keyPressed(int basic_keycode){
    const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
    int row, col, word, bit;
    row = basic_keycode%10;
    col = basic_keycode/10-1;
    word = row>>1;
    bit = col + ((row&1)<<3);
    return (0 != (keyboard_register[word] & 1<<bit));
}