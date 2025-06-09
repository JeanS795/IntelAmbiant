/*
 * song_patterns.h
 * Définition des séquences de notes pour la musique - Optimisé pour mémoire
 */

#ifndef SONG_PATTERNS_H
#define SONG_PATTERNS_H

#include "notes_frequencies.h"
#include "tempo.h" // Ajouté pour que chaque pattern puisse définir son tempo
#include <avr/pgmspace.h>

// Structure pour stocker une note et sa durée - optimisée
typedef struct {
    uint16_t frequency;  // Fréquence de la note (réduit de int à uint16_t)
    uint8_t duration;    // Durée de la note (réduit de int à uint8_t)
} MusicNote;

// Séquences de notes avec leurs durées - stockées en mémoire PROGMEM

// ===== NIVEAU 1 - TRÈS FACILE =====
// Notes très lentes, pas de sauts, octaves basses
const PROGMEM MusicNote level1_intro[] = {
    {NOTE_C4, 32}, {NOTE_D4, 32}, {NOTE_E4, 32}, {NOTE_F4, 32},
    {NOTE_G4, 32}, {NOTE_A4, 32}, {NOTE_B4, 32}, {NOTE_C5, 32},
    {NOTE_D5, 32}, {NOTE_E5, 32}
};

const PROGMEM MusicNote level1_verse[] = {
    {NOTE_C4, 32}, {NOTE_E4, 32}, {NOTE_G4, 32}, {NOTE_C5, 32},
    {NOTE_E5, 32}, {NOTE_G5, 32}, {NOTE_C5, 32}, {NOTE_G4, 32},
    {NOTE_E4, 32}, {NOTE_C4, 32}, {NOTE_D4, 32}, {NOTE_F4, 32},
    {NOTE_A4, 32}, {NOTE_D5, 32}, {NOTE_F5, 32}, {NOTE_A5, 32}
};

const PROGMEM MusicNote level1_chorus[] = {
    {NOTE_G4, 32}, {NOTE_A4, 32}, {NOTE_B4, 32}, {NOTE_C5, 32},
    {NOTE_D5, 32}, {NOTE_E5, 32}, {NOTE_F5, 32}, {NOTE_G5, 32},
    {NOTE_F5, 32}, {NOTE_E5, 32}, {NOTE_D5, 32}, {NOTE_C5, 32}
};

const PROGMEM MusicNote level1_hook[] = {
    {NOTE_C5, 32}, {NOTE_G4, 32}, {NOTE_E4, 32}, {NOTE_C4, 32},
    {NOTE_E4, 32}, {NOTE_G4, 32}, {NOTE_C5, 32}, {NOTE_G4, 32}
};

// ===== NIVEAU 2 - FACILE =====
// Notes lentes, petits sauts
const PROGMEM MusicNote level2_intro[] = {
    {NOTE_C4, 24}, {NOTE_E4, 24}, {NOTE_G4, 24}, {NOTE_C5, 24},
    {NOTE_A4, 24}, {NOTE_F4, 24}, {NOTE_D4, 24}, {NOTE_G4, 24},
    {NOTE_B4, 24}, {NOTE_D5, 24}, {NOTE_F5, 24}, {NOTE_A5, 24}
};

const PROGMEM MusicNote level2_verse[] = {
    {NOTE_C4, 24}, {NOTE_G4, 24}, {NOTE_E5, 24}, {NOTE_C5, 24},
    {NOTE_F4, 24}, {NOTE_A4, 24}, {NOTE_D5, 24}, {NOTE_F5, 24},
    {NOTE_G4, 24}, {NOTE_B4, 24}, {NOTE_G5, 24}, {NOTE_D5, 24},
    {NOTE_A4, 24}, {NOTE_C5, 24}, {NOTE_E5, 24}, {NOTE_A5, 24},
    {NOTE_F5, 24}, {NOTE_D5, 24}, {NOTE_B4, 24}, {NOTE_G4, 24}
};

const PROGMEM MusicNote level2_chorus[] = {
    {NOTE_E4, 24}, {NOTE_C5, 24}, {NOTE_G5, 24}, {NOTE_E5, 24},
    {NOTE_F4, 24}, {NOTE_D5, 24}, {NOTE_A5, 24}, {NOTE_F5, 24},
    {NOTE_G4, 24}, {NOTE_E5, 24}, {NOTE_B5, 24}, {NOTE_G5, 24},
    {NOTE_A4, 24}, {NOTE_F5, 24}, {NOTE_C6, 24}, {NOTE_A5, 24}
};

const PROGMEM MusicNote level2_hook[] = {
    {NOTE_C5, 24}, {NOTE_E4, 24}, {NOTE_G5, 24}, {NOTE_C4, 24},
    {NOTE_F5, 24}, {NOTE_A4, 24}, {NOTE_D5, 24}, {NOTE_G4, 24},
    {NOTE_B5, 24}, {NOTE_D4, 24}
};

// ===== NIVEAU 3 - FACILE-MOYEN =====
// Notes moyennes, plus de variété
const PROGMEM MusicNote level3_intro[] = {
    {NOTE_C4, 16}, {NOTE_F4, 16}, {NOTE_A4, 16}, {NOTE_D5, 16},
    {NOTE_G4, 16}, {NOTE_B4, 16}, {NOTE_E5, 16}, {NOTE_A5, 16},
    {NOTE_D4, 16}, {NOTE_G4, 16}, {NOTE_C5, 16}, {NOTE_F5, 16},
    {NOTE_B4, 16}, {NOTE_E5, 16}, {NOTE_A5, 16}, {NOTE_D6, 16}
};

const PROGMEM MusicNote level3_verse[] = {
    {NOTE_C4, 16}, {NOTE_A4, 16}, {NOTE_F5, 16}, {NOTE_D5, 16},
    {NOTE_G4, 16}, {NOTE_E5, 16}, {NOTE_C6, 16}, {NOTE_A5, 16},
    {NOTE_F4, 16}, {NOTE_D5, 16}, {NOTE_B5, 16}, {NOTE_G5, 16},
    {NOTE_E4, 16}, {NOTE_C5, 16}, {NOTE_A5, 16}, {NOTE_F5, 16},
    {NOTE_D4, 16}, {NOTE_B4, 16}, {NOTE_G5, 16}, {NOTE_E5, 16},
    {NOTE_A4, 16}, {NOTE_F5, 16}, {NOTE_D6, 16}, {NOTE_B5, 16}
};

const PROGMEM MusicNote level3_chorus[] = {
    {NOTE_G4, 16}, {NOTE_D5, 16}, {NOTE_B5, 16}, {NOTE_F5, 16},
    {NOTE_C5, 16}, {NOTE_A5, 16}, {NOTE_E6, 16}, {NOTE_C6, 16},
    {NOTE_F4, 16}, {NOTE_C5, 16}, {NOTE_A5, 16}, {NOTE_F6, 16},
    {NOTE_D5, 16}, {NOTE_B5, 16}, {NOTE_G6, 16}, {NOTE_D6, 16}
};

const PROGMEM MusicNote level3_hook[] = {
    {NOTE_E5, 16}, {NOTE_C4, 16}, {NOTE_A5, 16}, {NOTE_F4, 16},
    {NOTE_D6, 16}, {NOTE_G4, 16}, {NOTE_B5, 16}, {NOTE_E4, 16},
    {NOTE_C6, 16}, {NOTE_A4, 16}, {NOTE_F5, 16}, {NOTE_D4, 16}
};

// ===== NIVEAU 4 - MOYEN =====
// Notes moyennes, sauts plus grands
const PROGMEM MusicNote level4_intro[] = {
    {NOTE_C4, 12}, {NOTE_G5, 12}, {NOTE_E4, 12}, {NOTE_B5, 12},
    {NOTE_F4, 12}, {NOTE_D6, 12}, {NOTE_A4, 12}, {NOTE_F6, 12},
    {NOTE_D4, 12}, {NOTE_A5, 12}, {NOTE_G4, 12}, {NOTE_E6, 12},
    {NOTE_B4, 12}, {NOTE_G6, 12}, {NOTE_C5, 12}, {NOTE_A6, 12}
};

const PROGMEM MusicNote level4_verse[] = {
    {NOTE_C4, 12}, {NOTE_E6, 12}, {NOTE_G4, 12}, {NOTE_C6, 12},
    {NOTE_F4, 12}, {NOTE_A6, 12}, {NOTE_D5, 12}, {NOTE_F6, 12},
    {NOTE_B4, 12}, {NOTE_D6, 12}, {NOTE_G5, 12}, {NOTE_B6, 12},
    {NOTE_E4, 12}, {NOTE_G6, 12}, {NOTE_A4, 12}, {NOTE_C7, 12},
    {NOTE_F5, 12}, {NOTE_A5, 12}, {NOTE_C4, 12}, {NOTE_E6, 12},
    {NOTE_D4, 12}, {NOTE_B5, 12}, {NOTE_G4, 12}, {NOTE_D7, 12}
};

const PROGMEM MusicNote level4_chorus[] = {
    {NOTE_E4, 12}, {NOTE_C7, 12}, {NOTE_A4, 12}, {NOTE_F6, 12},
    {NOTE_D5, 12}, {NOTE_B6, 12}, {NOTE_G4, 12}, {NOTE_E7, 12},
    {NOTE_C5, 12}, {NOTE_A6, 12}, {NOTE_F4, 12}, {NOTE_D7, 12},
    {NOTE_B4, 12}, {NOTE_G7, 12}, {NOTE_E5, 12}, {NOTE_C6, 12},
    {NOTE_A5, 12}, {NOTE_F7, 12}, {NOTE_D4, 12}, {NOTE_B5, 12}
};

const PROGMEM MusicNote level4_hook[] = {
    {NOTE_G4, 12}, {NOTE_E7, 12}, {NOTE_C5, 12}, {NOTE_A6, 12},
    {NOTE_F4, 12}, {NOTE_D7, 12}, {NOTE_B5, 12}, {NOTE_G6, 12},
    {NOTE_E4, 12}, {NOTE_C7, 12}, {NOTE_A4, 12}, {NOTE_F6, 12}
};

// ===== NIVEAU 5 - MOYEN-DIFFICILE =====
// Notes rapides, sauts moyens
const PROGMEM MusicNote level5_intro[] = {
    {NOTE_C4, 8}, {NOTE_F5, 8}, {NOTE_A4, 8}, {NOTE_D6, 8},
    {NOTE_G4, 8}, {NOTE_C6, 8}, {NOTE_E5, 8}, {NOTE_A6, 8},
    {NOTE_F4, 8}, {NOTE_B5, 8}, {NOTE_D5, 8}, {NOTE_G6, 8},
    {NOTE_A4, 8}, {NOTE_E6, 8}, {NOTE_C5, 8}, {NOTE_F6, 8},
    {NOTE_B4, 8}, {NOTE_D6, 8}, {NOTE_G5, 8}, {NOTE_C7, 8}
};

const PROGMEM MusicNote level5_verse[] = {
    {NOTE_E4, 8}, {NOTE_A6, 8}, {NOTE_C5, 8}, {NOTE_F6, 8},
    {NOTE_G4, 8}, {NOTE_B6, 8}, {NOTE_D5, 8}, {NOTE_E6, 8},
    {NOTE_A4, 8}, {NOTE_C7, 8}, {NOTE_F5, 8}, {NOTE_G6, 8},
    {NOTE_B4, 8}, {NOTE_D7, 8}, {NOTE_E5, 8}, {NOTE_A6, 8},
    {NOTE_C4, 8}, {NOTE_F7, 8}, {NOTE_G5, 8}, {NOTE_B6, 8},
    {NOTE_D4, 8}, {NOTE_E7, 8}, {NOTE_A5, 8}, {NOTE_C6, 8},
    {NOTE_F4, 8}, {NOTE_G7, 8}, {NOTE_B5, 8}, {NOTE_D6, 8}
};

const PROGMEM MusicNote level5_chorus[] = {
    {NOTE_A4, 8}, {NOTE_F7, 8}, {NOTE_D5, 8}, {NOTE_B6, 8},
    {NOTE_G4, 8}, {NOTE_E7, 8}, {NOTE_C6, 8}, {NOTE_A6, 8},
    {NOTE_F5, 8}, {NOTE_D7, 8}, {NOTE_B4, 8}, {NOTE_G7, 8},
    {NOTE_E5, 8}, {NOTE_C7, 8}, {NOTE_A5, 8}, {NOTE_F6, 8},
    {NOTE_D4, 8}, {NOTE_B7, 8}, {NOTE_G5, 8}, {NOTE_E6, 8}
};

const PROGMEM MusicNote level5_hook[] = {
    {NOTE_C5, 8}, {NOTE_A7, 8}, {NOTE_F4, 8}, {NOTE_D7, 8},
    {NOTE_B5, 8}, {NOTE_G6, 8}, {NOTE_E4, 8}, {NOTE_C7, 8},
    {NOTE_A5, 8}, {NOTE_F7, 8}, {NOTE_D4, 8}, {NOTE_B6, 8},
    {NOTE_G5, 8}, {NOTE_E7, 8}
};

// ===== NIVEAU 6 - DIFFICILE =====
// Notes rapides, grands sauts
const PROGMEM MusicNote level6_intro[] = {
    {NOTE_C4, 6}, {NOTE_G7, 6}, {NOTE_E4, 6}, {NOTE_C7, 6},
    {NOTE_A4, 6}, {NOTE_F7, 6}, {NOTE_D4, 6}, {NOTE_B7, 6},
    {NOTE_F4, 6}, {NOTE_D7, 6}, {NOTE_B4, 6}, {NOTE_A7, 6},
    {NOTE_G4, 6}, {NOTE_E7, 6}, {NOTE_C5, 6}, {NOTE_G7, 6},
    {NOTE_A5, 6}, {NOTE_C4, 6}, {NOTE_F5, 6}, {NOTE_B7, 6},
    {NOTE_D5, 6}, {NOTE_F4, 6}, {NOTE_B5, 6}, {NOTE_E7, 6}
};

const PROGMEM MusicNote level6_verse[] = {
    {NOTE_E4, 6}, {NOTE_B7, 6}, {NOTE_A4, 6}, {NOTE_D7, 6},
    {NOTE_C4, 6}, {NOTE_G7, 6}, {NOTE_F5, 6}, {NOTE_A7, 6},
    {NOTE_G4, 6}, {NOTE_C7, 6}, {NOTE_B5, 6}, {NOTE_E7, 6},
    {NOTE_D4, 6}, {NOTE_F7, 6}, {NOTE_A5, 6}, {NOTE_B7, 6},
    {NOTE_F4, 6}, {NOTE_E7, 6}, {NOTE_C6, 6}, {NOTE_G7, 6},
    {NOTE_B4, 6}, {NOTE_D7, 6}, {NOTE_G5, 6}, {NOTE_A7, 6},
    {NOTE_E5, 6}, {NOTE_C4, 6}, {NOTE_A6, 6}, {NOTE_F7, 6},
    {NOTE_D6, 6}, {NOTE_G4, 6}, {NOTE_B6, 6}, {NOTE_E7, 6}
};

const PROGMEM MusicNote level6_chorus[] = {
    {NOTE_F4, 6}, {NOTE_C8, 6}, {NOTE_A5, 6}, {NOTE_D7, 6},
    {NOTE_C4, 6}, {NOTE_B7, 6}, {NOTE_G6, 6}, {NOTE_E7, 6},
    {NOTE_D4, 6}, {NOTE_A7, 6}, {NOTE_F6, 6}, {NOTE_C7, 6},
    {NOTE_B4, 6}, {NOTE_G7, 6}, {NOTE_E6, 6}, {NOTE_F7, 6},
    {NOTE_A4, 6}, {NOTE_D8, 6}, {NOTE_C5, 6}, {NOTE_B7, 6},
    {NOTE_G4, 6}, {NOTE_E8, 6}, {NOTE_F5, 6}, {NOTE_A7, 6}
};

const PROGMEM MusicNote level6_hook[] = {
    {NOTE_E4, 6}, {NOTE_C8, 6}, {NOTE_B5, 6}, {NOTE_F7, 6},
    {NOTE_G4, 6}, {NOTE_D8, 6}, {NOTE_A5, 6}, {NOTE_E7, 6},
    {NOTE_C4, 6}, {NOTE_B7, 6}, {NOTE_F6, 6}, {NOTE_G7, 6},
    {NOTE_D4, 6}, {NOTE_A8, 6}, {NOTE_E6, 6}, {NOTE_C7, 6}
};

// ===== NIVEAU 7 - TRÈS DIFFICILE =====
// Notes très rapides, très grands sauts
const PROGMEM MusicNote level7_intro[] = {
    {NOTE_C4, 4}, {NOTE_A8, 4}, {NOTE_F4, 4}, {NOTE_D8, 4},
    {NOTE_B4, 4}, {NOTE_G8, 4}, {NOTE_E4, 4}, {NOTE_C8, 4},
    {NOTE_A4, 4}, {NOTE_F8, 4}, {NOTE_D4, 4}, {NOTE_B8, 4},
    {NOTE_G4, 4}, {NOTE_E8, 4}, {NOTE_C5, 4}, {NOTE_A8, 4},
    {NOTE_F5, 4}, {NOTE_C4, 4}, {NOTE_B5, 4}, {NOTE_G8, 4},
    {NOTE_E5, 4}, {NOTE_D4, 4}, {NOTE_A6, 4}, {NOTE_F8, 4},
    {NOTE_D6, 4}, {NOTE_B4, 4}, {NOTE_G6, 4}, {NOTE_E8, 4}
};

const PROGMEM MusicNote level7_verse[] = {
    {NOTE_C4, 4}, {NOTE_B8, 4}, {NOTE_G5, 4}, {NOTE_E8, 4},
    {NOTE_F4, 4}, {NOTE_C8, 4}, {NOTE_A6, 4}, {NOTE_F8, 4},
    {NOTE_D4, 4}, {NOTE_A8, 4}, {NOTE_B6, 4}, {NOTE_G8, 4},
    {NOTE_E4, 4}, {NOTE_D8, 4}, {NOTE_C7, 4}, {NOTE_A8, 4},
    {NOTE_A4, 4}, {NOTE_F8, 4}, {NOTE_D7, 4}, {NOTE_B8, 4},
    {NOTE_B4, 4}, {NOTE_G8, 4}, {NOTE_E7, 4}, {NOTE_C8, 4},
    {NOTE_C5, 4}, {NOTE_A8, 4}, {NOTE_F7, 4}, {NOTE_D8, 4},
    {NOTE_G4, 4}, {NOTE_E8, 4}, {NOTE_A7, 4}, {NOTE_F8, 4},
    {NOTE_F5, 4}, {NOTE_C4, 4}, {NOTE_B7, 4}, {NOTE_G8, 4},
    {NOTE_E6, 4}, {NOTE_D4, 4}, {NOTE_G7, 4}, {NOTE_A8, 4}
};

const PROGMEM MusicNote level7_chorus[] = {
    {NOTE_D4, 4}, {NOTE_C9, 4}, {NOTE_A5, 4}, {NOTE_F8, 4},
    {NOTE_E4, 4}, {NOTE_B8, 4}, {NOTE_C6, 4}, {NOTE_G8, 4},
    {NOTE_F4, 4}, {NOTE_A8, 4}, {NOTE_D6, 4}, {NOTE_E8, 4},
    {NOTE_G4, 4}, {NOTE_C8, 4}, {NOTE_E6, 4}, {NOTE_D8, 4},
    {NOTE_A4, 4}, {NOTE_F9, 4}, {NOTE_F6, 4}, {NOTE_B8, 4},
    {NOTE_B4, 4}, {NOTE_G9, 4}, {NOTE_G6, 4}, {NOTE_A8, 4},
    {NOTE_C5, 4}, {NOTE_E9, 4}, {NOTE_A6, 4}, {NOTE_C8, 4}
};

const PROGMEM MusicNote level7_hook[] = {
    {NOTE_E4, 4}, {NOTE_G9, 4}, {NOTE_B6, 4}, {NOTE_D8, 4},
    {NOTE_F4, 4}, {NOTE_A9, 4}, {NOTE_C7, 4}, {NOTE_E8, 4},
    {NOTE_G4, 4}, {NOTE_F9, 4}, {NOTE_D7, 4}, {NOTE_B8, 4},
    {NOTE_A4, 4}, {NOTE_C9, 4}, {NOTE_E7, 4}, {NOTE_G8, 4},
    {NOTE_C4, 4}, {NOTE_B9, 4}, {NOTE_F7, 4}, {NOTE_A8, 4},
    {NOTE_D4, 4}, {NOTE_E9, 4}, {NOTE_G7, 4}, {NOTE_F8, 4}
};

// ===== NIVEAU 8 - EXPERT =====
// Notes ultra-rapides, sauts extrêmes
const PROGMEM MusicNote level8_intro[] = {
    {NOTE_C4, 2}, {NOTE_B9, 2}, {NOTE_E4, 2}, {NOTE_A9, 2},
    {NOTE_G4, 2}, {NOTE_F9, 2}, {NOTE_C4, 2}, {NOTE_G9, 2},
    {NOTE_F4, 2}, {NOTE_D9, 2}, {NOTE_A4, 2}, {NOTE_E9, 2},
    {NOTE_D4, 2}, {NOTE_C9, 2}, {NOTE_B4, 2}, {NOTE_F9, 2},
    {NOTE_G4, 2}, {NOTE_A9, 2}, {NOTE_E4, 2}, {NOTE_B9, 2},
    {NOTE_C5, 2}, {NOTE_G9, 2}, {NOTE_A4, 2}, {NOTE_D9, 2},
    {NOTE_F5, 2}, {NOTE_E9, 2}, {NOTE_D4, 2}, {NOTE_C9, 2},
    {NOTE_B5, 2}, {NOTE_F9, 2}, {NOTE_G4, 2}, {NOTE_A9, 2},
    {NOTE_E6, 2}, {NOTE_B9, 2}, {NOTE_C4, 2}, {NOTE_G9, 2}
};

const PROGMEM MusicNote level8_verse[] = {
    {NOTE_F4, 2}, {NOTE_G9, 2}, {NOTE_C7, 2}, {NOTE_D9, 2},
    {NOTE_A4, 2}, {NOTE_F9, 2}, {NOTE_D7, 2}, {NOTE_A9, 2},
    {NOTE_B4, 2}, {NOTE_E9, 2}, {NOTE_E7, 2}, {NOTE_B9, 2},
    {NOTE_C4, 2}, {NOTE_C9, 2}, {NOTE_F7, 2}, {NOTE_E9, 2},
    {NOTE_G4, 2}, {NOTE_A9, 2}, {NOTE_G7, 2}, {NOTE_F9, 2},
    {NOTE_D4, 2}, {NOTE_B9, 2}, {NOTE_A7, 2}, {NOTE_G9, 2},
    {NOTE_E4, 2}, {NOTE_D9, 2}, {NOTE_B7, 2}, {NOTE_A9, 2},
    {NOTE_F5, 2}, {NOTE_E9, 2}, {NOTE_C8, 2}, {NOTE_B9, 2},
    {NOTE_A5, 2}, {NOTE_F9, 2}, {NOTE_D8, 2}, {NOTE_C9, 2},
    {NOTE_B5, 2}, {NOTE_G9, 2}, {NOTE_E8, 2}, {NOTE_D9, 2},
    {NOTE_C6, 2}, {NOTE_A9, 2}, {NOTE_F8, 2}, {NOTE_E9, 2},
    {NOTE_G6, 2}, {NOTE_B9, 2}, {NOTE_G8, 2}, {NOTE_F9, 2}
};

const PROGMEM MusicNote level8_chorus[] = {
    {NOTE_D4, 2}, {NOTE_A9, 2}, {NOTE_A8, 2}, {NOTE_G9, 2},
    {NOTE_E4, 2}, {NOTE_B9, 2}, {NOTE_B8, 2}, {NOTE_A9, 2},
    {NOTE_F4, 2}, {NOTE_C9, 2}, {NOTE_C9, 2}, {NOTE_B9, 2},
    {NOTE_G4, 2}, {NOTE_D9, 2}, {NOTE_D9, 2}, {NOTE_C9, 2},
    {NOTE_A4, 2}, {NOTE_E9, 2}, {NOTE_E9, 2}, {NOTE_D9, 2},
    {NOTE_B4, 2}, {NOTE_F9, 2}, {NOTE_F9, 2}, {NOTE_E9, 2},
    {NOTE_C5, 2}, {NOTE_G9, 2}, {NOTE_G9, 2}, {NOTE_F9, 2},
    {NOTE_D5, 2}, {NOTE_A9, 2}, {NOTE_A9, 2}, {NOTE_G9, 2}
};

const PROGMEM MusicNote level8_hook[] = {
    {NOTE_E5, 2}, {NOTE_B9, 2}, {NOTE_B9, 2}, {NOTE_A9, 2},
    {NOTE_F5, 2}, {NOTE_C9, 2}, {NOTE_C9, 2}, {NOTE_B9, 2},
    {NOTE_G5, 2}, {NOTE_D9, 2}, {NOTE_D9, 2}, {NOTE_C9, 2},
    {NOTE_A5, 2}, {NOTE_E9, 2}, {NOTE_E9, 2}, {NOTE_D9, 2},
    {NOTE_B5, 2}, {NOTE_F9, 2}, {NOTE_F9, 2}, {NOTE_E9, 2},
    {NOTE_C6, 2}, {NOTE_G9, 2}, {NOTE_G9, 2}, {NOTE_F9, 2}
};

// ===== NIVEAU 9 - IMPOSSIBLE =====
// Mix de durées courtes, sauts chaotiques
const PROGMEM MusicNote level9_intro[] = {
    {NOTE_C4, 1}, {NOTE_G9, 3}, {NOTE_E4, 1}, {NOTE_B9, 2},
    {NOTE_A8, 1}, {NOTE_D4, 3}, {NOTE_F9, 1}, {NOTE_G4, 2},
    {NOTE_C9, 1}, {NOTE_B4, 3}, {NOTE_A9, 1}, {NOTE_E4, 2},
    {NOTE_D9, 1}, {NOTE_F5, 3}, {NOTE_G9, 1}, {NOTE_A4, 2},
    {NOTE_E9, 1}, {NOTE_C6, 3}, {NOTE_B9, 1}, {NOTE_D4, 2},
    {NOTE_F9, 1}, {NOTE_G6, 3}, {NOTE_A9, 1}, {NOTE_B5, 2},
    {NOTE_C9, 1}, {NOTE_E7, 3}, {NOTE_D9, 1}, {NOTE_F4, 2},
    {NOTE_G9, 1}, {NOTE_A7, 3}, {NOTE_E9, 1}, {NOTE_C5, 2},
    {NOTE_B9, 1}, {NOTE_D8, 3}, {NOTE_F9, 1}, {NOTE_G5, 2}
};

const PROGMEM MusicNote level9_verse[] = {
    {NOTE_A9, 1}, {NOTE_E8, 2}, {NOTE_C4, 1}, {NOTE_B9, 3},
    {NOTE_F8, 1}, {NOTE_D4, 2}, {NOTE_G9, 1}, {NOTE_A8, 3},
    {NOTE_E4, 1}, {NOTE_C9, 2}, {NOTE_B8, 1}, {NOTE_F4, 3},
    {NOTE_D9, 1}, {NOTE_G4, 2}, {NOTE_C9, 1}, {NOTE_A4, 3},
    {NOTE_E9, 1}, {NOTE_D8, 2}, {NOTE_F9, 1}, {NOTE_B4, 3},
    {NOTE_G8, 1}, {NOTE_C5, 2}, {NOTE_A9, 1}, {NOTE_E8, 3},
    {NOTE_B9, 1}, {NOTE_F5, 2}, {NOTE_C8, 1}, {NOTE_G5, 3},
    {NOTE_D9, 1}, {NOTE_A8, 2}, {NOTE_E9, 1}, {NOTE_B5, 3},
    {NOTE_F8, 1}, {NOTE_C6, 2}, {NOTE_G9, 1}, {NOTE_D8, 3},
    {NOTE_A9, 1}, {NOTE_E6, 2}, {NOTE_B8, 1}, {NOTE_F6, 3},
    {NOTE_C9, 1}, {NOTE_G8, 2}, {NOTE_D9, 1}, {NOTE_A6, 3},
    {NOTE_E8, 1}, {NOTE_B6, 2}, {NOTE_F9, 1}, {NOTE_C7, 3},
    {NOTE_G8, 1}, {NOTE_D7, 2}, {NOTE_A9, 1}, {NOTE_E7, 3},
    {NOTE_B8, 1}, {NOTE_F7, 2}, {NOTE_C9, 1}, {NOTE_G7, 3}
};

const PROGMEM MusicNote level9_chorus[] = {
    {NOTE_D9, 1}, {NOTE_A7, 2}, {NOTE_E8, 1}, {NOTE_B7, 3},
    {NOTE_F9, 1}, {NOTE_C8, 2}, {NOTE_G8, 1}, {NOTE_D8, 3},
    {NOTE_A9, 1}, {NOTE_E8, 2}, {NOTE_B8, 1}, {NOTE_F8, 3},
    {NOTE_C9, 1}, {NOTE_G8, 2}, {NOTE_D8, 1}, {NOTE_A8, 3},
    {NOTE_E9, 1}, {NOTE_B8, 2}, {NOTE_F8, 1}, {NOTE_C9, 3},
    {NOTE_G9, 1}, {NOTE_D9, 2}, {NOTE_A8, 1}, {NOTE_E9, 3},
    {NOTE_B9, 1}, {NOTE_F9, 2}, {NOTE_C8, 1}, {NOTE_G9, 3},
    {NOTE_A9, 1}, {NOTE_E9, 2}, {NOTE_D8, 1}, {NOTE_B9, 3},
    {NOTE_F9, 1}, {NOTE_C9, 2}, {NOTE_G8, 1}, {NOTE_A9, 3},
    {NOTE_E9, 1}, {NOTE_D9, 2}, {NOTE_B8, 1}, {NOTE_F9, 3}
};

const PROGMEM MusicNote level9_hook[] = {
    {NOTE_G9, 1}, {NOTE_C8, 3}, {NOTE_A8, 1}, {NOTE_E9, 2},
    {NOTE_B9, 1}, {NOTE_D8, 3}, {NOTE_C8, 1}, {NOTE_F9, 2},
    {NOTE_D9, 1}, {NOTE_G8, 3}, {NOTE_E8, 1}, {NOTE_A9, 2},
    {NOTE_F9, 1}, {NOTE_B8, 3}, {NOTE_G8, 1}, {NOTE_C9, 2},
    {NOTE_A9, 1}, {NOTE_D8, 3}, {NOTE_B8, 1}, {NOTE_E9, 2},
    {NOTE_C9, 1}, {NOTE_F8, 3}, {NOTE_D8, 1}, {NOTE_G9, 2},
    {NOTE_E9, 1}, {NOTE_A8, 3}, {NOTE_F8, 1}, {NOTE_B9, 2},
    {NOTE_G9, 1}, {NOTE_C8, 3}, {NOTE_A8, 1}, {NOTE_D9, 2}
};

// ===== MUSIQUE ORIGINALE (sera le niveau par défaut si besoin) =====
// Introduction - notes avec durée 4
const PROGMEM MusicNote intro[] = {
    {NOTE_C4, 4}, {NOTE_D4, 4}, {NOTE_E4, 4}, {NOTE_F4, 4},
    {NOTE_G4, 4}, {NOTE_A4, 4}, {NOTE_B4, 4}, {NOTE_C5, 4},
    {NOTE_D5, 4}, {NOTE_E5, 4}, {NOTE_F5, 4}, {NOTE_G5, 4},
    {NOTE_A5, 4}, {NOTE_B5, 4}, {NOTE_C6, 4}, {NOTE_D6, 4},
    {NOTE_E6, 4}, {NOTE_F6, 4}
};

// Couplet - notes avec durée 8
const PROGMEM MusicNote verse[] = {
    {NOTE_C4, 8}, {NOTE_D4, 8}, {NOTE_E4, 8}, {NOTE_F4, 8},
    {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_B4, 8}, {NOTE_C5, 8},
    {NOTE_D5, 8}, {NOTE_E5, 8}, {NOTE_F5, 8}, {NOTE_G5, 8},
    {NOTE_A5, 8}, {NOTE_B5, 8}, {NOTE_C6, 8}, {NOTE_D6, 8},
    {NOTE_E6, 8}, {NOTE_F6, 8}, {NOTE_G6, 8}, {NOTE_A6, 8},
    {NOTE_B6, 8}, {NOTE_C7, 8}, {NOTE_D7, 8}, {NOTE_E7, 8},
    {NOTE_F7, 8}, {NOTE_G7, 8}, {NOTE_A7, 8}, {NOTE_B7, 8}
    // Réduction de la taille pour économiser la mémoire
};

// Refrain - notes avec durée 16
const PROGMEM MusicNote chorus[] = {
    {NOTE_C4, 16}, {NOTE_D4, 16}, {NOTE_E4, 16}, {NOTE_F4, 16},
    {NOTE_G4, 16}, {NOTE_A4, 16}, {NOTE_B4, 16}, {NOTE_C5, 16},
    {NOTE_D5, 16}, {NOTE_E5, 16}, {NOTE_F5, 16}, {NOTE_G5, 16},
    {NOTE_A5, 16}, {NOTE_B5, 16}, {NOTE_C6, 16}, {NOTE_D6, 16}
    // Réduction de la taille pour économiser la mémoire
};

// Hook - notes avec durée 2
const PROGMEM MusicNote hookPart[] = {
    {NOTE_C4, 2}, {NOTE_D4, 2}, {NOTE_E4, 2}, {NOTE_F4, 2},
    {NOTE_G4, 2}, {NOTE_A4, 2}, {NOTE_B4, 2}, {NOTE_C5, 2},
    {NOTE_D5, 2}, {NOTE_E5, 2}, {NOTE_F5, 2}, {NOTE_G5, 2}
    // Réduction de la taille pour économiser la mémoire
};

// Tailles des tableaux - valeurs constantes pour économiser des calculs
#define INTRO_SIZE 18
#define VERSE_SIZE 28
#define CHORUS_SIZE 16
#define HOOK_SIZE 12

// ===== TAILLES DES NOUVEAUX PATTERNS PAR NIVEAU =====
// Niveau 1 - Très facile
#define LEVEL1_INTRO_SIZE 10
#define LEVEL1_VERSE_SIZE 16
#define LEVEL1_CHORUS_SIZE 12
#define LEVEL1_HOOK_SIZE 8

// Niveau 2 - Facile
#define LEVEL2_INTRO_SIZE 12
#define LEVEL2_VERSE_SIZE 20
#define LEVEL2_CHORUS_SIZE 16
#define LEVEL2_HOOK_SIZE 10

// Niveau 3 - Facile-Moyen
#define LEVEL3_INTRO_SIZE 16
#define LEVEL3_VERSE_SIZE 24
#define LEVEL3_CHORUS_SIZE 16
#define LEVEL3_HOOK_SIZE 12

// Niveau 4 - Moyen
#define LEVEL4_INTRO_SIZE 16
#define LEVEL4_VERSE_SIZE 24
#define LEVEL4_CHORUS_SIZE 20
#define LEVEL4_HOOK_SIZE 12

// Niveau 5 - Moyen-Difficile
#define LEVEL5_INTRO_SIZE 20
#define LEVEL5_VERSE_SIZE 28
#define LEVEL5_CHORUS_SIZE 20
#define LEVEL5_HOOK_SIZE 14

// Niveau 6 - Difficile
#define LEVEL6_INTRO_SIZE 24
#define LEVEL6_VERSE_SIZE 32
#define LEVEL6_CHORUS_SIZE 20
#define LEVEL6_HOOK_SIZE 16

// Niveau 7 - Très Difficile
#define LEVEL7_INTRO_SIZE 28
#define LEVEL7_VERSE_SIZE 40
#define LEVEL7_CHORUS_SIZE 28
#define LEVEL7_HOOK_SIZE 24

// Niveau 8 - Expert
#define LEVEL8_INTRO_SIZE 36
#define LEVEL8_VERSE_SIZE 48
#define LEVEL8_CHORUS_SIZE 32
#define LEVEL8_HOOK_SIZE 24

// Niveau 9 - Impossible
#define LEVEL9_INTRO_SIZE 36
#define LEVEL9_VERSE_SIZE 56
#define LEVEL9_CHORUS_SIZE 40
#define LEVEL9_HOOK_SIZE 32

// Fonction helper pour lire une note depuis PROGMEM
inline void getNote(const MusicNote* array, uint8_t index, MusicNote* result) {
  result->frequency = pgm_read_word(&(array[index].frequency));
  result->duration = pgm_read_byte(&(array[index].duration));
}

#endif // SONG_PATTERNS_H