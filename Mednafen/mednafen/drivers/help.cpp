/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "main.h"

#include <string.h>

static bool IsActive;

#define MK_COLOR_A(r,g,b,a) ( ((a)<<surface->format->Ashift) | ((r)<<surface->format->Rshift) | ((g) << surface->format->Gshift) | ((b) << surface->format->Bshift))

void Help_Draw(SDL_Surface *surface, const SDL_Rect *rect)
{
 if(!IsActive) return;

 uint32 * pixels = (uint32 *)surface->pixels;
 uint32 pitch32 = surface->pitch >> 2;

 for(unsigned int y = 0; y < rect->h; y++)
 {
  uint32 *row = pixels + y * pitch32;
  for(unsigned int x = 0; x < rect->w; x++)
   row[x] = MK_COLOR_A(0, 0, 0, 0xFF); //0xE0);
 }
 static const char *HelpStrings[][2] = { 
	{ "F1", gettext_noop("Toggle Help") },
	{ "-", NULL },
	{ "F2", gettext_noop("Configure Command Key") },
	{ "ALT + SHIFT + [n]", gettext_noop("Configure buttons on port n(1-5)") },
	{ "CTRL + SHIFT + [n]", gettext_noop("Select device on port n") },
	{ "-", NULL },
	{ "ALT + O", gettext_noop("Rotate Screen") },
	{ "ALT + ENTER", gettext_noop("Toggle Fullscreen Mode") },
	{ "F9", gettext_noop("Take Screen Snapshot") },
	{ "-", NULL },
	{ "ALT + S", gettext_noop("Enable state rewinding") },
	{ "BACKSPACE", gettext_noop("Rewind") },
	{ "F5", gettext_noop("Save State") },
	{ "F6", gettext_noop("Select Disk") },
	{ "F7", gettext_noop("Load State") },
	{ "F8", gettext_noop("Insert coin; Insert/Eject disk") },
	{ "-", NULL },
	{ "F10", gettext_noop("(Soft) Reset, if available on emulated system.") },
	{ "F11", gettext_noop("Power Toggle/Hard Reset") },
	{ "F12", gettext_noop("Exit") },
};

 unsigned y = 0;

 DrawTextTrans(pixels + pitch32 * y, surface->pitch, rect->w, (UTF8*)_("Mednafen Quickie Help"), MK_COLOR_A(0x00,0xFF,0x00,0xFF), TRUE, MDFN_FONT_9x18_18x18);
 y += 30;

 DrawTextTrans(pixels + pitch32 * y, surface->pitch, rect->w, (UTF8 *)_("Default key assignments:"), MK_COLOR_A(0xC0,0xC0,0xC0,0xFF), false, MDFN_FONT_9x18_18x18);
 y += 18;

 for(unsigned int i = 0; i < sizeof(HelpStrings) / sizeof(HelpStrings[0]); i++)
 {
  if((y + 18) > rect->h)
   break;

  unsigned x = 0;
  if(HelpStrings[i][0][0] == '-')
  {
   y -= 4;
   DrawTextTrans(pixels + pitch32 * y, surface->pitch, rect->w, (UTF8*)" -------------------------------------------------------", MK_COLOR_A(0x60,0x60,0x60,0xFF), FALSE, MDFN_FONT_9x18_18x18);
   y += 14;
  }
  else
  {
   x += 9;

   x += DrawTextTrans(pixels + pitch32 * y + x, surface->pitch, rect->w - x, (UTF8*)HelpStrings[i][0],
	MK_COLOR_A(0x40,0xDF,0x40,0xFF), FALSE, MDFN_FONT_9x18_18x18);

   x += DrawTextTrans(pixels + pitch32 * y + x, surface->pitch, rect->w - x, (UTF8*)" - ",
        MK_COLOR_A(0x40,0x40,0x40,0xFF), FALSE, MDFN_FONT_9x18_18x18);

   x += DrawTextTrans(pixels + pitch32 * y + x, surface->pitch, rect->w - x, (UTF8*)_(HelpStrings[i][1]),
        MK_COLOR_A(0xC0,0xC0,0xC0,0xFF), FALSE, MDFN_FONT_9x18_18x18);

   y += 18;
  }

 }
}

bool Help_IsActive(void)
{

 return(IsActive);
}

bool Help_Toggle(void)
{
 IsActive = !IsActive;
 return(IsActive);
}

void Help_Init(void)
{
 IsActive = false;
}

void Help_Close(void)
{

}
