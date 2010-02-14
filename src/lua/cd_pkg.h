#ifndef CD_PKG_H
#define CD_PKG_H

//tolua_begin

// -----------------------------------------------------------------------------
// graphics: CD library definitions
// -----------------------------------------------------------------------------
#define  CD_RED           0xFF0000L   /* 255,  0,  0 */
#define  CD_DARK_RED      0x800000L   /* 128,  0,  0 */
#define  CD_GREEN         0x00FF00L   /*   0,255,  0 */
#define  CD_DARK_GREEN    0x008000L   /*   0,128,  0 */
#define  CD_BLUE          0x0000FFL   /*   0,  0,255 */
#define  CD_DARK_BLUE     0x000080L   /*   0,  0,128 */

#define  CD_YELLOW        0xFFFF00L   /* 255,255,  0 */
#define  CD_DARK_YELLOW   0x808000L   /* 128,128,  0 */
#define  CD_MAGENTA       0xFF00FFL   /* 255,  0,255 */
#define  CD_DARK_MAGENTA  0x800080L   /* 128,  0,128 */
#define  CD_CYAN          0x00FFFFL   /*   0,255,255 */
#define  CD_DARK_CYAN     0x008080L   /*   0,128,128 */

#define  CD_WHITE         0xFFFFFFL   /* 255,255,255 */
#define  CD_BLACK         0x000000L   /*   0,  0,  0 */

#define  CD_DARK_GRAY     0x808080L   /* 128,128,128 */
#define  CD_GRAY          0xC0C0C0L   /* 192,192,192 */

typedef enum {                          /* background opacity mode */
  CD_OPAQUE,
  CD_TRANSPARENT
} cd_opacity;

typedef enum {                          /* write mode */
  CD_REPLACE,
  CD_XOR,
  CD_NOT_XOR
} cd_writemode;

typedef enum {                          /* line style */
  CD_CONTINUOUS,
  CD_DASHED,
  CD_DOTTED,
  CD_DASH_DOT,
  CD_DASH_DOT_DOT
} cd_linestyle;

enum {                          /* marker type */
 CD_PLUS,
 CD_STAR,
 CD_CIRCLE,
 CD_X,
 CD_BOX,
 CD_DIAMOND,
 CD_HOLLOW_CIRCLE,
 CD_HOLLOW_BOX,
 CD_HOLLOW_DIAMOND
} cd_marker;

enum {                          /* hatch type */
 CD_HORIZONTAL,
 CD_VERTICAL,
 CD_FDIAGONAL,
 CD_BDIAGONAL,
 CD_CROSS,
 CD_DIAGCROSS
};

enum {                          /* interior style */
 CD_SOLID,
 CD_HATCH,
 CD_STIPPLE,
 CD_PATTERN,
 CD_HOLLOW
};

enum {                          /* text alignment */
 CD_NORTH,
 CD_SOUTH,
 CD_EAST,
 CD_WEST,
 CD_NORTH_EAST,
 CD_NORTH_WEST,
 CD_SOUTH_EAST,
 CD_SOUTH_WEST,
 CD_CENTER,
 CD_BASE_LEFT,
 CD_BASE_CENTER,
 CD_BASE_RIGHT
};

enum {                          /* style */
 CD_PLAIN  = 0,
 CD_BOLD   = 1,
 CD_ITALIC = 2,
 CD_BOLD_ITALIC = 3,
 CD_UNDERLINE = 4,
 CD_STRIKEOUT = 8,
} cd_textstyle;

typedef enum {                          /* font size */
  CD_SMALL    =  8,
  CD_STANDARD = 12,
  CD_LARGE    = 18
} cd_testsize;

//tolua_end
#endif

