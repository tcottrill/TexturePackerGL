/*
BASIC BMFont example implementation with Kerning, for C++ and OpenGL 2.0+
Completely self contained, drop into any Opengl 2.0 - 3.2 Project and use.
For 3.3 plus and GL ES, see the shader based version, coming soon.

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
--------------------------------------------------------------------------------
These editors can be used to generate BMFonts:
 • http://www.angelcode.com/products/bmfont/ (free, windows)
 • http://glyphdesigner.71squared.com/ (commercial, mac os x)
 • http://www.n4te.com/hiero/hiero.jnlp (free, java, multiplatform)
 • http://slick.cokeandcode.com/demos/hiero.jnlp (free, java, multiplatform)

Some code below based on code snippets from this gamedev posting:

http://www.gamedev.net/topic/330742-quick-tutorial-variable-width-bitmap-fonts/

Although I'm giving this away, I'd appreciate an email with fixes or better code!

****@outlook.com Revision 5 2021-2023
*/


#include <cstdarg>					//va_start, va_end, std::va_list
#include <fstream>
#include <sstream>
#include <string>
#define _USE_MATH_DEFINES
#include "math.h"
#include "bmfont.h"
#include "log.h"

// For VERY FAST Kerning character lookup. Static Function
// This is a simple hash of the kerning values. This only works because the values do not repeat!
inline std::size_t key(int i, int j) { return ((std::size_t) i << 16 | (unsigned int)j); }

//Init
BMFont::BMFont(int width, int height)
{
	surface_width = width;              //Size of drawing window/surface width
	surface_height = height;			//Size of drawing window/surface height 
	setColor(RGB_WHITE);			    //Default color is White
	KernCount = 0;						//Set the Total for Kerning Information
	fblend = 0;							//Blending Value for Depreciated GL 2 Functions.
	fscale = 1.0;						//Default scaling value
	fangle = 0;							//Default Rotation Angle
	fcache = false;						//Whether or not to use caching
	falign = fontalign::AlignNone;		//Default Drawing alignment. 
	forigin = fontorigin::Bottom;		//Default drawing Origin
	UseKern = true;		                //To use the supplied kerning values or not
};


//Cleanup
BMFont::~BMFont()
{
	if (ftexid)
	{
		if (ftexid->texid)
			//delete the attached texture
			glDeleteTextures(1, &ftexid->texid);
		//delete the associated TEX object
		delete ftexid;
	}
	//All std:: objects call their own destructors
	wrlog("BMFont Instance Freed");
}

/**
Parses the font definition file and reads the specified keywords.
This could be broken out in other, more robust (and more compact) parsing code.
Next release this parsing code may change significantly.
*/
bool BMFont::parseFont(const std::string &fontfile)
{
	std::string Line, Read, Key, Value;
	std::size_t i;
	std::ifstream Stream(fontfile.c_str());
	
	KearningInfo K;
	CharDescriptor C;

	while (!Stream.eof())
	{
		std::stringstream LineStream;
		std::getline(Stream, Line);
		LineStream << Line;

		//read the line's type
		LineStream >> Read;
		if (Read == "common")
		{
			//this holds common data
			while (!LineStream.eof())
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);
				//assign the correct value
				Converter << Value;
				if (Key == "lineHeight") {
					Converter >> LineHeight;
					wrlog("Line height is %d", LineHeight);
				}
				else if (Key == "base") 	Converter >> Base;
				else if (Key == "scaleW")	Converter >> Width;
				else if (Key == "scaleH")	Converter >> Height;
				else if (Key == "pages")	Converter >> Pages;
				else if (Key == "outline")	Converter >> Outline;
			}
		}
		else if (Read == "page")
		{
			while (!LineStream.eof())
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				//assign the correct value
				Converter << Value;
				if (Key == "file")
				{
					Converter >> Pngname;
					if (Pngname.front() == '"') {
						Pngname.erase(0, 1); // erase the first character
						Pngname.erase(Pngname.size() - 1); // erase the last character
						//To remove the double quotes.
					}
				}
			}
		}
		else if (Read == "char")
		{
			//This is data for each specific character.
			int CharID = 0;

			while (!LineStream.eof())
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				//Assign the correct value
				Converter << Value;
				if (Key == "id")			Converter >> CharID;
				else if (Key == "x")		Converter >> C.x;
				else if (Key == "y")		Converter >> C.y;
				else if (Key == "width")	Converter >> C.Width;
				else if (Key == "height")	Converter >> C.Height;
				else if (Key == "xoffset")	Converter >> C.XOffset;
				else if (Key == "yoffset")	Converter >> C.YOffset;
				else if (Key == "xadvance")	Converter >> C.XAdvance;
				else if (Key == "page")		Converter >> C.Page;
			}
			Chars.emplace(CharID, C);
		}
		else if (Read == "kernings")
		{
			while (!LineStream.eof())
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				//assign the correct value
				Converter << Value;
				if (Key == "count")			Converter >> KernCount;
			}
		}
		else if (Read == "kerning")
		{
			while (!LineStream.eof())
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				//assign the correct value
				Converter << Value;
				if (Key == "first")			Converter >> K.First;
				else if (Key == "second")	Converter >> K.Second;
				else if (Key == "amount")	Converter >> K.Amount;
			}
			//Add to the kerning map
			KernMap[key(K.First, K.Second)] = K.Amount;
		}
	}
	Stream.close();
	return true;
}


//Simple loader that checks if the file exists and then loads it and the associated png
bool  BMFont::loadFont(std::string fontfile)
{

	if (!datapath.empty())
	{
		fontfile = datapath + fontfile;
	}
	//Open Font
	wrlog("Opening file %s", fontfile.c_str());

	std::ifstream Stream(fontfile);
	if (!Stream)
	{
		Stream.close();
		wrlog("WARNING::::->Cannot find font file for loading %s",  fontfile.c_str());
		return false;
	}
	wrlog("Passing font");

	parseFont(fontfile);

	wrlog("Kearning pair count from text file is %d", KernCount);

	KernCount = KernMap.size();//(short)Kearn.size();
	wrlog("Kearning pair count from reader is %d", KernCount);
	wrlog("Finished parsing font");
	Stream.close();
	
	if (!datapath.empty())	{ Pngname = datapath + Pngname;	}

	wrlog("Loading Texture %s", Pngname.c_str());
	ftexid = LoadPNG(Pngname.c_str());
	if (!ftexid)
	{
		wrlog("WARNING:::: -> Cannot find font texture for loading %s", fontfile);
		return false;
	}
	wrlog("Finished loading Texture");
	return true;
}


//Simple Vector Renderer Needs moved to it's own file and class for real world use. (OR changed to a shader)
void BMFont::Render()
{
	//Select and enable the font texture. (With mipmapping.)
	UseTexture(&ftexid->texid, 1, 1);
	//Set type of blending to use with this font.
	SetBlendMode(fblend);

	//Enable Client States
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(txdata), &txlist[0].x);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(txdata), &txlist[0].tx);

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(txdata), &txlist[0].colors);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)txlist.size());

	// Finished Drawing, disable client states.
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	//Caching Check, if we are pre caching the text.
	if (!fcache) txlist.clear();
}



void BMFont::setAngle(int angle)
{
	if (angle > 360 || angle < -360) angle = 0;
	if (angle < 0) angle = 360 + angle;
	fangle = angle;
}


void BMFont::setCaching(bool en)
{
	if (en) fcache = true;
	else fcache = false;
}


void BMFont::setAlign(fontalign align)
{
	if (align < 4) falign = align;
}


void BMFont::setScale(float newscale)
{
	if (newscale > 0.1f && newscale < 10.0f)
	{
		fscale = newscale;
	}
}

void BMFont::setOrigin(fontorigin origin)
{
	if (origin == 0) forigin = false;
	else forigin = true;
}


void BMFont::setPath(const char *path)
{
	datapath.assign(path);
}


void BMFont::setPath(std::string &path)
{
	datapath.assign(path);
}


std::string BMFont::GetPath()
{
	return datapath;
}


/**
Tries to obtain the Kerning values for each pair of letters, if loaded and requested.
*/
int BMFont::getKerningPair(int first, int second)
{
	//Only process if there actually is kerning information
	if (KernCount && UseKern) 
	{
		return (int)KernMap[key(first, second)];
	}
	return 0;
}


//Returns the width in pixels of the font string being processed.
float BMFont::getStrWidth(const char *string)
{
	float total = 0;
	size_t len = strlen(string);
	CharDescriptor  *f;

	for (size_t i = 0; i != len; ++i)
	{
		f = &Chars[string[i]];

		if (len > 1 && i < len)
		{
			total += getKerningPair(string[i], string[i + 1]);
		}

		total += fscale * f->XAdvance;
	}

	return total;
}


//Returns the width in pixels of the font string being processed.
float BMFont::getStrWidth(std::string &string)
{

	float total = 0;
	total = getStrWidth(string.c_str());
	return total;
}


//This is the main printing process that scales and fills in the vector structure to be printed later.
void BMFont::printString(float x, float y, char *textstr)
{
	CharDescriptor  *f;		   //Pointer to font.
	float CurX = 0.0f;
	float CurY = 0.0f;
	float DstX = 0.0f;
	float DstY = 0.0f;
	size_t Flen = 0;
	float s;
	float t;
	float w;
	float h;

	//Calculate String Length and save for later
	Flen = strlen(textstr);

	int total = 0;

	switch (falign)
	{
	case fontalign::AlignNear:
		x = LineHeight;// 1 + Chars[textstr[0]].Width; //Dangerous, assumes there is at least 1 character passed to printing function. Add check for this somewhere.
		break;									       //This places the Character 1 char width from the edge, so it doesn't disappear when rotated 90.
													   // Switched to LineHeight, which covers spacing when normal or rotated 90.
	case fontalign::AlignFar:
		x = (float)(surface_width);
		total = (int)getStrWidth(textstr);
		x -= total;
		break;
	case fontalign::AlignCenter:
		x = (float)(surface_width / 2);
		total = (int)getStrWidth(textstr);
		total = total / 2;
		x -= total;
		break;
	default: break;
	}

	//Now that alignment is settled, Get the origin point for rotation, this can be modified to the the center of the string, add more code here.
	// Currently this is the bottom left point.
	fpoint center(x, y);

	//Adjust for scaling factor
	//If orgin of Ortho View is bottom left.
	if (forigin == fontorigin::Bottom)
	{
		y += (LineHeight * fscale);
	}

	//Makes sure the x and y origin doesn't move with scaling.
	x = x * (1.0f / fscale);
	y = y * (1.0f / fscale);
	//Now scaled X and Y are the new origins.

	for (int i = 0; i != Flen; ++i)
	{

		f = &Chars[textstr[i]];

		//TBD
		//Fix this with actual text size
		if (textstr[i] == '\n') {x = (float) origx; y -= (float) (32 * fscale);	} 

		CurX = x + f->XOffset; 
		CurY = y;

		//Adjust for Origin Y
		if (forigin == fontorigin::Bottom)
			CurY -= f->YOffset; // - to flip
		else
			CurY += f->YOffset;

		DstX = CurX + f->Width;
		DstY = CurY;

		//Adjust for origin Y
		if (forigin == fontorigin::Bottom)
			DstY -= f->Height; // - to flip
		else
			DstY += f->Height;

		//Scaling
		CurX *= fscale;
		CurY *= fscale;
		DstX *= fscale;
		DstY *= fscale;

		//Calculate texture coords for this letter
		//This could be pre-calculated. I'll work on that later.
		s = (float)f->x / Width;
		t = (float)1.0f - (f->y / (float)Height);
		w = (float)f->Width / Width;
		h = (float)f->Height / (float)Height;

		//If we are printing at an angle, there are a few more calculations.
		if (fangle)
		{
			float cosTheta = cosf((float)(fangle * M_PI) / 180);
			float sinTheta = sinf((float)(fangle * M_PI) / 180);

			fpoint p0 = rotateAroundPoint(CurX, CurY, center.x, center.y, cosTheta, sinTheta);
			fpoint p1 = rotateAroundPoint(DstX, CurY, center.x, center.y, cosTheta, sinTheta);
			fpoint p2 = rotateAroundPoint(DstX, DstY, center.x, center.y, cosTheta, sinTheta);
			fpoint p3 = rotateAroundPoint(CurX, DstY, center.x, center.y, cosTheta, sinTheta);

			//V0
			txlist.emplace_back(p0.x, p0.y, s, t, fcolor);
			//V1
			txlist.emplace_back(p1.x, p1.y, s + w, t, fcolor);
			//V2
			txlist.emplace_back(p2.x, p2.y, s + w, t - h, fcolor);
			//V2
			txlist.emplace_back(p2.x, p2.y, s + w, t - h, fcolor);
			//V3
			txlist.emplace_back(p3.x, p3.y, s, t - h, fcolor);
			//V0
			txlist.emplace_back(p0.x, p0.y, s, t, fcolor);
		}
		else //We're not printing at an angle, easy. Emplace back all 6 triangle vertex's.
		{
			//V0
			txlist.emplace_back(CurX, CurY, s, t, fcolor);
			//V1
			txlist.emplace_back(DstX, CurY, s + w, t, fcolor);
			//V2
			txlist.emplace_back(DstX, DstY, s + w, t - h, fcolor);
			//V2
			txlist.emplace_back(DstX, DstY, s + w, t - h, fcolor);
			//V3
			txlist.emplace_back(CurX, DstY, s, t - h, fcolor);
			//V0
			txlist.emplace_back(CurX, CurY, s, t, fcolor);
		}
		
		x += getKerningPair(textstr[i], textstr[i + 1]);
		x += f->XAdvance; //Move to the next character.
	}
}



void BMFont::fPrintCenter(float y,  char *astring)
{
	float x = getStrWidth(astring);
	origx = x;
	printString((float) (surface_width / 2) - (x / 2), (float) y, astring);
}

//This is the main printing process that scales and fills in the vector structure to be printed later.
void BMFont::fPrint(float x, float y, const char *fmt, ...)
{
	char	text[1024] = "";	                        // Holds Our String

	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing
	origx = x;
	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						    // And Converts Symbols To Actual Numbers
	va_end(ap);
	printString(x, y, text);
}


void BMFont::fPrint(float scale, float x, float y, const char *fmt, ...)
{
	char	text[1024] = { 0 };	                        // Holds Our String

	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing
	origx = x;
	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						    // And Converts Symbols To Actual Numbers
	va_end(ap);

	setScale(scale);
	printString(x, y, text);
}


void BMFont::fPrint(float x, float y, rgb_t color, float scale, const char *fmt, ...)
{
	char	text[1024] = { 0 };	                        // Holds Our String

	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing
	origx = x;
	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						    // And Converts Symbols To Actual Numbers
	va_end(ap);

	setScale(scale);
	setColor(color);
	printString(x, y, text);
}


void BMFont::fPrint(float y, rgb_t color, float scale, int angle, const char *fmt, ...)
{
	char	text[1024] = { 0 };                        // Holds Our String

	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing
	origx = 0;
	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						    // And Converts Symbols To Actual Numbers
	va_end(ap);

	setAngle(angle);
	setScale(scale);
	setColor(color);
	setAlign(AlignCenter);
	printString(0, y, text);
}


//Debug
void BMFont::printKerningPairs()
{
	/*
	//DEBUG CODE
	  //Kearning is checked for every character processed. This is expensive in terms of processing time.
	for (size_t j = 0; j < Kearn.size(); j++)
	{
		//wrlog("FIRST: %d SECOND: %d offset %d", Kearn[j].First, Kearn[j].Second, Kearn[j].Amount);
		wrlog("%d,%d,%d,", Kearn[j].First, Kearn[j].Second, Kearn[j].Amount);
	}
	*/
}


//Rotation Code
fpoint BMFont::rotateAroundPoint(float x, float y, float cx, float cy, float cosTheta, float sinTheta)
{
	fpoint p;
	p.x = (((x - cx) * cosTheta) - ((y - cy) * sinTheta) + cx);
	p.y = (((x - cx) * sinTheta) + ((y - cy) * cosTheta) + cy);
	return p;
}