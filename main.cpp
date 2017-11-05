#include "Pokitto.h"
#include "gfx.h"
#include "music.h"

Pokitto::Core game;
#define SOUND_PLAUNCH 0
#define SOUND_ELAUNCH 1
#define SOUND_DETONATE 2
#define SOUND_SCORE 3
#define SOUND_DEAD 4
#define SOUND_LOSE 5

/**************************************************************************/
#define HELD 0
#define NEW 1
#define RELEASE 2
byte CompletePad, ExPad, TempPad, myPad;
bool _A[3], _B[3], _C[3], _Up[3], _Down[3], _Left[3], _Right[3];

DigitalIn _aPin(P1_9);
DigitalIn _bPin(P1_4);
DigitalIn _cPin(P1_10);
DigitalIn _upPin(P1_13);
DigitalIn _downPin(P1_3);
DigitalIn _leftPin(P1_25);
DigitalIn _rightPin(P1_7);

void UPDATEPAD(int pad, int var) {
  _C[pad] = (var >> 1)&1;
  _B[pad] = (var >> 2)&1;
  _A[pad] = (var >> 3)&1;
  _Down[pad] = (var >> 4)&1;
  _Left[pad] = (var >> 5)&1;
  _Right[pad] = (var >> 6)&1;
  _Up[pad] = (var >> 7)&1;
}

void UpdatePad(int joy_code){
  ExPad = CompletePad;
  CompletePad = joy_code;
  UPDATEPAD(HELD, CompletePad); // held
  UPDATEPAD(RELEASE, (ExPad & (~CompletePad))); // released
  UPDATEPAD(NEW, (CompletePad & (~ExPad))); // newpress
}

byte updateButtons(byte var){
   var = 0;
   if (_cPin) var |= (1<<1);
   if (_bPin) var |= (1<<2);
   if (_aPin) var |= (1<<3); // P1_9 = A
   if (_downPin) var |= (1<<4);
   if (_leftPin) var |= (1<<5);
   if (_rightPin) var |= (1<<6);
   if (_upPin) var |= (1<<7);

   return var;
}

/**************************************************************************/



void drawFont(int x1, int y1, int wide, int high, int tile, int transparentColor, const uint8_t *gfx, int palReplace = -1){
    uint16_t tileBuffer[(wide+1)*(high+1)];

    uint8_t pix;
    uint8_t quartWide=wide/4;
    uint8_t pic;

    for(int y=0; y<high; y++){
        for(int x=0; x<quartWide; x++){
            pic = gfx[(tile*quartWide*high)+(x+quartWide*y)];
            pix = (pic >> 6)&3; if(pix != transparentColor) game.display.screenbuffer[(  x1+(x*4))*88+(y1+y)]=pix;
            pix = (pic >> 4)&3; if(pix != transparentColor) game.display.screenbuffer[(1+x1+(x*4))*88+(y1+y)]=pix;
            pix = (pic >> 2)&3; if(pix != transparentColor) game.display.screenbuffer[(2+x1+(x*4))*88+(y1+y)]=pix;
            pix = pic &3;       if(pix != transparentColor) game.display.screenbuffer[(3+x1+(x*4))*88+(y1+y)]=pix;
        }
    }
}

// print text
void print(uint16_t x, uint16_t y, const char* text, uint8_t bgCol, signed int repCol=-1){
    if(repCol==-1)repCol=bgCol;
    for(uint8_t t = 0; t < strlen(text); t++){
        uint8_t character = text[t]-32;
        drawFont(x, y, 8, 8, character, bgCol, myFont, repCol);
        x += 8;
    }
}



unsigned char* ReadBMP(char* filename){
    unsigned char col[4]; // for reading palette
    unsigned short palette[256];
	FILE* testRead = fopen (filename, "rb"); //rb = read
	if(testRead){
		fseek(testRead, 54, SEEK_SET); // find palette
		for(int img_temp=0; img_temp<256; img_temp++){ // assume 8bit
			fread(col, sizeof(char), 4, testRead);
			col[0]>>3; // b
			col[1]>>2; // g
			col[2]>>3; // r
			palette[img_temp] = (col[0] << 11) | (col[1] << 5) | col[1];
		}
        game.display.load565Palette(palette);
        fread(game.display.screenbuffer, 1, 110*88, testRead);
		fclose(testRead);
	}else{
        print(0, 0, "No Open File.",0,127);
	}
}





unsigned short pal[256];
int PntClr(int x, int y){
	return game.display.screenbuffer[y*game.display.width+x];
}
void Dot (int x, int y, int c){
	game.display.screenbuffer[y*game.display.width+x]=c;
}
int RandMinMax(int min, int max){
    return rand() % max + min;
}
int Adjust (int xa, int ya, int x, int y, int xb, int yb){
	if(PntClr(x, y) != 0) return 0;
	int q = abs(xa - xb) + abs(ya - yb);
	int v = (PntClr(xa, ya) + PntClr(xb, yb)) / 2 + (RandMinMax(0,q*10)) / 10;
	if (v < 1) v = 1;
	if (v > 255) v = 255;
	Dot(x, y, v);
	return 1;
}
void SubDivide (int x1, int y1, int x2, int y2){
	if ((x2 - x1 < 2) && (y2 - y1 < 2)) return;
	int x = (x1 + x2) / 2;
	int y = (y1 + y2) / 2;
	Adjust(x1, y1, x, y1, x2, y1);
	Adjust(x1, y2, x, y2, x2, y2);
	Adjust(x2, y1, x2, y, x2, y2);
	Adjust(x1, y1, x1, y, x1, y2);
	if(PntClr(x, y) == 0)	{
		int v = PntClr(x1, y1) + PntClr(x2, y1) + PntClr(x2, y2);
		v = v + PntClr(x1, y2) + PntClr(x1, y) + PntClr(x, y1);
		v = v + PntClr(x2, y) + PntClr(x, y2);
		v = v / 8;
		Dot(x, y, v);
	}
	SubDivide(x1, y1, x, y);
	SubDivide(x, y, x2, y2);
	SubDivide(x, y1, x2, y);
	SubDivide(x1, y, x, y2);
}
void make_plasma(int x1=0,int y1=0,int x2=game.display.width-1,int y2=game.display.height-1){
	if(x1<0)x1=0;
	if(y1<0)y1=0;
	if(x2>game.display.width-1)x2=game.display.width-1;
	if(y2>game.display.height-1)y2=game.display.height-1;

	Dot(x1, y1, RandMinMax(0,255) + 1);
	Dot(x2, y1, RandMinMax(0,255) + 1);
	Dot(x2, y2, RandMinMax(0,255) + 1);
	Dot(x1, y2, RandMinMax(0,255) + 1);
	SubDivide(x1, y1, x2, y2);
}
void make_pal(void){
	int a,s,r,g,b;
	for(a=0; a<=63; a++){
		s = 0; 	r = a; 		g = 63-a;	b = 0;		pal[a+s] = game.display.RGBto565(r*4,g*4,b*4);
		s = 64; r = 63-a;	g = 0;		b = a; 		pal[a+s] = game.display.RGBto565(r*4,g*4,b*4);
		s = 128; r = 0;	 	g = 0;		b = 63-a;	pal[a+s] = game.display.RGBto565(r*4,g*4,b*4);
		s = 192; r = 0;		g = a;		b = 0;	 	pal[a+s] = game.display.RGBto565(r*4,g*4,b*4);
	}
    game.display.load565Palette(&pal[0]);
}



void setup(){
    game.begin();
    game.display.width = 110; // half size
    game.display.height = 88;
    game.display.persistence=1;
}


// Wave, pitch, duration, arpeggio step duration, arpeggio step size
const int8_t sounds[][5] = {
  { 0, 20, 5, 1, 1 }, //Player launch
  { 0, 25, 5, 1, -1 }, //Enemy launch
  { 1, 10, 5, 1, -1 }, //Detonating enemy missile
  { 1, 10, 2, 0, 0 }, //Score pips
  { 1, 2, 10, 1, -1 }, //A city dies
  { 0, 20, 14, 3, -1 }, //Lose
};

void initSound(){
  game.sound.command(CMD_VOLUME, 5, 0, 0);
  game.sound.command(CMD_SLIDE, 0, 0, 0);
}

void playSound(uint8_t i){
  game.sound.command(CMD_VOLUME, 5, 0, 0);
  game.sound.command(CMD_SLIDE, 0, 0, 0);
  game.sound.command(CMD_ARPEGGIO, sounds[i][3], sounds[i][4], 0);
  game.sound.command(CMD_INSTRUMENT, sounds[i][0], 0, 0);
  game.sound.playNote(sounds[i][1], sounds[i][2], 0);

  //gb.sound.playNote(20, 5, 0);
}

int main(){
    setup();
    initSound();

	srand(game.getTime());
	make_pal();
    make_plasma(0,16,game.display.width-1,game.display.height-1);
    char col=0;

    playSound(SOUND_PLAUNCH);

    int FPS = 0;
    int frameNumber=0;
    long tempTime=game.getTime();
    int myFPS=20;
    game.setFrameRate(myFPS);
    char tempText[10];

    while (game.isRunning()) {
        if(game.update()){
            myPad = updateButtons(myPad);
            UpdatePad(myPad);

            if(_Up[NEW]){ myFPS++; game.setFrameRate(myFPS); }
            if(_Down[NEW]){ myFPS--; game.setFrameRate(myFPS); }

            game.display.setCursor(0,0);
            game.display.setColor(col--); if(col<0)col=255;
            game.display.setInvisibleColor(-1);
            game.display.print(myFPS);
            game.display.setCursor(0,8);
            game.display.print(FPS);

            frameNumber++;
            unsigned long currentTicks = game.getTime();
            if(currentTicks-tempTime>1000){
                tempTime=currentTicks;
                FPS=frameNumber-1;

                frameNumber=0;
            }

            if(_A[NEW]){
                playSound(SOUND_PLAUNCH);
                make_plasma(0,16,game.display.width-1,game.display.height-1);
            }
            game.display.palOffset = 255-col;
            //game.display.rotatePalette(1);
            //game.display.update();

        }
    }

    return 1;
}
