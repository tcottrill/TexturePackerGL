/*
Sprite Sheet example implementation  for C++ and OpenGL 2.0+

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#ifndef __Sprite__
#define __Sprite__

#include "gl_basics.h"
#include <vector>
#include <map>
#include "colordefs.h"

//Helper internal Vertex classes for this implementation

class _point
{
public:
	float x;
	float y;
	_point(float x, float y) : x(x), y(y) {}
	_point() : x(0), y(0) { }
};


class _txdata 
{
public:

	float x, y;
	float tx, ty;
	rgb_t colors;

	_txdata() : x(0), y(0), tx(0), ty(0), colors(0) { }
	_txdata(float x, float y, float tx, float ty, rgb_t colors) : x(x), y(y), tx(tx), ty(ty), colors(colors) {}

};


class _stdata 
{
public:

	float s, t;
	float w, h;
	
	_stdata() : s(0), t(0), w(0), h(0)  { }
	_stdata(float s, float t, float w, float h) : s(s), t(t), w(w), h(h) {}

};


class SpriteDescriptor
{
public:

	short x, y;
	short Width;
	short Height;
	std::string name;
	_point spOffset;
	float scale;
	int angle;
	int rot;
	std::vector< _stdata> stlist;              //SpriteSheet Coordinates for each sprite;
	SpriteDescriptor() : x(0), y(0), Width(0), Height(0), name(" "), spOffset(0, 0), scale(1.0), angle(0), rot(0) { }
};


class Sprite
{
 public:

	bool LoadSheet(std::string fontfile);
	void SetColor(int r, int g, int b, int a) {fcolor = MAKE_RGBA(r,g,b,a);}
	void SetColor(rgb_t color) {fcolor=color;}
	void SetBlend(int b) {fblend = b;}
	void SetScale(int, float);
	void SetCaching(bool en);
	void ClearCache(){txlist.clear();};
	void SetOriginOffset(float x, float y, int spritenum);
	void SetOriginOffset(_point a, int spritenum);
	void SetRotationAngle(int spritenum, int sangle);
	int ByName(std::string name);
	std::string ByNum(int spritenum);
	void SetPath(std::string &path);
	void SetPath(const char *path);
	std::string GetPath();
	
	void Add(float x, float y, int spritenum);
	void Add(float scale, float x, float y, int spritenum);
	void Add(float x, float y, rgb_t color, float scale, int spritenum);
	void Add(float x, float y, rgb_t color, float scale, int angle, int spritenum);
		
	void  Render();
		
	Sprite() 
	  {
		  SetColor(RGB_WHITE);
		  fblend = 0;
          fscale = 1.0;
		  fangle = 0;
		  fcache = false;
		  count = 0;
	  };
	 ~Sprite ();

 private:
	 //Character and page variables
	 short sheetWidth;
	 short sheetHeight;
	 short count;

	 std::vector<SpriteDescriptor> Sprites;     //Character Descriptor Map
	 std::vector<_txdata> txlist;               //Vertex Array Data
	 std::string Pngname;                       //Png File name storage for loading

	 //Modifiers
	 rgb_t fcolor;							    //Sprite current color
	 TEX *ftexid;							    //Sprite Texture ID
	 float fscale;							    //Current Font scaling factor
	 int fblend;								//Current Rendering Blending Value"Do I need this?"
	 int fangle;								//Set the Sprite angle in degrees
	 bool fcache;                               // You can "Cache" all the sprite data if static, not sure why you would use this, hud maybe.
	 std::string datapath;                      //Path for data files, if empty use the root directory       

	 //Internal Functions
	 bool ParseDefinition(const std::string &);
	 void InternalAdd(float x, float y, int spriteid);
	 _point RotateAroundPoint(float, float, float, float, float, float);
};



#endif
