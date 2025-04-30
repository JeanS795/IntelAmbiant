#define LIB_MAGIC_CPP
#include "lib_magic.h"

#include <Wire.h>
#include <Arduino.h>
#include "ht1632.h"
#include <avr/pgmspace.h>

// Variables globales
byte ht1632_shadowram[64][4] = {0};
const unsigned char Tab7Segts[] = {0x7E,0x06,0xDA,0x9E,0xA6,0xBC,0xFC,0x0E,0xFE,0xBE};




//**************************************************************************************************
//Function Name: OutputCLK_Pulse
//Function Feature: enable CLK_74164 pin to output a clock pulse
//Input Argument: void
//Output Argument: void
//**************************************************************************************************
void OutputCLK_Pulse(void) //Output a clock pulse
{
  digitalWrite(ht1632_clk, HIGH);
  digitalWrite(ht1632_clk, LOW);
}


//**************************************************************************************************
//Function Name: OutputA_74164
//Function Feature: enable pin A of 74164 to output 0 or 1
//Input Argument: x: if x=1, 74164 outputs high. If x?1, 74164 outputs low.
//Output Argument: void
//**************************************************************************************************
void OutputA_74164(unsigned char x) //Input a digital level to 74164
{
  digitalWrite(ht1632_cs, (x==1 ? HIGH : LOW));
}


//**************************************************************************************************
//Function Name: ChipSelect
//Function Feature: enable HT1632C
//Input Argument: select: HT1632C to be selected
// If select=0, select none.
// If s<0, select all.
//Output Argument: void
//**************************************************************************************************
void ChipSelect(int select)
{
  unsigned char tmp = 0;
  if(select<0) //Enable all HT1632Cs
  {
    OutputA_74164(0);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else if(select==0) //Disable all HT1632Cs
  {
    OutputA_74164(1);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else
  {
    OutputA_74164(1);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
    OutputA_74164(0);
    CLK_DELAY;
    OutputCLK_Pulse();
    CLK_DELAY;
    OutputA_74164(1);
    CLK_DELAY;
    tmp = 1;
    for( ; tmp<select; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
}


/*
 * ht1632_writebits
 * Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk
 * Chip is assumed to already be chip-selected
 * Bits are shifted out from MSB to LSB, with the first bit sent
 * being (bits & firstbit), shifted till firsbit is zero.
 */
void ht1632_writebits (byte bits, byte firstbit)
{
  DEBUGPRINT(" ");
  while (firstbit) {
    DEBUGPRINT((bits&firstbit ? "1" : "0"));
    digitalWrite(ht1632_wrclk, LOW);
    if (bits & firstbit) {
      digitalWrite(ht1632_data, HIGH);
    } 
    else {
      digitalWrite(ht1632_data, LOW);
    }
    digitalWrite(ht1632_wrclk, HIGH);
    firstbit >>= 1;
  }
}


/*
 * ht1632_sendcmd
 * Send a command to the ht1632 chip.
 */
static void ht1632_sendcmd (byte chipNo, byte command)
{
  ChipSelect(chipNo);
  ht1632_writebits(HT1632_ID_CMD, 1<<2);  // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1<<7);  // send the actual command
  ht1632_writebits(0, 1); 	/* one extra dont-care bit in commands. */
  ChipSelect(0);
}


/*
 * ht1632_senddata
 * send a nibble (4 bits) of data to a particular memory location of the
 * ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
 *    Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
 * Note that the address is sent MSB first, while the data is sent LSB first!
 * This means that somewhere a bit reversal will have to be done to get
 * zero-based addressing of words and dots within words.
 */
static void ht1632_senddata (byte chipNo, byte address, byte data)
{
  ChipSelect(chipNo);
  ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
  ht1632_writebits(address, 1<<6); // Send address
  ht1632_writebits(data, 1<<3); // send 4 bits of data
  ChipSelect(0);
}


void ht1632_setup()
{
  pinMode(ht1632_cs, OUTPUT);
  digitalWrite(ht1632_cs, HIGH); 	/* unselect (active low) */
  pinMode(ht1632_wrclk, OUTPUT);
  pinMode(ht1632_data, OUTPUT);
  pinMode(ht1632_clk, OUTPUT);

  for (int j=1; j<5; j++)
  {
    ht1632_sendcmd(j, HT1632_CMD_SYSDIS);  // Disable system
    ht1632_sendcmd(j, HT1632_CMD_COMS00);
    ht1632_sendcmd(j, HT1632_CMD_MSTMD); 	/* Master Mode */
    ht1632_sendcmd(j, HT1632_CMD_RCCLK);  // HT1632C
    ht1632_sendcmd(j, HT1632_CMD_SYSON); 	/* System on */
    ht1632_sendcmd(j, HT1632_CMD_LEDON); 	/* LEDs on */
  }
  
  for (byte i=0; i<96; i++)
  {
    ht1632_senddata(1, i, 0);  // clear the display!
    ht1632_senddata(2, i, 0);  // clear the display!
    ht1632_senddata(3, i, 0);  // clear the display!
    ht1632_senddata(4, i, 0);  // clear the display!
  }
  delay(LONGDELAY);
}


/*
 * plot a point on the display, with the upper left hand corner
 * being (0,0), and the lower right hand corner being (31, 15);
 * parameter "color" could have one of the 4 values:
 * black (off), red, green or yellow;
 */
void ht1632_plot (byte x, byte y, byte color)
{
  if (x<0 || x>X_MAX || y<0 || y>Y_MAX)
    return;
  
  if (color != BLACK && color != GREEN && color != RED && color != ORANGE)
    return;
  
  byte nChip = 1 + x/16 + (y>7?2:0) ;
  x = x % 16;
  y = y % 8;
  byte addr = (x<<1) + (y>>2);
  byte bitval = 8>>(y&3);  // compute which bit will need set
  switch (color)
  {
    case BLACK:
      // clear the bit in both planes;
      ht1632_shadowram[addr][nChip-1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip-1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      break;
    case GREEN:
      // set the bit in the green plane and clear the bit in the red plane;
      ht1632_shadowram[addr][nChip-1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip-1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      break;
    case RED:
      // clear the bit in green plane and set the bit in the red plane;
      ht1632_shadowram[addr][nChip-1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip-1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      break;
    case ORANGE:
      // set the bit in both the green and red planes;
      ht1632_shadowram[addr][nChip-1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip-1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip-1]);
      break;
  }
}


/*
 * ht1632_clear
 * clear the display, and the shadow memory, and the snapshot
 * memory.  This uses the "write multiple words" capability of
 * the chipset by writing all 96 words of memory without raising
 * the chipselect signal.
 */
void ht1632_clear()
{
  char i;
  for (int i=1; i< 5; i++)
  {
    ChipSelect(-1);
    ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
    ht1632_writebits(0, 1<<6); // Send address
    for (i = 0; i < 96/2; i++) // Clear entire display
      ht1632_writebits(0, 1<<7); // send 8 bits of data
    ChipSelect(0);

    for (int j=0; j < 64; j++)
      ht1632_shadowram[j][i-1] = 0;
  }
}


void setup7Seg(void)
{
    Wire.beginTransmission(0x20);      // start talking to the device
    Wire.write(0x00);                   // select the GPIO register
    Wire.write(0x00);                   // set register value-all high
    Wire.endTransmission();            // stop talking to the device

    // dizaine
    Wire.beginTransmission(0x21);      // start talking to the device
    Wire.write(0x00);                   // select the GPIO register
    Wire.write(0x00);                   // set register value-all high
    Wire.endTransmission();            // stop talking to the device

    // Centaine
    Wire.beginTransmission(0x22);      // start talking to the device
    Wire.write(0x00);                   // select the GPIO register
    Wire.write(0x00);                   // set register value-all high
    Wire.endTransmission();            // stop talking to the device

    // Millier
    Wire.beginTransmission(0x23);      // start talking to the device
    Wire.write(0x00);                   // select the GPIO register
    Wire.write(0x00);                   // set register value-all high
    Wire.endTransmission();            // stop talking to the device
}

