#ifndef LIB_MAGIC_H
#define LIB_MAGIC_H

#include <Wire.h>
#include <Arduino.h>
#include "ht1632.h"

// Variables externes
extern byte ht1632_shadowram[64][4];

// Fonctions publiques
void ht1632_setup(void);
void ht1632_plot(byte x, byte y, byte color);
void ht1632_clear(void);
void setup7Seg(void);

// Fonctions privées utilisées uniquement dans lib_magic.cpp
#ifdef LIB_MAGIC_CPP
static void OutputCLK_Pulse(void);
static void OutputA_74164(unsigned char x);
static void ChipSelect(int select);
static void ht1632_writebits(byte bits, byte firstbit);
static void ht1632_sendcmd(byte chipNo, byte command);
static void ht1632_senddata(byte chipNo, byte address, byte data);
#endif

#endif
