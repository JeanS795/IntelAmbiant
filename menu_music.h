#include "pitches_menu.h"

// Define the buzzer pin
#define BUZZER_PIN 3

// Structure pour combiner note et durée
struct MusicNote {
  int pitch;
  int duration;
};

// Séquence d'introduction (riff de cuivres)
struct MusicNote intro[] = {
  {NOTE_D5, 8}, {NOTE_CS5, 8}, {NOTE_D5, 8}, {NOTE_D5, 8}, 
  {NOTE_CS5, 8}, {NOTE_D5, 8}, {NOTE_B4, 8}, {NOTE_G4, 4},
  {0, 8}, // Pause
  {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_B4, 8}, {NOTE_D5, 8},
  {NOTE_CS5, 8}, {NOTE_D5, 8}, {NOTE_B4, 8}, {NOTE_G4, 4},
  {0, 8} // Pause
};

// Couplet principal
struct MusicNote verse[] = {
  {NOTE_D5, 8}, {NOTE_C5, 8}, {NOTE_G4, 4}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 4},
  
  {NOTE_D5, 8}, {NOTE_C5, 8}, {NOTE_G4, 4}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 4},
  
  {NOTE_D5, 8}, {NOTE_C5, 8}, {NOTE_G4, 4}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, 
  {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_D5, 4}
};

// Refrain "Don't believe me just watch"
struct MusicNote chorus[] = {
  {NOTE_D5, 8}, {NOTE_D5, 8}, {NOTE_D5, 8}, {NOTE_D5, 8},
  {NOTE_D5, 4}, {NOTE_C5, 8}, {NOTE_AS4, 8},
  {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 8},
  {NOTE_C5, 4}, {NOTE_AS4, 8}, {NOTE_G4, 8},
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8},
  {NOTE_G4, 4}, {NOTE_F4, 8}, {NOTE_DS4, 8},
  {NOTE_F4, 8}, {NOTE_F4, 8}, {NOTE_F4, 8}, {NOTE_F4, 8},
  {NOTE_F4, 8}, {NOTE_DS4, 8}, {NOTE_F4, 8}, {NOTE_G4, 8}
};

// Partie "Uptown Funk you up"
struct MusicNote hookPart[] = {
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_G4, 8},
  {NOTE_G4, 8}, {NOTE_G4, 8}, {NOTE_A4, 8}, {NOTE_AS4, 8},
  {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 8},
  {NOTE_C5, 8}, {NOTE_AS4, 8}, {NOTE_G4, 8}, {NOTE_F4, 8}
};

// Fonction pour jouer une séquence de notes
void playSequence(struct MusicNote *notes, int numNotes) {
  for (int i = 0; i < numNotes; i++) {
    // Si la note est 0, c'est une pause
    if (notes[i].pitch == 0) {
      noTone(BUZZER_PIN);
      delay(1000 / notes[i].duration);
    } else {
      // Jouer la note
      int noteDuration = 1000 / notes[i].duration;
      tone(BUZZER_PIN, notes[i].pitch, noteDuration);
      
      // Pause entre les notes (30% de la durée)
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      
      // Arrêter la note
      noTone(BUZZER_PIN);
    }
  }
}
