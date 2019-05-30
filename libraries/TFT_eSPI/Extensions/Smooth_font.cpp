 // Coded by Bodmer 10/2/18, see license in root directory.
 // This is part of the TFT_eSPI class and is associated with anti-aliased font functions
 

////////////////////////////////////////////////////////////////////////////////////////
// New anti-aliased (smoothed) font functions added below
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           loadFont
** Description:             loads parameters from a new font vlw file stored in SPIFFS
*************************************************************************************x*/
#define NOMINMAX
#include <stdint.h>

extern const uint8_t trado1_start[] asm("_binary_trado1_vlw_start");
extern const uint8_t trado1_end[]   asm("_binary_trado1_vlw_end");

extern const uint8_t trado2_start[] asm("_binary_trado2_vlw_start");
extern const uint8_t trado2_end[]   asm("_binary_trado2_vlw_end");

extern const uint8_t trado3_start[] asm("_binary_trado3_vlw_start");
extern const uint8_t trado3_end[]   asm("_binary_trado3_vlw_end");

extern const uint8_t trado4_start[] asm("_binary_trado4_vlw_start");
extern const uint8_t trado4_end[]   asm("_binary_trado4_vlw_end");

extern const uint8_t trado5_start[] asm("_binary_trado5_vlw_start");
extern const uint8_t trado5_end[]   asm("_binary_trado5_vlw_end");

#include "../TFT_eSPI.h"



void TFT_eSPI::set_unicode_font(size_t font_ind){
	font_index = font_ind - 1;
}

void TFT_eSPI::loadFont(String fontName)
{
	
	
  ESP_LOGE("TTT", "#loadFont");
  /*
    The vlw font format does not appear to be documented anywhere, so some reverse
    engineering has been applied!

    Header of vlw file comprises 6 uint32_t parameters (24 bytes total):
      1. The gCount (number of character glyphs)
      2. A version number (0xB = 11 for the one I am using)
      3. The font size (in points, not pixels)
      4. Deprecated mboxY parameter (typically set to 0)
      5. Ascent in pixels from baseline to top of "d"
      6. Descent in pixels from baseline to bottom of "p"

    Next are gCount sets of values for each glyph, each set comprises 7 int32t parameters (28 bytes):
      1. Glyph Unicode stored as a 32 bit value
      2. Height of bitmap bounding box
      3. Width of bitmap bounding box
      4. gxAdvance for cursor (setWidth in Processing)
      5. dY = distance from cursor baseline to top of glyph bitmap (signed value +ve = up)
      6. dX = distance from cursor to left side of glyph bitmap (signed value -ve = left)
      7. padding value, typically 0

    The bitmaps start next at 24 + (28 * gCount) bytes from the start of the file.
    Each pixel is 1 byte, an 8 bit Alpha value which represents the transparency from
    0xFF foreground colour, 0x00 background. The sketch uses a linear interpolation
    between the foreground and background RGB component colours. e.g.
        pixelRed = ((fgRed * alpha) + (bgRed * (255 - alpha))/255
    To gain a performance advantage fixed point arithmetic is used with rounding and
    division by 256 (shift right 8 bits is faster).

    After the bitmaps is:
       1 byte for font name string length (excludes null)
       a zero terminated character string giving the font name
       1 byte for Postscript name string length
       a zero/one terminated character string giving the font name
       last byte is 0 for non-anti-aliased and 1 for anti-aliased (smoothed)

    Then the font name seen by Java when it's created
    Then the postscript name of the font
    Then a boolean to tell if smoothing is on or not.

    Glyph bitmap example is:
    // Cursor coordinate positions for this and next character are marked by 'C'
    // C<------- gxAdvance ------->C  gxAdvance is how far to move cursor for next glyph cursor position
    // |                           |
    // |                           |   ascent is top of "d", descent is bottom of "p"
    // +-- gdX --+             ascent
    // |         +-- gWidth--+     |   gdX is offset to left edge of glyph bitmap
    // |   +     x@.........@x  +  |   gdX may be negative e.g. italic "y" tail extending to left of
    // |   |     @@.........@@  |  |   cursor position, plot top left corner of bitmap at (cursorX + gdX)
    // |   |     @@.........@@ gdY |   gWidth and gHeight are glyph bitmap dimensions
    // |   |     .@@@.....@@@@  |  |
    // | gHeight ....@@@@@..@@  +  +    <-- baseline
    // |   |     ...........@@     |
    // |   |     ...........@@     |   gdY is the offset to the top edge of the bitmap
    // |   |     .@@.......@@. descent plot top edge of bitmap at (cursorY + yAdvance - gdY)
    // |   +     x..@@@@@@@..x     |   x marks the corner pixels of the bitmap
    // |                           |
    // +---------------------------+   yAdvance is y delta for the next line, font size or (ascent + descent)
    //                                  some fonts can overlay in y direction so may need a user adjust value

  */


   unloadFont();
  _gFontFilename.emplace_back("_binary_trado1_vlw_start");
  _gFontFilename.emplace_back("_binary_trado2_vlw_start");
  _gFontFilename.emplace_back("_binary_trado3_vlw_start");
  _gFontFilename.emplace_back("_binary_trado4_vlw_start");
  _gFontFilename.emplace_back("_binary_trado5_vlw_start");
  //_gFontFilename = "_binary_" + fontName + "_vlw_start";

  if(fontName == "trado1"){
	fontFile.emplace_back(trado1_start);
	fontFile.emplace_back(trado2_start);
	fontFile.emplace_back(trado3_start);
	fontFile.emplace_back(trado4_start);
	fontFile.emplace_back(trado5_start);
  }
  else{
	return;
  }
  fontFileStart.emplace_back(fontFile[0]);
  fontFileStart.emplace_back(fontFile[1]);
  fontFileStart.emplace_back(fontFile[2]);
  fontFileStart.emplace_back(fontFile[3]);
  fontFileStart.emplace_back(fontFile[4]);
  
  //fontFileStart = fontFile;

  if(fontFile.empty()) return;
 
  gFont.resize(5);
  for(size_t i = 0;i < 5;++i){
	  gFont[i].gCount   = (uint16_t)readInt32(i); // glyph count in file
								 readInt32(i); // vlw encoder version - discard
	  gFont[i].yAdvance = (uint16_t)readInt32(i); // Font size in points, not pixels
								 readInt32(i); // discard
	  gFont[i].ascent   = (uint16_t)readInt32(i); // top of "d"
	  gFont[i].descent  = (uint16_t)readInt32(i); // bottom of "p"

	  // These next gFont values will be updated when the Metrics are fetched
	  gFont[i].maxAscent  = gFont[i].ascent;   // Determined from metrics
	  gFont[i].maxDescent = gFont[i].descent;  // Determined from metrics
	  gFont[i].yAdvance   = gFont[i].ascent + gFont[i].descent;
	  gFont[i].spaceWidth = gFont[i].yAdvance / 4;  // Guess at space width
  
	  // Fetch the metrics for each glyph
	  loadMetrics(gFont[i].gCount, i);
  }
  fontLoaded = true;
}


uint16_t hash_unicode(const uint16_t unicode){
	return (unicode <= 0x0040) * (unicode - 0x0020) + 
		   (unicode >= 0x621  && unicode <= 0x66c)*   (unicode - 0x0621 + (0x0040-0x0020)) +
		   (unicode >= 0xFE80 && unicode <= 0xFEFC)* (unicode - 0xFE80 + (0x0040-0x0020) + (0x066C-0x0621));
	
	/*if(unicode <= 0x3B){
		return unicode - 37;
	}	
	if(unicode <= 0x66c){
		return unicode - 1569 + 23;
	}	
	return unicode - 65152 + 98;*/
}
/***************************************************************************************
** Function name:           loadMetrics
** Description:             Get the metrics for each glyph and store in RAM
*************************************************************************************x*/
//#define SHOW_ASCENT_DESCENT
void TFT_eSPI::loadMetrics(uint16_t gCount, size_t ind)
{
  uint32_t headerPtr = 24;
  uint32_t bitmapPtr = 24 + gCount * 28;
  uint32_t bitmapPtrStart = bitmapPtr;
  gUnicodeIndexHash[ind] = (uint16_t*)heap_caps_malloc( gCount * 2, MALLOC_CAP_SPIRAM);
  for(size_t i = 0; i < gCount; ++i){
	gUnicodeIndexHash[ind][i] = 0;
  }
  gUnicode[ind]  = (uint16_t*)heap_caps_malloc( gCount * 2, MALLOC_CAP_SPIRAM); // Unicode 16 bit Basic Multilingual Plane (0-FFFF)
  gHeight[ind]   =  (uint8_t*)heap_caps_malloc( gCount , MALLOC_CAP_SPIRAM);    // Height of glyph
  gWidth[ind]    =  (uint8_t*)heap_caps_malloc( gCount , MALLOC_CAP_SPIRAM);    // Width of glyph
  gxAdvance[ind] =  (uint8_t*)heap_caps_malloc( gCount , MALLOC_CAP_SPIRAM);    // xAdvance - to move x cursor
  gdY[ind]       =   (int8_t*)heap_caps_malloc( gCount , MALLOC_CAP_SPIRAM);    // offset from bitmap top edge from lowest point in any character
  gdX[ind]       =   (int8_t*)heap_caps_malloc( gCount , MALLOC_CAP_SPIRAM);    // offset for bitmap left edge relative to cursor X
  gBitmap[ind]   = (uint32_t*)heap_caps_malloc( gCount * 4, MALLOC_CAP_SPIRAM); // seek pointer to glyph bitmap in SPIFFS file
  
#ifdef SHOW_ASCENT_DESCENT
  Serial.print("ascent  = "); Serial.println(gFont[ind].ascent);
  Serial.print("descent = "); Serial.println(gFont[ind].descent);
#endif

  uint16_t gNum = 0;
  //fontFile.seek(headerPtr, fs::SeekSet);
  fontFile[ind] = fontFileStart[ind] + headerPtr;
  Serial.println("CCCCCOOOOOOUUUUUUUUNT: " + String(gCount));
  while (gNum < gCount)
  {
	
    gUnicode[ind][gNum]  = (uint16_t)readInt32(ind); // Unicode code point value
	gUnicodeIndexHash[ind][hash_unicode(gUnicode[ind][gNum])] = gNum;
    gHeight[ind][gNum]   =  (uint8_t)readInt32(ind); // Height of glyph
    gWidth[ind][gNum]    =  (uint8_t)readInt32(ind); // Width of glyph
    gxAdvance[ind][gNum] =  (uint8_t)readInt32(ind); // xAdvance - to move x cursor
    gdY[ind][gNum]       =   (int8_t)readInt32(ind); // y delta from baseline
    gdX[ind][gNum]       =   (int8_t)readInt32(ind); // x delta from cursor
    readInt32(ind); // ignored

    // Different glyph sets have different ascent values not always based on "d", so get maximum glyph ascent
    if (gdY[ind][gNum] > gFont[ind].maxAscent)
    {
      // Avoid UTF coding values and characters that tend to give duff values
      if (((gUnicode[ind][gNum] > 0x20) && (gUnicode[ind][gNum] < 0xA0) && (gUnicode[ind][gNum] != 0x7F)) || (gUnicode[ind][gNum] > 0xFF))
      {
        gFont[ind].maxAscent   = gdY[ind][gNum];
#ifdef SHOW_ASCENT_DESCENT
        Serial.print("Unicode = 0x"); Serial.print(gUnicode[ind][gNum], HEX); Serial.print(", maxAscent  = "); Serial.println(gFont[ind].maxAscent);
#endif
      }
    }

    // Different glyph sets have different descent values not always based on "p", so get maximum glyph descent
    if (((int16_t)gHeight[ind][gNum] - (int16_t)gdY[ind][gNum]) > gFont[ind].maxDescent)
    {
      // Avoid UTF coding values and characters that tend to give duff values
      if (((gUnicode[ind][gNum] > 0x20) && (gUnicode[ind][gNum] < 0xA0) && (gUnicode[ind][gNum] != 0x7F)) || (gUnicode[ind][gNum] > 0xFF))
      {
        gFont[ind].maxDescent   = gHeight[ind][gNum] - gdY[ind][gNum];
#ifdef SHOW_ASCENT_DESCENT
        Serial.print("Unicode = 0x"); Serial.print(gUnicode[ind][gNum], HEX); Serial.print(", maxDescent = "); Serial.println(gHeight[ind][gNum] - gdY[ind][gNum]);
#endif
      }
    }

    gBitmap[ind][gNum] = bitmapPtr;

    headerPtr += 28;

    bitmapPtr += gWidth[ind][gNum] * gHeight[ind][gNum];

    gNum++;
    yield();
  }

  gFont[ind].yAdvance = gFont[ind].maxAscent + gFont[ind].maxDescent;

  gFont[ind].spaceWidth = (gFont[ind].ascent + gFont[ind].descent) * 2/7;  // Guess at space width
  
  
  gBitmapData[ind] = (uint8_t*)heap_caps_malloc(bitmapPtr - gBitmap[ind][0] + 4, MALLOC_CAP_SPIRAM);
  
  gNum = 0;
  uint32_t gBitmapDataPtr = 0;
  while (gNum < gCount)
  {
	//fontFile.seek(gBitmap[gNum], fs::SeekSet); // This is taking >30ms for a significant position shift
	fontFile[ind] = fontFileStart[ind] + gBitmap[ind][gNum];
    for (int y = 0; y < gHeight[ind][gNum]; y++)
    {
      //fontFile.read(gBitmapData + gBitmapDataPtr, gWidth[gNum]); //<//
	  memcpy(gBitmapData[ind] + gBitmapDataPtr, fontFile[ind], gWidth[ind][gNum]);
	  fontFile[ind] +=gWidth[ind][gNum];
	  
	  gBitmapDataPtr += gWidth[ind][gNum];	  
	}
	gBitmap[ind][gNum] -= bitmapPtrStart;
	//Serial.println("gBitmap[" + String(gNum) + "] = " + String(gBitmap[gNum]));	
    gNum++;
    yield();
  }
}


/***************************************************************************************
** Function name:           deleteMetrics
** Description:             Delete the old glyph metrics and free up the memory
*************************************************************************************x*/
void TFT_eSPI::unloadFont( void )
{
	for(size_t i =0;i<5;++i){
	  ESP_LOGE("TTT", "#1");
	  if (gUnicode[i])
	  {
		free(gUnicode[i]);
		gUnicode[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#2");

	  if (gHeight[i])
	  {
		free(gHeight[i]);
		gHeight[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#3");

	  if (gWidth[i])
	  {
		free(gWidth[i]);
		gWidth[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#4");

	  if (gxAdvance[i])
	  {
		free(gxAdvance[i]);
		gxAdvance[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#5");

	  if (gdY[i])
	  {
		free(gdY[i]);
		gdY[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#6");

	  if (gdX[i])
	  {
		free(gdX[i]);
		gdX[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#7");

	  if (gBitmap[i])
	  {
		free(gBitmap[i]);
		gBitmap[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#8");
	  
	  if(gBitmapData[i]){
		  free(gBitmapData[i]);
		  gBitmapData[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#9");

	  if(gUnicodeIndexHash[i]){
		free(gUnicodeIndexHash[i]);
		gUnicodeIndexHash[i] = NULL;
	  }
	  ESP_LOGE("TTT", "#10");
	  
	  /*if(fontFile){ 
		fontFile.close();
	  }*/
	  ESP_LOGE("TTT", "#11");
	}
  fontLoaded = false;
}


/***************************************************************************************
** Function name:           decodeUTF8
** Description:             Line buffer UTF-8 decoder with fall-back to extended ASCII
*************************************************************************************x*/
#define DECODE_UTF8
uint16_t TFT_eSPI::decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining)
{
  byte c = buf[(*index)++];
  //Serial.print("Byte from string = 0x"); Serial.println(c, HEX);

#ifdef DECODE_UTF8
  // 7 bit Unicode
  if ((c & 0x80) == 0x00) return c;

  // 11 bit Unicode
  if (((c & 0xE0) == 0xC0) && (remaining > 1))
    return ((c & 0x1F)<<6) | (buf[(*index)++]&0x3F);

  // 16 bit Unicode
  if (((c & 0xF0) == 0xE0) && (remaining > 2))
    return ((c & 0x0F)<<12) | ((buf[(*index)++]&0x3F)<<6) | ((buf[(*index)++]&0x3F));

  // 21 bit Unicode not supported so fall-back to extended ASCII
  // if ((c & 0xF8) == 0xF0) return c;
#endif

  return c; // fall-back to extended ASCII
}

/***************************************************************************************
** Function name:           decodeUTF8
** Description:             Serial UTF-8 decoder with fall-back to extended ASCII
*************************************************************************************x*/
uint16_t TFT_eSPI::decodeUTF8(uint8_t c)
{

#ifdef DECODE_UTF8
  if (decoderState == 0)
  {
    // 7 bit Unicode
    if ((c & 0x80) == 0x00) return (uint16_t)c;

    // 11 bit Unicode
    if ((c & 0xE0) == 0xC0)
    {
      decoderBuffer = ((c & 0x1F)<<6);
      decoderState = 1;
      return 0;
    }

    // 16 bit Unicode
    if ((c & 0xF0) == 0xE0)
    {
      decoderBuffer = ((c & 0x0F)<<12);
      decoderState = 2;
      return 0;
    }
    // 21 bit Unicode not supported so fall-back to extended ASCII
    if ((c & 0xF8) == 0xF0) return (uint16_t)c;
  }
  else
  {
    if (decoderState == 2)
    {
      decoderBuffer |= ((c & 0x3F)<<6);
      decoderState--;
      return 0;
    }
    else
    {
      decoderBuffer |= (c & 0x3F);
      decoderState = 0;
      return decoderBuffer;
    }
  }
#endif

  return (uint16_t)c; // fall-back to extended ASCII
}



/***************************************************************************************
** Function name:           alphaBlend
** Description:             Blend foreground and background and return new colour
*************************************************************************************x*/
uint16_t TFT_eSPI::alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc)
{
  // For speed use fixed point maths and rounding to permit a power of 2 division
  uint16_t fgR = ((fgc >> 10) & 0x3E) + 1;
  uint16_t fgG = ((fgc >>  4) & 0x7E) + 1;
  uint16_t fgB = ((fgc <<  1) & 0x3E) + 1;

  uint16_t bgR = ((bgc >> 10) & 0x3E) + 1;
  uint16_t bgG = ((bgc >>  4) & 0x7E) + 1;
  uint16_t bgB = ((bgc <<  1) & 0x3E) + 1;

  // Shift right 1 to drop rounding bit and shift right 8 to divide by 256
  uint16_t r = (((fgR * alpha) + (bgR * (255 - alpha))) >> 9);
  uint16_t g = (((fgG * alpha) + (bgG * (255 - alpha))) >> 9);
  uint16_t b = (((fgB * alpha) + (bgB * (255 - alpha))) >> 9);

  // Combine RGB565 colours into 16 bits
  //return ((r&0x18) << 11) | ((g&0x30) << 5) | ((b&0x18) << 0); // 2 bit greyscale
  //return ((r&0x1E) << 11) | ((g&0x3C) << 5) | ((b&0x1E) << 0); // 4 bit greyscale
  return (r << 11) | (g << 5) | (b << 0);
}


/***************************************************************************************
** Function name:           readInt32
** Description:             Get a 32 bit integer from the font file
*************************************************************************************x*/
uint32_t TFT_eSPI::readInt32(size_t i)
{
  uint32_t val = 0;
  val |= fontFile[i][0] << 24;
  val |= fontFile[i][1] << 16;
  val |= fontFile[i][2] << 8;
  val |= fontFile[i][3];
  fontFile[i] += 4;
  return val;
}


/***************************************************************************************
** Function name:           getUnicodeIndex
** Description:             Get the font file index of a Unicode character
*************************************************************************************x*/
bool TFT_eSPI::getUnicodeIndex(uint16_t unicode, uint16_t *index)
{
	*index = gUnicodeIndexHash[font_index][hash_unicode(unicode)];
	return true;
  
  // for (uint16_t i = 0; i < gFont.gCount; i++)
  // {
    // if (gUnicode[i] == unicode)
    // {
      // *index = i;
      // return true;
    // }
  // }
  // return false;
}


/***************************************************************************************
** Function name:           drawGlyph
** Description:             Write a character to the TFT cursor position
*************************************************************************************x*/
// Expects file to be open
void TFT_eSPI::drawGlyph(uint16_t code)
{
  if (code < 0x21)
  {
    if (code == 0x20) {
      cursor_x += gFont[font_index].spaceWidth;
      return;
    }

    if (code == '\n') {
      cursor_x = 0;
      cursor_y += gFont[font_index].yAdvance;
      if (cursor_y >= _height) cursor_y = 0;
      return;
    }
  }

  uint16_t gNum = 0;
  bool found = getUnicodeIndex(code, &gNum);
  
  uint16_t fg = textcolor;
  uint16_t bg = textbgcolor;

  if (found)
  {

    if (textwrapX && (cursor_x + gWidth[font_index][gNum] + gdX[font_index][gNum] > _width))
    {
      cursor_y += gFont[font_index].yAdvance;
      cursor_x = 0;
    }
    if (textwrapY && ((cursor_y + gFont[font_index].yAdvance) >= _height)) cursor_y = 0;
    if (cursor_x == 0) cursor_x -= gdX[font_index][gNum];

    //fontFile.seek(gBitmap[gNum], fs::SeekSet); // This is taking >30ms for a significant position shift
	uint8_t* gBitmapDataPtr = gBitmapData[font_index] + gBitmap[font_index][gNum];
    uint8_t* pbuffer = NULL;

    uint16_t xs = 0;
    uint32_t dl = 0;

    int16_t cy = cursor_y + gFont[font_index].maxAscent - gdY[font_index][gNum];
    int16_t cx = cursor_x + gdX[font_index][gNum];
    for (int y = 0; y < gHeight[font_index][gNum]; y++)
    {
      //fontFile.read(pbuffer, gWidth[gNum]); //<//
      for (int x = 0; x < gWidth[font_index][gNum]; x++)
      {
        uint8_t pixel = *(gBitmapDataPtr); //<//
		++gBitmapDataPtr;
        if (pixel)
        {
          if (pixel != 0xFF)
          {
            if (dl) {
              if (dl==1) drawPixel(xs, y + cy, fg);
              else drawFastHLine( xs, y + cy, dl, fg);
              dl = 0;
            }
            drawPixel(x + cx, y + cy, alphaBlend(pixel, fg, bg));
          }
          else
          {
            if (dl==0) xs = x + cx;
            dl++;
          }
        }
        else
        {
          if (dl) { drawFastHLine( xs, y + cy, dl, fg); dl = 0; }
        }
      }
      if (dl) { drawFastHLine( xs, y + cy, dl, fg); dl = 0; }
    }

    cursor_x += gxAdvance[font_index][gNum];
  }
  else
  {
    // Not a Unicode in font so draw a rectangle and move on cursor
    drawRect(cursor_x, cursor_y + gFont[font_index].maxAscent - gFont[font_index].ascent, gFont[font_index].spaceWidth, gFont[font_index].ascent, fg);
    cursor_x += gFont[font_index].spaceWidth + 1;
  }
  
}

/***************************************************************************************
** Function name:           showFont
** Description:             Page through all characters in font, td ms between screens
*************************************************************************************x*/
void TFT_eSPI::showFont(uint32_t td)
{
  if(!fontLoaded) return;
//  fontFile = SPIFFS.open( _gFontFilename, "r" );

  if(fontFile.empty())
  {
    fontLoaded = false;
    return;
  }

  int16_t cursorX = width(); // Force start of new page to initialise cursor
  int16_t cursorY = height();// for the first character
  uint32_t timeDelay = 0;    // No delay before first page

  fillScreen(textbgcolor);
  
  for (uint16_t i = 0; i < gFont[font_index].gCount; i++)
  {
    // Check if this will need a new screen
    if (cursorX + gdX[font_index][i] + gWidth[font_index][i] >= width())  {
      cursorX = -gdX[font_index][i];

      cursorY += gFont[font_index].yAdvance;
      if (cursorY + gFont[font_index].maxAscent + gFont[font_index].descent >= height()) {
        cursorX = -gdX[font_index][i];
        cursorY = 0;
        delay(timeDelay);
        timeDelay = td;
        fillScreen(textbgcolor);
      }
    }

    setCursor(cursorX, cursorY);
    drawGlyph(gUnicode[font_index][i]);
    cursorX += gxAdvance[font_index][i];
    //cursorX +=  printToSprite( cursorX, cursorY, i );
    yield();
  }

  delay(timeDelay);
  fillScreen(textbgcolor);
  //fontFile.close();

}
