#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <termios.h>

// if press any key return 1, else return 0
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt); // The file descriptor for standard input is 0 (zero); the POSIX <unistd.h> definition is STDIN_FILENO;
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);  // 不要 用標準輸入 + 不要 顯示輸入的character
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // TCSANOW:立即生效
  
  //函數fcntl()可以用來改變文件I/O操作的工作方式
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0); // get file status flags and file access modes
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); // F_SETFL:表示讀取檔案狀態值 O_NONBLOCK:non-blocking I/O
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore old terminal i/o settings 
  fcntl(STDIN_FILENO, F_SETFL, oldf);      // Restore old I/O操作的工作方式
 
  if(ch != EOF)        // press something 
  {
    ungetc(ch, stdin); // rollback
    return 1;
  }
 
  return 0;
}


int main(int argc, char* argv[])
{

	int fbfd = 0;
	unsigned char *fbp;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize = 0;
	fbfd = open("/dev/fb0", O_RDWR); /* open the device file, /dev/fb0 */
	/*取得顯示螢幕相關參數*/
	ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
	ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
	/*計算顯示螢幕緩衝區大小*/
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	/*映射螢幕緩衝區到用戶位址空間*/
	fbp=(unsigned char*)mmap(0, screensize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd,0);
	/*下面可透過 fbp pointer 讀寫緩衝區*/

	// try to draw a rectangle
	int x=0, y=0;
	long int location = 0;
 	
// with format 16bit 5-6-5RGB :rrrrrggggggbbbbb
while (1)
{
	

	for(y=0;y<vinfo.yres;y++)
	{
		for(x=0;x<vinfo.xres;x++){
		
			int r = 0, g= 0, b=0;
			unsigned short int t;  // 16 bit
			// bits_per_pixel = 每個pixel用多少bit表示
			// xoffset, yoffset: offset from virtual to visible resolution.
			// line_length: length of a line in byte
			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y + vinfo.yoffset) * finfo.line_length;
			
			if (y< vinfo.yres / 3)
			{
				b = 0; g = 255 ; r = 0;
			}else if (y >= vinfo.yres / 3 && y< 2* vinfo.yres/3)
			{
				b = 0; g = 0 ;r = 255;
			}else
			{
				b = 255;g =0; r=0;			
			}
			t = r <<11 | g<<5 | b;
			*((unsigned short int*)(fbp + location)) = t;
		}
	}

	sleep(2);
	if ( kbhit() )
		break;		

	for(y=0;y<vinfo.yres;y++)
	{
		for(x=0;x<vinfo.xres;x++){
		
			int r = 0, g= 0, b=0;
			unsigned short int t;
			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y + vinfo.yoffset) * finfo.line_length;
			
			if (y< vinfo.yres / 3)
			{
				b = 255; g = 0 ; r = 0;
			}else if (y >= vinfo.yres / 3 && y< 2* vinfo.yres/3)
			{
				b = 0; g = 255 ;r = 0;
			}else
			{
				b = 0;g =0; r=255;			
			}
			t = r <<11 | g<<5 | b;
			*((unsigned short int*)(fbp + location)) = t;
		}
	}
	sleep(2);
	if ( kbhit() )
		break;		


	for(y=0;y<vinfo.yres;y++)
	{
		for(x=0;x<vinfo.xres;x++){
		
			int r = 0, g= 0, b=0;
			unsigned short int t;
			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y + vinfo.yoffset) * finfo.line_length;
			
			if (y< vinfo.yres / 3)
			{
				b = 0 ; g = 0 ; r = x % 32;
			}else if (y >= vinfo.yres / 3 && y< 2* vinfo.yres/3)
			{
				b = 0; g = x % 64 ;r = 0;
			}else
			{
				b = x % 32 ; g =0 ; r=0;			
			}
			t = r <<11 | g<<5 | b;
			*((unsigned short int*)(fbp + location)) = t;
		}
	}
	sleep(2);
	if ( kbhit() )
		break;		

	
}


// release memory		
	munmap(fbp, screensize);
	close(fbfd);
	return 0;
}



