//#########################################################################
// 
//       Puzzle Bobble  (aka Bust-a-Move)
//                Version 0.3
//      
//
//          Main Program
//   
//      Created 26/05/2002; 11:07:44
//
//           By David Coz  
//
//       Coz.hubert@infonie.fr
//
//  You must add extgraph.h and extgraph.a in your project
//
//##########################################################################




#define USE_TI89              // Produce .89z File

#define OPTIMIZE_ROM_CALLS    // Use ROM Call Optimization

#define NO_EXIT_SUPPORT

#include <tigcclib.h>         // Include All Header Files
#include "sprites_bob.h"        
#include "trig_tables.h"  
#include "sprite_test.h"  
#include "extgraph.h"  

#define ESC 264
#define BALL_SPEED 30
#define RIGHT_BORD 104
#define LEFT_BORD 15

// Array to store the bubbles on the screen
// 12 rows,15 columns
// ex : 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
//      0 1 0 1 0 1 0 1 0 1 0 1 0 1 0
//      1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
//  ..................................

char storage[12][15];

// Virtual Screens
void *light;
void *dark;

// Stores the bubble coulours that are present on the screen. 
char allowed_color[7];

int game_over;
int shots;
int bubble;
long int score;



//  Clock Definition ,thanx to Thomas Nussbaumer.
volatile long int mseconds = 0;
volatile long int mseconds50 = 0; 
volatile int seconds    = 0; 
volatile int minutes    = 0; 
volatile int resettime  = 1; 
volatile int running  =1;


INT_HANDLER oldint5 = NULL;
DEFINE_INT_HANDLER (myint5handler) {
       if (resettime) {
        seconds=minutes=resettime=0;
    }
    else if (running){
 mseconds++;
//mseconds50=mseconds>>1;
 mseconds50++;
//mseconds50=mseconds50>>1;
        if (mseconds==19) {
         //mseconds = 0;
            seconds++;
            if (seconds == 60) {
                seconds = 0;
                minutes++;
              }

        }
    }
    ExecuteHandler(oldint5);
}


// Little function,calculate n^exp (used for the score)
long int power(int n,int exp)
{
	int i;
	if (exp==0)
	return 1;
  int p=n; 	
	for (i=1;i<exp;i++)
	p=p*n;
	
	return p;
}


// Waiting Function
void wait(unsigned short int w)
{
OSFreeTimer(USER_TIMER);
	OSRegisterTimer(USER_TIMER,w);
	
	 while(!OSTimerExpired(USER_TIMER));
	
}

// Structure for the Bubbles
typedef struct
{
int  on; // '1' if a bubble is 'moving','0' else
// Bubble coord
 long	 x; 
 long	y;
// Bubble direction
	long dx;
	long dy;
// Bubble number (for sprite)
	int num;
} BALL;
BALL ball;

// Structure for the bubble erasing/falling
typedef struct
{
// Number of bubbles to erase
	int erasing;
// Bubbles coord
	int *ball_x;
	int *ball_y;
// Bubble sprites
  int *spr;
  int flag;
  long int *time;
} ERASE;

ERASE erasing;
ERASE falling;



// Some Stuff to display the cursor

int center_x=13<<2,center_y=6<<2;
int pt[8][2]={{13<<2,13<<2},{13<<2,-6<<2},{13<<2,-6<<2},{1<<4,10<<4},
{8<<4,10<<4},{0<<4,6<<4},{8<<4,6<<4},{5<<4,(-5)<<4}};

int norm[8];//={5<<4,45,45,115,115,86,86};
int angle[8];//={64,160,224,168,216,143,240};
int line[6][2]={{0,1},{2,2},{1,4},{2,3},{3,5},{4,6}};

unsigned char cursor_angle;
int rotate_angle=1;  
unsigned char old_angle=0<<7;
long int frame;
long int key_repeat;

int old_num;
int next_num;
int score_refresh;

// Variable, 0 or 1
// 1 means the left bubble on the top is attached to the wall
int top_offset=1;

void calc()
{
int x,y,i;
	for (i=0;i<3;i++)
	{
	x=pt[i][0]-center_x;
	y=center_y-pt[i][1];
	norm[i]=sqrt(x*x+y*y);
	angle[i]=128*atan2(x,y)/3.14159;	
	angle[i]+=64;
	if (angle[i]<0)
	angle[i]=256+angle[i];
	//printf_xy(50,50,"%d  :%d  ",norm[i],angle[i]);
	//ngetchx();
	}
	
}


// Draw stuff on the Screen
void draw_bubbles()
{
FontSetSys(F_6x8);
char buf[50];
sprintf(buf,"%04d",bubble);
DrawGrayStrExt2B(115,7,"Bubbles",A_NORMAL|A_SHADOWED,F_6x8,light,dark);
DrawGrayStrExt2B(120,17,buf,A_REPLACE|A_SHADOWED,F_6x8,light,dark);
sprintf(buf,"%06ld",score);
DrawGrayStrExt2B(117,37,"Score",A_NORMAL|A_SHADOWED,F_6x8,light,dark);
DrawGrayStrExt2B(117,47,buf,A_REPLACE|A_SHADOWED,F_6x8,light,dark);
DrawGrayStrExt2B(117,67,"Next",A_NORMAL|A_SHADOWED,F_6x8,light,dark);
}

//Draw the Cursor depending on the Cursor_angle
void draw_cursor(unsigned char ang,int att,unsigned char* plane)
{
unsigned char i,an;
int p[7][2];
	for (i=0;i<3;i++)
{
an=angle[i]+ang;
p[i][0]=(center_x+((norm[i]*cos255[an])>>7))>>2;
p[i][1]=(center_y-((norm[i]*sin255[an])>>7))>>2;
}

for (i=0;i<1;i++)
FastDrawLine(plane,LEFT_BORD+35+p[line[i][0]][0],86+p[line[i][0]][1]
,LEFT_BORD+35+p[line[i][1]][0],86+p[line[i][1]][1],att);
}



// Draw the two Walls
void draw_border()
{
	int i;
	for (i=0;i<4;i++)
	{
	GraySprite8_XOR(LEFT_BORD-7,i*26,26,border,border+26,light,dark);
	GraySprite8_XOR(RIGHT_BORD,i*26,26,border,border+26,light,dark);
	}
}


// Important routine
// Check if some bubbles are not 'attached',to make them falling
void fall()
{
// Contain the coordinates of the surrounding bubbles
// viewing from the bubble '0'   
//    1   2
//  6   0   3
//    5   4

int next_x[6]={-1,1,2,1,-1,-2};
int next_y[6]={-1,-1,0,1,1,0};

// Array,like 'storage'
// In fact,we will start from the top
// the top bubbles are set to '1' in 'check'
// and also all the bubbles attached to them
// Then,if a bubble has not its value egal to '1'
// in 'check',it must fall

int check[12][15];

int store_x[200];
int store_y[200];

int xx,yy,xxx,yyy,i,j,k;
int count=1; // total of bubbles we have to check the neighbourhood
int current=1;// number of the current bubble we are checking

store_x[0]=0;
store_y[0]=0;

// init check to 0
for (i=0;i<15;i++)
for (j=0;j<12;j++)
check[j][i]=0;


for (k=1-top_offset;k<15;k+=2)
{
if (storage[0][k]==0)
continue;
//Set the Top Bubbles to '1' in 'check'
check[0][k]=1;

// xx,yy => coordinates of the bubbles we're working with
	xx=k;
	yy=0;	
	
		while(1)
		{
		 // Check the 6 surrounding bubbles
			for (i=0;i<6;i++)
			{
		  	// xxx,yyy => coordinates of one of the 6 'possible' 
		  	// surrounding bubbles
		  	xxx=next_x[i]+xx;
			  yyy=next_y[i]+yy;
			  
	     // if this bubble is out
	     //or if we have already checked it 
	     //or if there is no bubble here
	     //don't go on 
				if (xxx<0||xxx>14||yyy<0||yyy>11||check[yyy][xxx]!=0||storage[yyy][xxx]==0)
				continue;
	
        // this bubble is attached,is 'check' value egal '1'	
				check[yyy][xxx]=1;
				
				// We have to check another bubble later
				// Store its coord. ,increase 'count'
				store_x[count]=xxx;
				store_y[count]=yyy;
				count++;
			}
			// if we have checked all the bubbles
			// break
			if (count==current)
			break;
      // else store in (xx,yy) the coord. of the next bubble to check
      // increase current		
 			xx=store_x[current];
			yy=store_y[current];
		  current++;
		}	

}

// Now test if a bubble hasn't been checked
// In this case it must be registered in 'falling'

falling.erasing=0;
falling.flag=0;
long int ms=mseconds50;
// For all the array
for (i=0;i<15;i++)
for (j=11;j>-1;j--)
{
// if a bubble hasn't been checked
	if (storage[j][i]!=0&&check[j][i]==0)
	{
   // Save the bubble sprite in 'falling.spr'
    falling.spr[falling.erasing]=storage[j][i];
   // The ball no longer exists in 'Storage'
  	storage[j][i]=0;
  	
	  falling.time[falling.erasing]=mseconds50+falling.erasing*10;/*econds50;;*/
   
   // Save the bubble coord in 'falling.spr'
    falling.ball_x[falling.erasing]=i;
    falling.ball_y[falling.erasing]=j;
    // Increase the number of falling bubbles
    falling.erasing++; 
	  // Increase the 'bubble' hit score
	  bubble++;
	score_refresh=1;
		}
}	
	
}


// Important routine
// Check if you make three (or more) alike bubbles together
int three(int x,int y)
{
int bub=bubble;
int num=ball.num;
// Contain the coordinates of the surrounding bubbles
// viewing from the bubble '0'   
//    1   2
//  6   0   3
//    5   4

int next_x[6]={-1,1,2,1,-1,-2};
int next_y[6]={-1,-1,0,1,1,0};

// Array,like storage
// use for seeing if we have previously tested
// a bubble position
// if check[x][y]=0 => we haven't testest this bubble position
// if check[x][y]=1 => we have already testest this bubble position
// if check[x][y]=2 => this bubble position contains a bubble with the good 
//                     color

int check[12][15];

int store_x[200];
int store_y[200];

int xx=x,yy=y,xxx,yyy,i,j;
int count=1;     // total of bubbles we have to check the neighbourhood
int current=1;  // number of the current bubble we are checking

store_x[0]=x;
store_y[0]=y;

// Init 'check'
for (i=0;i<15;i++)
for (j=0;j<12;j++)
check[j][i]=0;


check[y][x]=2;


while(1)
{
// xx,yy => coordinates of the bubbles we're working with

 // Check the 6 surrounding bubbles
	for (i=0;i<6;i++)
	{
	// xxx,yyy => coordinates of one of the 6 'possible' 
	// surrounding bubbles
  	xxx=next_x[i]+xx;
	  yyy=next_y[i]+yy;
	  	  
	     // if this bubble is out
	     //or if we have already checked it 
	     //don't go on 
		
		if (xxx<0||xxx>14||yyy<0||yyy>11||check[yyy][xxx]!=0)
		continue;
		// We have checked this bubble position
		check[yyy][xxx]=1;
		
		// If this bubble hasn't the same color as the one we want
		// don't continue
		if (storage[yyy][xxx]!=num+1)
		continue;
		// this bubble has a 'good' color 
		check[yyy][xxx]=2;
		
	 // We have to check this bubble later (i.e  its neighbourhood)
	 // Store its coord. ,increase 'count'
		store_x[count]=xxx;
		store_y[count]=yyy;
		count++;
	}
	// if we have checked all the bubbles
	// break
	if (count==current)
	break;
	
// else store in (xx,yy) the coord. of the next bubble to check
// increase current		

	xx=store_x[current];
	yy=store_y[current];
  current++;
}	

// if we tested more than two bubble neighbourhood
// that means there are more than 2 alike bubbles touching
// we must erase them 

if (count>2)
{
// Increase the 'bubble' hit score
bubble+=count;
score_refresh=1;
// Refresh the screen,
draw_bubbles();

erasing.flag=0;
erasing.erasing=count;

// the 'count' bubbles must be erased...(registered in 'erasing')
  		for (i=0;i<count;i++)
	  {
	   	
	   	yy=store_y[i];
	  	xx=store_x[i];
      // Save the bubble sprite in 'erasing.spr'
      erasing.spr[i]=storage[yy][xx];
	   // The ball no longer exists in 'Storage'
	   storage[yy][xx]=0;
    // Save the bubble coord in 'erasing.spr'
      erasing.ball_x[i]=xx;
      erasing.ball_y[i]=yy;
    }
// As we erased some bubbles,we must
// check if bubbles are no longer 'attached',to make them falling
fall();
}

// Increase Score
if (bub!=bubble)
score+=30*power(2,bubble-bub-3);

return count;	
}


// The bubble has been stopped 
//we must find its position in the chain
// the bubble must 'fit' in the chain 
void search_pos()
{

int x=ball.x>>4,y=ball.y>>4;
	int pos_x,pos_y,offset,xx;

// if you made more 10 shots
// the screen must fall down
if (shots>10)
{
	int i,j;
	// scroll down the storage array
	for (i=10;i>0;i--)
	for (j=0;j<15;j++)
	storage[i][j]=storage[i-1][j];
	// init a new bubble row
	for (i=0;i<15;i+=2)
	{
		storage[0][i+top_offset]=random(7)+1;
		storage[0][i+1-top_offset]=0;
	}
	// Init 'shot'
	shots=0;
	top_offset=1-top_offset;
// Refresh the screen
	refresh(0);
	y+=8;
}

// (x,y) => coordinates of the stopped bubble 

pos_y=(y)/8+(y%8)/4;
offset=pos_y%2;
if (top_offset==0)
offset=1-offset;
xx=x-LEFT_BORD-1-5*offset;

// (pos_x,pos_y) => its position in the 'storage' array
//pos_x=(2*xx)/11+(2*(xx%11))/11+offset;
pos_x=xx/11+(2*(xx%11))/11;
pos_x=2*pos_x+offset;
//pos_x=xx/11+(xx%11)
if (pos_x==15)
pos_x-=2;

// Update the Bubble coord.,now it fits in the Chain 
storage[pos_y][pos_x]=ball.num+1;
ball.y=(pos_y*8)<<4;
xx=(pos_x*11)>>1;
ball.x=(LEFT_BORD+1+xx)<<4;

// Check if you make three (or more) alike bubbles together
three(pos_x,pos_y);

// Refresh the screen
refresh(0);

	key_repeat=mseconds50;
 erasing.time[0]=mseconds50;

 GraySprite32_XOR(LEFT_BORD+43,88,10,sprites[old_num],
sprites[old_num]+10,light,dark);

}


// Function that draws the bubble when it 's moving
void draw_ball()
{
//Erase it
GraySprite32_XOR(ball.x>>4,ball.y>>4,10,sprites[ball.num],
sprites[ball.num]+10,light,dark);

// Increase ball.y
ball.y+=ball.dy;

// if we touched the top , ball.on=0
if (ball.y<0)
{
ball.on=0;
ball.y-=ball.dy;
}
// if we touched another bubble, ball.on=0
	if (test_sprite(ball.x>>4,ball.y>>4,sprites[ball.num],10,dark))
		{
				ball.on=0;
				ball.y-=ball.dy;
		}

// Increase ball.x
  ball.x+=ball.dx;

// if we touch one of the walls,change ball direction  
  if ((ball.x>>4)<LEFT_BORD+1||(ball.x>>4)>RIGHT_BORD-11)
  {
  ball.dx=-ball.dx;	
  ball.x+=ball.dx;	
  score+=50;
  score_refresh=1;
  }
  
// if we touched another bubble, ball.on=0 
if (test_sprite(ball.x>>4,ball.y>>4,sprites[ball.num],10,dark))
		{
				ball.on=0;
				ball.x-=ball.dx;
		 score+=0;
		}
// If the bubble has been stooped
// store it in 'storage' and find it fitting position in the chain 		
if (ball.on==0)
search_pos();

if (ball.on!=0)
GraySprite32_XOR(ball.x>>4,ball.y>>4,10,sprites[ball.num],
sprites[ball.num]+10,light,dark);
	
}

// You pressed 'fire',generate a new bubble
void new_ball()
{
// Erase the 'next' bubble
GraySprite32_XOR(123,80,10,sprites[next_num],
sprites[next_num]+10,light,dark);

// Erase the 'current' bubble (next the cursor)
GraySprite32_XOR(LEFT_BORD+43,88,10,sprites[old_num],
sprites[old_num]+10,light,dark);


ball.num=old_num;
old_num=next_num;

// generate a random color
while(1)  
{
  next_num=random(7);
if (allowed_color[next_num])
break;
}
// Draw the 'next' bubble
GraySprite32_XOR(123,80,10,sprites[next_num],
sprites[next_num]+10,light,dark);
  

	ball.on=1;
	
unsigned char an2=cursor_angle+angle[2];

// Init the bubble coord.	
	ball.x=(LEFT_BORD+43)<<4;
	ball.y=88<<4;
// Init the bubble direction,depending on cursor_angle	
	ball.dx=-(BALL_SPEED*sin255[cursor_angle])>>7;
	ball.dy=-(BALL_SPEED*cos255[cursor_angle])>>7;

GraySprite32_XOR(ball.x>>4,ball.y>>4,10,sprites[ball.num],
sprites[ball.num]+10,light,dark);

// We made another shot
shots++;

}



// Read the Keys
int key()
{
// you pressed ESC
if 	(_rowread(~((short)(1<<6))) & (1<<0))
return ESC;


if(_rowread(0x7D)&0x40)//Clear
off();

// You pressed Left
if (_rowread(0x7E)&0x08&&(cursor_angle<63||cursor_angle>197))
cursor_angle-=rotate_angle;

// You pressed Right
if (_rowread(0x7E)&0x02&&(cursor_angle<60||cursor_angle>194))
cursor_angle+=rotate_angle;

// You press 2nd

if 	(_rowread(~((short)(1<<0))) & (1<<4)&&ball.on==0
 &&erasing.erasing==0&&falling.erasing==0&&mseconds50-key_repeat>50)
{
// if there is no bubbles running 
// if the erasing/falling has been made
// if the key_repetition delay has passed 
// then generate a new bubble
	new_ball();
}


	old_angle=cursor_angle;
	return 0;
}


// Erase the Bubbles stored in 'erasing'
// (3 or more bubbles 'alike')
void erase_ball()
{
	int i,x,y,xx,sprt,flag=0,mask=(mseconds50-erasing.time[0])>>4;
mask=(mask*3)>>2;
if (mask>3)
{
	erasing.erasing=0;
	refresh(0);
return;	
}
if (erasing.flag)
flag=1;

// For all the bubbles to erase
	for (i=0;i<erasing.erasing;i++) 
	{
		 	// (x,y) => coord of the bubble to erase
		 	y=(erasing.ball_y[i]*8);
		  xx=(erasing.ball_x[i]*11)>>1;
		  x=(LEFT_BORD+1+xx);
		  
		  sprt=erasing.spr[i]-1;		
    // 	if (flag==0)
		  //  {
		   	GraySprite32_OR(x,y,10,sprites[sprt],sprites[sprt]+10,light,dark);
	     	//erasing.flag=1;
		   //  }
		// add a mask on the bubble
		// to simulate a progressive erasing
		GraySprite16_AND(x,y,10,spe_mask[mask],spe_mask[mask],light,dark);
	 }
GraySprite32_OR(LEFT_BORD+43,88,10,sprites[old_num],
sprites[old_num]+10,light,dark);
}

// Make the bubbles stored in 'falling' fall down
void fall_ball()
{
	int i,x,y,xx,sprt,count=0,dy;/*=(mseconds50-falling.time)>>2;

// Y coordinates depending on the time passing
// to simulate a 'fall down'
dy+=((dy>>1)*(dy>>1));
dy=(dy>>2)-(dy>>3);
*/

// For all the bubbles to fall down
	for (i=0;i<falling.erasing;i++)
	{
	dy=(mseconds50-falling.time[i])>>2;
   if (dy<0)
	dy=0;
  dy+=((dy>>1)*(dy>>1));
  dy=(dy>>2)-(dy>>3);
	 
	
		// (x,y) => coord of the bubble falling down
		y=(falling.ball_y[i]*8);
    xx=(falling.ball_x[i]*11)>>1;
    x=(LEFT_BORD+1+xx);
    sprt=falling.spr[i]-1;		
   
   // if the bubble reaches the bottom of the screen 
		if (y+dy>90)
		{
			count++;
			continue;
		}
    // Draw the bubble in its new position
		GraySprite16_AND(x,y+dy,10,bob_mask,bob_mask,GetPlane(0),GetPlane(1));
		GraySprite32_OR(x,y+dy,10,sprites[sprt],sprites[sprt]+10,GetPlane(0),GetPlane(1));
	}

if (count==falling.erasing)
{
 falling.erasing=0;
 refresh(0);
}

}

void gameover()
{
refresh(1);
//FastCopyScreen(light,GetPlane(0));
//FastCopyScreen(dark,GetPlane(1));
while (!(_rowread(~((short)(1<<6))) & (1<<0)));

}

// 'Heart' of bubble
int display()
{
score_refresh=0;
if (game_over)
{
gameover();
//wait(500);
return 	ESC;
}

int i,j,speed1=3,result=0;

// maintain  constant frame rate
if (mseconds50-frame<speed1)
return 0;

frame=mseconds50;



// Draw the moving bubble if there is one
if (ball.on)
draw_ball();

// test the keys
result=key();

// Erase bubbles if necessary
if (erasing.erasing!=0)
erase_ball();

// Display score,'next' bubble,etc...
if (score_refresh)
draw_bubbles();



// Copy Virtual Screen in 'Real' Screens...

FastCopyScreen(light,GetPlane(0));
FastCopyScreen(dark,GetPlane(1));
draw_cursor(cursor_angle,A_NORMAL,GetPlane(1));

// Make bubbles fall downif necessary
if (falling.erasing!=0)
fall_ball();
return  result;
}


// Refresh the bubble Drawing
void refresh(int mo)
{
game_over=0;
int x,xx,y,i,j,o=top_offset,l;

for (i=0;i<7;i++)
allowed_color[i]=0;

DrawGrayRect2B(LEFT_BORD+1,0,RIGHT_BORD-1,87,COLOR_WHITE,RECT_FILLED,light,dark);

for (j=0;j<12;j++)
{
o=1-o;
for (i=o;i<15;i+=2)
{
if (storage[j][i]==0)
continue;
if (j==10)
game_over=1;
allowed_color[storage[j][i]-1]=1;
	  	y=(j*8);
      xx=(i*11)>>1;
      x=(LEFT_BORD+1+xx);
      
      GraySprite16_AND(x,y,10,bob_mask,bob_mask,light,dark);
   
      GraySprite32_XOR(x,y,10,sprites[storage[j][i]-1],
sprites[storage[j][i]-1]+10,light,dark);
if (mo)
{
GraySprite32_OR(x,y,10,sprites[storage[j][i]-1]+10,
sprites[storage[j][i]-1],GetPlane(0),GetPlane(1));
wait(4);
}
}

}
FastDrawGrayHLine2B(LEFT_BORD+1,RIGHT_BORD-1,82,COLOR_LIGHTGRAY,light,dark); // ported: ExtGraph 2 changed the GrayColors values, never hard-code '2'
}

// Init the 'Storage' Array
void init()
{
int i,j,o=top_offset;
for (j=0;j<5;j++)
{
o=o-1;
for (i=o;i<15;i+=2)
 	storage[j][i]=random(7)+1;

}
refresh(0);

}

// Main Function
void _main(void)
{
 LCD_BUFFER screen;
 LCD_save(screen);

FontSetSys(F_4x6);
DrawStr(0,95,"By David Coz    Coz.hubert@infonie.fr",A_REPLACE);
ngetchx();

// Init
bubble=0;
shots=0;
score=0;
top_offset=1;
cursor_angle=0;
int i,j;
for (i=0;i<15;i++)
for (j=0;j<12;j++)
storage[j][i]=0;


// Memory Allocation
light=malloc(3840);
dark=malloc(3840);
erasing.ball_x=calloc(70,sizeof(*erasing.ball_x));
erasing.ball_y=calloc(70,sizeof(*erasing.ball_y));
erasing.spr=calloc(70,sizeof(*erasing.spr));
erasing.time=calloc(1,sizeof(*erasing.time));

erasing.erasing=0;

falling.ball_x=calloc(70,sizeof(*falling.ball_x));
falling.ball_y=calloc(70,sizeof(*falling.ball_y));
falling.spr=calloc(70,sizeof(*falling.spr));
falling.time=calloc(70,sizeof(*falling.time));

falling.erasing=0;


ClearGrayScreen2B(light,dark);

randomize();
next_num=random(7);
old_num=random(7);


// Speed Up Timers
unsigned char old_start=PRG_getStart();   // ported: save the real value (see teardown)
pokeIO(0x600017,0xFC);
ball.on=0;
// Disable Auto 1 and load our Clock
INT_HANDLER save_int_1;
save_int_1 = GetIntVec (AUTO_INT_1);
SetIntVec (AUTO_INT_1, DUMMY_HANDLER);
oldint5 = GetIntVec(AUTO_INT_5);
SetIntVec(AUTO_INT_5, myint5handler);

calc();
resettime=1;
running=1;
frame=mseconds50;
key_repeat=mseconds50;

ClrScr();
GrayOn();

GraySprite32_XOR(LEFT_BORD+43,88,10,sprites[old_num],
sprites[old_num]+10,light,dark);

GraySprite32_XOR(123,80,10,sprites[next_num],
sprites[next_num]+10,light,dark);

// Draw the Walls
draw_border();

init();
// Draw the score,'next'
draw_bubbles();

// Main Loop
while(1)
{
if (display()==ESC)
break;
}
// Free Allocated Memory
free(light);
free(dark);
free(erasing.ball_x);
free(erasing.ball_y);
free(erasing.time);
free(erasing.spr);

free(falling.ball_x);
free(falling.ball_y);
free(falling.spr);
free(falling.time);
GrayOff();

// Restore 'Normal' timers Speed
PRG_setStart(old_start);   // ported: the HW table restored 0xB2 on the Titanium (13 Hz instead of AMS's 0xCC)

// Restore Old Timers
SetIntVec (AUTO_INT_1, save_int_1);
  SetIntVec(AUTO_INT_5, oldint5);
   LCD_restore(screen);
  ST_showHelp(EXTGRAPH_VERSION_PWDSTR); 
}

// By David Coz     Coz.hubert@infonie.fr
