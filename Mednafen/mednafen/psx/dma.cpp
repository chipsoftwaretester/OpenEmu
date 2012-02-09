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

#include "psx.h"
#include "mdec.h"
#include "cdc.h"
#include "spu.h"

// FIXME: 0-length block count?


/* Notes:

 Channel 6:
	DMA hangs if D28 is 0?
	D1 did not have an apparent effect.

*/

enum
{
 CH_MDEC_IN = 0,
 CH_MDEC_OUT = 1,
 CH_GPU = 2,
 CH_CDC = 3,
 CH_SPU = 4,
 CH_OT = 6,
};


// RunChannels(128 - whatevercounter);
//
// GPU next event, std::max<128, wait_time>, or something similar, for handling FIFO.

namespace MDFN_IEN_PSX
{

static int32 DMACycleCounter;

static uint32 DMAControl;
static uint32 DMAIntControl;
static uint8 DMAIntStatus;
static bool IRQOut;

struct Channel
{
 uint32 BaseAddr;
 uint32 BlockControl;
 uint32 ChanControl;

 //
 //
 //
 uint32 CurAddr;
 uint32 NextAddr;

 uint16 BlockCounter;
 uint16 WordCounter; 

 int32 ClockCounter;
};

static Channel DMACH[7];
static pscpu_timestamp_t lastts;


static const char *PrettyChannelNames[7] = { "MDEC IN", "MDEC OUT", "GPU", "CDC", "SPU", "PIO", "OTC" };

void DMA_Init(void)
{

}

void DMA_Kill(void)
{

}

static INLINE void RecalcIRQOut(void)
{
 bool irqo;

 irqo = (bool)(DMAIntStatus & ((DMAIntControl >> 16) & 0x7F));
 irqo &= (DMAIntControl >> 23) & 1;

 // I think it's logical OR, not XOR/invert.  Still kind of weird, maybe it actually does something more complicated?
 //irqo ^= (DMAIntControl >> 15) & 1;
 irqo |= (DMAIntControl >> 15) & 1;

 IRQOut = irqo;
 IRQ_Assert(IRQ_DMA, irqo);
}

void DMA_ResetTS(void)
{
 lastts = 0;
}

void DMA_Power(void)
{
 lastts = 0;

 memset(DMACH, 0, sizeof(DMACH));

 DMACycleCounter = 128;

 DMAControl = 0;
 DMAIntControl = 0;
 DMAIntStatus = 0;
 RecalcIRQOut();
}

static void RecalcHalt(void)
{
 bool Halt = false;

 if((DMACH[0].WordCounter || (DMACH[0].ChanControl & (1 << 24))) && (DMACH[0].ChanControl & 0x200) /*&& MDEC_DMACanWrite()*/)
  Halt = true;

 if((DMACH[1].WordCounter || (DMACH[1].ChanControl & (1 << 24))) && (DMACH[1].ChanControl & 0x200) && MDEC_DMACanRead())
  Halt = true;

 if((DMACH[2].WordCounter || (DMACH[2].ChanControl & (1 << 24))) && (DMACH[2].ChanControl & 0x200) && ((DMACH[2].ChanControl & 0x1) && GPU->DMACanWrite()))
  Halt = true;

 if((DMACH[3].WordCounter || (DMACH[3].ChanControl & (1 << 24))))
  Halt = true;

 //printf("Halt: %d\n", Halt);

 CPU->SetHalt(Halt);
}

template<int ch, bool write_mode>
static INLINE bool ChCan(void)
{
 switch(ch)
 {
  case 0: return(true);	// MDEC IN
  case 1: return(MDEC_DMACanRead());	// MDEC out
  case 2: 
	  if(write_mode)
	   return(GPU->DMACanWrite());
	  else
	   return(true);	// GPU
  case 3: return(true);	// CDC
  case 4: return(true);	// SPU
  case 5: return(true);	// ??
  case 6: return(true);	// OT
 }
 abort();
}

template<int ch, bool write_mode>
static INLINE void ChRW(int32 timestamp, uint32 *V)
{
 switch(ch)
 {
  default:
	abort();

  case CH_MDEC_IN:
	  MDEC_DMAWrite(*V);
	  break;

  case CH_MDEC_OUT:
	  *V = MDEC_DMARead();
	  break;

  case CH_GPU:
	  if(write_mode)
	   GPU->WriteDMA(*V);
	  else
	   *V = GPU->Read(timestamp, 0);
	  break;

  case CH_CDC:
	  *V = CDC->DMARead();
	  break;

  case CH_SPU:
	  DMACH[ch].ClockCounter -= 7;
	  if(write_mode)
	   SPU->WriteDMA(*V);
	  else
	   *V = SPU->ReadDMA();
	  break;

  case CH_OT:
	  if(DMACH[ch].WordCounter == 1)
	   *V = 0xFFFFFF;
	  else
	   *V = (DMACH[ch].CurAddr - 4) & 0x1FFFFF;
	  break;
 }
}

template<int ch, bool write_mode>
static INLINE void RunChannelT(pscpu_timestamp_t timestamp, int32 clocks)
{
 //const uint32 dc = (DMAControl >> (ch * 4)) & 0xF;

 DMACH[ch].ClockCounter += clocks;

 while(DMACH[ch].ClockCounter > 0)
 {
  if(!DMACH[ch].WordCounter)
  {
   if(!(DMACH[ch].ChanControl & (1 << 24)))
   {
    break;
   }

   if(DMACH[ch].NextAddr & 0x800000)
   {
    DMACH[ch].ChanControl &= ~(1 << 24);
    if(DMAIntControl & (1 << (16 + ch)))
    {
     DMAIntStatus |= 1 << ch;
     RecalcIRQOut();
    }
    break;
   }

   if((DMACH[ch].ChanControl & (1 << 10)) && write_mode)
   {
    uint32 header;
    uint32 null_bull = 0;

    if(!ChCan<ch, write_mode>())
     break;

    ChRW<ch, write_mode>(timestamp, &null_bull);

    DMACH[ch].CurAddr = DMACH[ch].NextAddr & 0x1FFFFC;
    header = *(uint32 *)&MainRAM[DMACH[ch].CurAddr];
    DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;

    DMACH[ch].WordCounter = header >> 24;
    DMACH[ch].NextAddr = header & 0xFFFFFF;

    if(DMACH[ch].WordCounter)
     DMACH[ch].ClockCounter -= 15;
    else
     DMACH[ch].ClockCounter -= 10;

    continue;
   }
   else
   {
    DMACH[ch].CurAddr = DMACH[ch].NextAddr & 0x1FFFFC;
    DMACH[ch].WordCounter = DMACH[ch].BlockControl & 0xFFFF;
    DMACH[ch].BlockCounter--;

    if(!DMACH[ch].BlockCounter || ch == 6 || ch == 3)
     DMACH[ch].NextAddr = 0xFFFFFF;
    else
     DMACH[ch].NextAddr = (DMACH[ch].CurAddr + ((DMACH[ch].BlockControl & 0xFFFF) << 2)) & 0x1FFFFF;
   }
  }

  if(!ChCan<ch, write_mode>())
   break;

  ChRW<ch, write_mode>(timestamp, (uint32*)&MainRAM[DMACH[ch].CurAddr]);

  if(ch == 6)
   DMACH[ch].CurAddr = (DMACH[ch].CurAddr - 4) & 0x1FFFFF;
  else
   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;

  DMACH[ch].WordCounter--;
  DMACH[ch].ClockCounter--;
 }


 if(DMACH[ch].ClockCounter > 0)
  DMACH[ch].ClockCounter = 0;
}

static INLINE void RunChannel(pscpu_timestamp_t timestamp, int32 clocks, int ch)
{
 switch(ch)
 {
  default: abort();

  case 0:
	RunChannelT<0, true>(timestamp, clocks);
	break;

  case 1:
	RunChannelT<1, false>(timestamp, clocks);
	break;

  case 2:
	if(DMACH[2].ChanControl & 0x1)
  	 RunChannelT<2, true>(timestamp, clocks);
	else
	 RunChannelT<2, false>(timestamp, clocks);
	break;

  case 3:
	RunChannelT<3, false>(timestamp, clocks);
	break;

  case 4:
	if(DMACH[4].ChanControl & 0x1)
 	RunChannelT<4, true>(timestamp, clocks);
	else
	 RunChannelT<4, false>(timestamp, clocks);
	break;

  case 6:
	RunChannelT<6, false>(timestamp, clocks);
	break;
 }
}

#if 0
static INLINE void RunChannel(const pscpu_timestamp_t timestamp, int ch, int32 clocks)
{
 int32 simu_time = 0;

   switch(ch)
   {
    case 0x0:	// MDEC in
	simu_time = -1;
	while(DMACH[ch].Counter /*&& MDEC_DMACanWrite()*/ && clocks > 0)
	{
	 MDEC_DMAWrite(*(uint32 *)&MainRAM[DMACH[ch].CurAddr]);

	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	 clocks--;
	}

	if(!DMACH[ch].Counter)
	{
	 DMACH[ch].ChanControl &= ~(1 << 24);
	}
	break;

  
    case 0x1:	// MDEC out
	simu_time = -1;
	while(DMACH[ch].Counter && MDEC_DMACanRead() && clocks > 0)
	{
	 uint32 data;

	 MDEC_DMARead(data);

	 *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = data;

	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	 clocks -= 1;
	}

	if(!DMACH[ch].Counter)
	{
	 DMACH[ch].ChanControl &= ~(1 << 24);
	}
	break;

    case 0x3: // CDC
#if 1
	simu_time = DMACH[ch].Counter;
	while(DMACH[ch].Counter)
	{
	 uint32 data;

	 CDC->DMARead(data);

	 *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = data;
	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	}
#else
	simu_time = -1;

	while(DMACH[ch].Counter && clocks > 0)
	{
	 uint32 data;

	 CDC->DMARead(data);
	 clocks--;

	 *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = data;
	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	}

	if(!DMACH[ch].Counter)
	{
	 DMACH[ch].ChanControl &= ~(1 << 24);
	}

#endif
	break;


    case 0x2:
	DMACH[ch].ClockCounter += clocks;
	GPU->Update(timestamp);
	if(DMACH[ch].ChanControl & (1 << 10))	// Linked list
	{
	 bool Finished = false;

	 simu_time = -1;

	 while(!Finished && GPU->DMACanWrite() && DMACH[ch].ClockCounter > 0) // && GPU->DTAGetWaitKludge() == 0) //GPU->DMACanWrite())
	 {
	  if(!DMACH[ch].Counter)
	  {
	   if(DMACH[ch].NextAddr & 0x800000)
	   {
	    Finished = true;
	    break;
	   }

	   DMACH[ch].CurAddr = DMACH[ch].NextAddr & 0x1FFFFC;

	   DMACH[ch].ClockCounter--;
	   uint32 header = *(uint32 *)&MainRAM[DMACH[ch].CurAddr];
	   GPU->WriteDMA(0);

	   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;

	   DMACH[ch].Counter = header >> 24;
	   DMACH[ch].NextAddr = header & 0xFFFFFF;


	   if(DMACH[ch].Counter)
	    DMACH[ch].ClockCounter -= 15;
	   else
	    DMACH[ch].ClockCounter -= 10;

	   continue;
	  }

	  DMACH[ch].ClockCounter--;
	  GPU->WriteDMA(*(uint32 *)&MainRAM[DMACH[ch].CurAddr]);

          DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	  DMACH[ch].Counter--;
	 }

         if(Finished)
         {
	  DMACH[ch].ChanControl &= ~(1 << 24);
         }
	}
	else
	{
	 simu_time = -1;

	 if(DMACH[ch].ChanControl & 0x1) // Write to GPU
	 {
	  while(DMACH[ch].Counter && GPU->DMACanWrite() && clocks > 0)
	  {
	   GPU->WriteDMA(*(uint32 *)&MainRAM[DMACH[ch].CurAddr]);

	   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	   DMACH[ch].Counter--;
		clocks--;
	  }
	  if(!DMACH[ch].Counter)
	  {
  	   DMACH[ch].ChanControl &= ~(1 << 24);
  	  }
	 }
	 else // Read from GPU
	 {
 	  simu_time = DMACH[ch].Counter * 2;
	  while(DMACH[ch].Counter)
	  {
	   *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = GPU->Read(timestamp, 0);
	   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	   DMACH[ch].Counter--;
	  }
	 }
	}


	if(DMACH[ch].ClockCounter > 0)
	 DMACH[ch].ClockCounter = 0;
	break;

    case 0x4:
	 simu_time = -1;
	 DMACH[ch].ClockCounter += clocks;

	 if(DMACH[ch].ChanControl & 0x1) // Write to SPU
	 {
	  while(DMACH[ch].Counter && DMACH[ch].ClockCounter > 0)
	  {
	   SPU->WriteDMA(*(uint32 *)&MainRAM[DMACH[ch].CurAddr]);
	   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	   DMACH[ch].Counter--;
	   DMACH[ch].ClockCounter -= 8;
	  }
	 }
	 else // Read from SPU
	 {
	  while(DMACH[ch].Counter && clocks > 0)
	  {
	   *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = SPU->ReadDMA();

	   DMACH[ch].CurAddr = (DMACH[ch].CurAddr + 4) & 0x1FFFFF;
	   DMACH[ch].Counter--;
	   DMACH[ch].ClockCounter -= 8;
	  }
	 }

	 if(!DMACH[ch].Counter)
	 {
  	  DMACH[ch].ChanControl &= ~(1 << 24);
 	 }
	 break;

    case 0x6:
	simu_time = -1;

	while(DMACH[ch].Counter > 1 && clocks > 0)
	{
	 //printf("OT: %08x %08x\n", DMACH[ch].CurAddr, (DMACH[ch].CurAddr - 4) & 0x1FFFFF);
	 *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = (DMACH[ch].CurAddr - 4) & 0x1FFFFF;

	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr - 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	 clocks -= 1;
	}

	if(DMACH[ch].Counter == 1 && clocks > 0)
	{
	 *(uint32 *)&MainRAM[DMACH[ch].CurAddr] = 0xFFFFFF;

	 DMACH[ch].CurAddr = (DMACH[ch].CurAddr - 4) & 0x1FFFFF;
	 DMACH[ch].Counter--;
	 clocks -= 1;
	}

	if(!DMACH[ch].Counter)
	 DMACH[ch].ChanControl &= ~(1 << 24);
	break;
   }

 if(!(DMACH[ch].ChanControl & (1 << 24)))
 {
  if(DMAIntControl & (1 << (16 + ch)))
  {
   DMAIntStatus |= 1 << ch;
   RecalcIRQOut();
  }
  //printf("DMA Channel %d Done --- int_status=0x%08x--- %d\n", ch, DMAIntStatus, GPU->GetScanlineNum());
 }
 else if(simu_time >= 0)
 {
  if(!simu_time)	// For 0-length DMA.
  {
   puts("HMM");
   simu_time = 1;
  }

  DMACH[ch].SimuFinishDelay = simu_time;
 }
}
#endif

static INLINE int32 CalcNextEvent(int32 next_event)
{
 if(DMACycleCounter < next_event)
  next_event = DMACycleCounter;

 return(next_event);
}

pscpu_timestamp_t DMA_Update(const pscpu_timestamp_t timestamp)
{
//   uint32 dc = (DMAControl >> (ch * 4)) & 0xF;
 int32 clocks = timestamp - lastts;
 lastts = timestamp;

 GPU->Update(timestamp);
 MDEC_Run(clocks);

 RunChannel(timestamp, clocks, 0);
 RunChannel(timestamp, clocks, 1);
 RunChannel(timestamp, clocks, 2);
 RunChannel(timestamp, clocks, 3);
 RunChannel(timestamp, clocks, 4);
 RunChannel(timestamp, clocks, 6);

 DMACycleCounter -= clocks;
 while(DMACycleCounter <= 0)
  DMACycleCounter += 128;

 RecalcHalt();

 return(timestamp + CalcNextEvent(0x10000000));
}

void DMA_Write(const pscpu_timestamp_t timestamp, uint32 A, uint32 V)
{
 int ch = (A & 0x7F) >> 4;

 //PSX_WARNING("[DMA] Write: %08x %08x, DMAIntStatus=%08x", A, V, DMAIntStatus);

 // FIXME if we ever have "accurate" bus emulation
 V <<= (A & 3) * 8;

 DMA_Update(timestamp);

 if(ch == 7)
 {
  switch(A & 0xC)
  {
   case 0x0: //fprintf(stderr, "Global DMA control: 0x%08x\n", V);
	     DMAControl = V;
	     RecalcHalt();
	     break;

   case 0x4: 
	     //for(int x = 0; x < 7; x++)
	     //{
             // if(DMACH[x].WordCounter || (DMACH[x].ChanControl & (1 << 24)))
	     // {
	     //  fprintf(stderr, "Write DMAIntControl while channel %d active: 0x%08x\n", x, V);
	     // }
	     //}
	     DMAIntControl = V & 0x00ff803f;
	     DMAIntStatus &= ~(V >> 24);

	     //if(DMAIntStatus ^ (DMAIntStatus & (V >> 16)))
	     // fprintf(stderr, "DMAINT Fudge: %02x\n", DMAIntStatus ^ (DMAIntStatus & (V >> 16)));
	     DMAIntStatus &= (V >> 16);	// THIS IS ALMOST CERTAINLY WRONG AND A HACK.  Remove when CDC emulation is better.
     	     RecalcIRQOut();
	     break;

   default: PSX_WARNING("[DMA] Unknown write: %08x %08x", A, V);
		assert(0);
	    break;
  }
  return;
 }
 switch(A & 0xC)
 {
  case 0x0: DMACH[ch].BaseAddr = V & 0xFFFFFF;
	    break;

  case 0x4: DMACH[ch].BlockControl = V;
	    break;

  case 0xC:
  case 0x8: 
	   {
	    uint32 OldCC = DMACH[ch].ChanControl;

	    //
            // Kludge for DMA timing granularity and other issues.  Needs to occur before setting all bits of ChanControl to the new value, to accommodate the
	    // case of a game cancelling DMA and changing the type of DMA(read/write, etc.) at the same time.
            //
	    if((DMACH[ch].ChanControl & (1 << 24)) && !(V & (1 << 24)))
	    {
	     DMACH[ch].ChanControl &= ~(1 << 24);	// Clear bit before RunChannel(), so it will only finish the block it's on at most.
	     RunChannel(timestamp, 512, ch);

	     PSX_WARNING("[DMA] Forced stop for channel %d -- scanline=%d", ch, GPU->GetScanlineNum());
	     MDFN_DispMessage("[DMA] Forced stop for channel %d", ch);
	    }

	    if(ch == 6)
	     DMACH[ch].ChanControl = V & 0x51000002; 	// Not 100% sure, but close.
	    else
	     DMACH[ch].ChanControl = V & 0x71770703;

	    if(!(OldCC & (1 << 24)) && (V & (1 << 24)))
	    {
	     //PSX_WARNING("[DMA] Started DMA for channel=%d --- CHCR=0x%08x --- BCR=0x%08x --- scanline=%d", ch, DMACH[ch].ChanControl, DMACH[ch].BlockControl, GPU->GetScanlineNum());

	     DMACH[ch].ClockCounter = 0;
	     DMACH[ch].NextAddr = DMACH[ch].BaseAddr & 0x1FFFFC;
	     DMACH[ch].BlockCounter = DMACH[ch].BlockControl >> 16;

	     //
	     // Viewpoint starts a short MEM->GPU LL DMA and apparently has race conditions that can cause a crash if it doesn't finish almost immediately(
	     // or at least very quickly, which the current DMA granularity has issues with, so run the channel ahead a bit to take of this issue and potentially
	     // games with similar issues).
	     //
	     // Though, Viewpoint isn't exactly a good game, so maybe we shouldn't bother? ;)
	     //
	     RunChannel(timestamp, 64, ch);	//std::max<int>(128 - DMACycleCounter, 1)); //64); //1); //128 - DMACycleCounter);
	    }

	    RecalcHalt();
	   }
	   break;
 }
 PSX_SetEventNT(PSX_EVENT_DMA, timestamp + CalcNextEvent(0x10000000));
}

uint32 DMA_Read(const pscpu_timestamp_t timestamp, uint32 A)
{
 int ch = (A & 0x7F) >> 4;
 uint32 ret = 0;
 
 //assert(!(A & 3));

 //if(ch == 2)
 // printf("DMA Read: %08x --- %d\n", A, GPU->GetScanlineNum());

 if(ch == 7)
 {
  switch(A & 0xC)
  {
   default: PSX_WARNING("[DMA] Unknown read: %08x", A);
		assert(0);
	    break;

   case 0x0: ret = DMAControl;
	     break;

   case 0x4: ret = DMAIntControl | (DMAIntStatus << 24) | (IRQOut << 31);
	     break;
  }
 }
 else switch(A & 0xC)
 {
  case 0x0: ret = DMACH[ch].BaseAddr;
  	    break;

  case 0x4: ret = DMACH[ch].BlockControl;
	    break;

  case 0xC:
  case 0x8: ret = DMACH[ch].ChanControl;
            break;

 }

 ret >>= (A & 3) * 8;

 //PSX_WARNING("[DMA] Read: %08x %08x", A, ret);

 return(ret);
}



}
