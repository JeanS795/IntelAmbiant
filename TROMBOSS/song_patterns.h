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

// Fonction helper pour lire une note depuis PROGMEM
inline void getNote(const MusicNote* array, uint8_t index, MusicNote* result) {
  result->frequency = pgm_read_word(&(array[index].frequency));
  result->duration = pgm_read_byte(&(array[index].duration));
}

#endif // SONG_PATTERNS_H