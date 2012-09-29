/***************************************************************************
 * Gens: (Win32) Select CD-ROM Drive Window.                               *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008 by David Korth                                       *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef GENS_WIN32_SELECT_CDROM_WINDOW_H
#define GENS_WIN32_SELECT_CDROM_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

HWND create_select_cdrom_window(void); 
extern HWND select_cdrom_window;
void Select_CDROM_Window_CreateChildWindows(HWND hWnd);

// Controls
extern HWND SelCD_cdromDropdownBox;
extern HWND SelCD_btnOK;
extern HWND SelCD_btnApply;

#ifdef __cplusplus
}
#endif

#endif /* GENS_WIN32_SELECT_CDROM_WINDOW_H */