 // Coded by Bodmer 10/2/18, see license in root directory.
 // This is part of the TFT_eSPI class and is associated with anti-aliased font functions

 public:

  // These are for the new antialiased fonts
  void     loadFont(String fontName);
  void     unloadFont( void );
  bool     getUnicodeIndex(uint16_t unicode, uint16_t *index);
  void     set_unicode_font(size_t font_ind);
  uint16_t decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining);
  uint16_t decodeUTF8(uint8_t c);

  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc);

  void     drawGlyph(uint16_t code);
  void     showFont(uint32_t td);

 std::vector<const uint8_t*>   fontFile;
 std::vector<const uint8_t*>   fontFileStart;

size_t font_index = 0;
  // This is for the whole font
  typedef struct
  {
    uint16_t gCount;     // Total number of characters
    uint16_t yAdvance;   // Line advance
    uint16_t spaceWidth; // Width of a space character
    int16_t  ascent;     // Height of top of 'd' above baseline, other characters may be taller
    int16_t  descent;    // Offset to bottom of 'p', other characters may have a larger descent
    uint16_t maxAscent;  // Maximum ascent found in font
    uint16_t maxDescent; // Maximum descent found in font
  } fontMetrics;

   std::vector<fontMetrics> gFont;// = { 0, 0, 0, 0, 0, 0, 0 };

   std::vector<uint16_t*> gUnicodeIndexHash{5};
   std::vector<uint8_t*> gBitmapData{5};
  // These are for the metrics for each individual glyph (so we don't need to seek this in file and waste time)
   std::vector<uint16_t*> gUnicode{5};  //UTF-16 code, the codes are searched so do not need to be sequential
   std::vector<uint8_t*>  gHeight{5};   //cheight
   std::vector<uint8_t*>  gWidth{5};    //cwidth
   std::vector<uint8_t*>  gxAdvance{5}; //setWidth
   std::vector<int8_t*>   gdY{5};       //topExtent
   std::vector<int8_t*>   gdX{5};       //leftExtent
   std::vector<uint32_t*> gBitmap{5};   //file pointer to greyscale bitmap

  std::vector<String>   _gFontFilename;

  uint8_t  decoderState = 0;   // UTF8 decoder state
  uint16_t decoderBuffer;      // Unicode code-point buffer

  bool     fontLoaded = false; // Flags when a anti-aliased font is loaded

 private:

  void     loadMetrics(uint16_t gCount, size_t i);
  uint32_t readInt32(size_t i);
