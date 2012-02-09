#ifndef __MDFN_DRIVERS_INPUT_CONFIG_H
#define __MDFN_DRIVERS_INPUT_CONFIG_H

int DTestButton(std::vector<ButtConfig> &bc, const char *KeyState, const uint32* MouseData, bool analog = false);
int DTestButton(ButtConfig &bc, const char *KeyState, const uint32 *MouseData, bool analog = false);

int DTestButtonCombo(std::vector<ButtConfig> &bc, const char *KeyState, const uint32 *MouseData);
int DTestButtonCombo(ButtConfig &bc, const char *KeyState, const uint32 *MouseData);

// TODO/WIP: joy event type filter stuff
#define ICJF_DIGITAL_BUTTON	0x1
//#define ICJF_ANALOG_BUTTON	0x2
#define ICJF_HAT		0x4
#define ICJF_AXIS		0x8
#define ICJF_ALL		(~0U)

int DTryButtonBegin(ButtConfig *bc, int commandkey, unsigned int jf = ICJF_ALL);
int DTryButton(void);
int DTryButtonEnd(ButtConfig *bc);

#endif
