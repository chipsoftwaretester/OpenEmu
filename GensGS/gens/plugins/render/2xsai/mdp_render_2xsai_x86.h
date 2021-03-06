/***************************************************************************
 * Gens: [MDP] 2xSaI renderer. (x86 asm function prototypes)               *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008 by David Korth                                       *
 * 2xSaI Copyright (c) by Derek Liauw Kie Fa and Robert J. Ohannessian     *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation; either version 2.1 of the License, or  *
 * (at your option) any later version.                                     *
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

#ifndef GENS_MDP_RENDER_2XSAI_X86_H
#define GENS_MDP_RENDER_2XSAI_X86_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void mdp_render_2xsai_16_x86_mmx(uint16_t *destScreen, uint16_t *mdScreen,
				 int destPitch, int srcPitch,
				 int width, int height, int mode555);

#ifdef __cplusplus
}
#endif

#endif /* GENS_MDP_RENDER_2XSAI_X86_H */
