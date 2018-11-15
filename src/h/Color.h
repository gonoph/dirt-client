#ifndef DIRT_COLOR_H
#define DIRT_COLOR_H

enum {
	fg_black,
	fg_blue,
	fg_green,
	fg_cyan,
	fg_red,
	fg_magenta,
	fg_yellow,
	fg_white,
	fg_bold
};

enum
{
	bg_white 	= fg_white << 4,
	bg_black 	= fg_black << 4,
	bg_red 		= fg_red << 4,
	bg_blue 	= fg_blue << 4,
	bg_green 	= fg_green << 4,
	bg_magenta 	= fg_magenta << 4,
	bg_yellow 	= fg_yellow << 4,
	bg_cyan 	= fg_cyan << 4,
	fg_blink	= fg_bold << 4
};

// 0xEA and above are special characters for dirt
// too bad for those with national characters :(

// ugh, can't change those, would break backwards compat

#define SET_COLOR		0xEA	// Next char after this is color
#define SOFT_CR			0xEB	// Change to a new line, but only if not at the
                                // beginning of one already

#define COL_SAVE        0xFF    // Save the current color
#define COL_RESTORE     0xFE    // Restore the saved color

#define SC_BASE         0xEC    // Special characters take up EC to...
#define SC_END          ( SC_BASE + max_sc) // this, which is +8, so Fsomething

								
#endif
