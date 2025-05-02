/*
 * frere_jacques_pattern.h
 * Définition des séquences de notes pour "Frère Jacques" - Optimisé pour mémoire
 */

#ifndef FRERE_JACQUES_PATTERN_H
#define FRERE_JACQUES_PATTERN_H

#include "notes_frequencies.h"
#include <avr/pgmspace.h>

// Structure pour stocker une note et sa durée - optimisée
#ifndef MUSIC_NOTE_DEFINED
#define MUSIC_NOTE_DEFINED
typedef struct {
    uint16_t frequency;  // Fréquence de la note
    uint8_t duration;    // Durée de la note (4 = noire, 8 = croche, etc.)
} MusicNote;
#endif

// Séquence "Frère Jacques" stockée en mémoire PROGMEM
const PROGMEM MusicNote frereJacques[] = {
    // Frère Jacques, Frère Jacques
    {NOTE_C4, 4}, {NOTE_D4, 4}, {NOTE_E4, 4}, {NOTE_C4, 4},
    {NOTE_C4, 4}, {NOTE_D4, 4}, {NOTE_E4, 4}, {NOTE_C4, 4},
    
    // Dormez-vous, dormez-vous
    {NOTE_E4, 4}, {NOTE_F4, 4}, {NOTE_G4, 2},
    {NOTE_E4, 4}, {NOTE_F4, 4}, {NOTE_G4, 2},
    
    // Sonnez les matines, sonnez les matines
    {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_G4, 8}, {NOTE_F4, 8}, {NOTE_E4, 4}, {NOTE_C4, 4},
    {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_G4, 8}, {NOTE_F4, 8}, {NOTE_E4, 4}, {NOTE_C4, 4},
    
    // Ding dang dong, ding dang dong
    {NOTE_C4, 4}, {NOTE_G4, 4}, {NOTE_C4, 2},
    {NOTE_C4, 4}, {NOTE_G4, 4}, {NOTE_C4, 2}
};

// Taille du tableau - valeur constante pour économiser des calculs
#define FRERE_JACQUES_SIZE 32

// Fonction helper pour lire une note depuis PROGMEM
inline void getNote(const MusicNote* array, uint8_t index, MusicNote* result) {
    result->frequency = pgm_read_word(&(array[index].frequency));
    result->duration = pgm_read_byte(&(array[index].duration));
}

#endif // FRERE_JACQUES_PATTERN_H