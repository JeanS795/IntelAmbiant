#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"
//je suius michel
// Définition du pin du potentiomètre
const int potPin = A0;

// Constantes pour la matrice LED
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define BLOCK_HEIGHT 2
#define MAX_BLOCKS 12

#define MUSIQUE 0
#define BUZZER_PIN 3

// Structure pour stocker les informations d'un bloc 
typedef struct {
  int16_t x;              // Position horizontale, changé en int16_t pour éviter les dépassements
  uint8_t y;              // Position verticale (0 à 255), changé de int8_t à uint8_t
  uint8_t length;         // Longueur du bloc (0 à 255)
  uint8_t color : 3;      // Couleur du bloc sur 2 bits (0-3)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint8_t priority : 5;   // Priorité sur 5 bits (0-31)
} Block;

// Définition des couleurs
#define BLOCK_COLOR 3    // RED = 2 selon ht1632.h
#define BORDER_COLOR 3   // RED = 2 selon ht1632.h

// Variables pour le contrôle des blocs et la déduplication
Block blocks[MAX_BLOCKS];
uint8_t nextBlockIndex = 0;
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;
uint8_t lastNoteFrequency = 0;  // Pour suivre la dernière fréquence utilisée
unsigned long lastNoteTime = 0;
bool lastNoteStillActive = false;  // Pour vérifier si la dernière note est encore active

// Variables de timing - RALENTISSEMENT SIGNIFICATIF
const uint16_t moveDelay = 300;     // Délai de mouvement (300 ms)
unsigned long lastMoveTime = 0;
uint16_t currentNoteDelay = 0;
uint8_t blockPriorityCounter = 0;

// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Fonction pour déterminer la position Y en fonction de la fréquence
uint8_t getPositionYFromFrequency(uint16_t frequency) {
  uint8_t position = 0;
  
  // Correspondance note -> position y sur 8 niveaux espacés de 2
  switch (frequency) {
    case 262: // Do (C4)
    case 523: // Do (C5)
    case 1047: // Do (C6)
    case 2093: // Do (C7)
      position = 0;
      break;
      
    case 294: // Ré (D4) 
    case 587: // Ré (D5)
    case 1175: // Ré (D6)
    case 2349: // Ré (D7)
      position = 2;
      break;
      
    case 330: // Mi (E4)
    case 659: // Mi (E5)
    case 1319: // Mi (E6)
    case 2637: // Mi (E7)
      position = 4;
      break;
      
    case 349: // Fa (F4)
    case 698: // Fa (F5)
    case 1397: // Fa (F6)
    case 2794: // Fa (F7)
      position = 6;
      break;
      
    case 392: // Sol (G4)
    case 784: // Sol (G5)
    case 1568: // Sol (G6)
    case 3136: // Sol (G7)
      position = 8;
      break;
      
    case 440: // La (A4)
    case 880: // La (A5)
    case 1760: // La (A6)
    case 3520: // La (A7)
      position = 10;
      break;
      
    case 494: // Si (B4)
    case 988: // Si (B5)
    case 1976: // Si (B6)
    case 3951: // Si (B7)  
      position = 12;
      break;
    
    default: // Pour toute autre fréquence
      position = 14;
      break;
  }
  
#if DEBUG_SERIAL
  Serial.print("Frequence: ");
  Serial.print(frequency);
  Serial.print(" -> Position Y: ");
  Serial.println(position);
#endif

  return position;
}

// Fonction pour vérifier si une position verticale est déjà occupée par un bloc actif
bool isVerticalPositionOccupied(uint8_t posY) {
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].y == posY) {
      return true;
    }
  }
  return false;
}

// Fonction pour vérifier si une colonne est déjà occupée par un bloc actif
bool isColumnOccupied(int16_t x) {
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      // Vérifier si la colonne x est comprise dans la plage du bloc existant
      if (x >= blocks[i].x && x < blocks[i].x + blocks[i].length) {
        return true;
      }
    }
  }
  return false;
}

// Fonction pour créer un nouveau bloc en fonction d'une note
void createNewBlock(const MusicNote* noteArray, uint8_t noteIndex) {
  // Récupérer les valeurs de la note depuis PROGMEM
  MusicNote note;
  getNote(noteArray, noteIndex, &note);
  
  // Vérifier si une note similaire est déjà active
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].y == getPositionYFromFrequency(note.frequency)) {
      if (blocks[i].x > MATRIX_WIDTH/2) {  // Si le bloc est encore dans la moitié droite
        return;  // Ne pas créer de nouveau bloc
      }
    }
  }
  
  // Vérification globale: éviter de créer trop de blocs actifs simultanément
  uint8_t activeCount = 0;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      activeCount++;
    }
  }
  
  // Si nous avons déjà beaucoup de blocs actifs, limiter la création
  if (activeCount >= MAX_BLOCKS/2) {
#if DEBUG_SERIAL
    Serial.println("Trop de blocs actifs, création différée");
#endif
    return;
  }
  
  // Trouver un emplacement libre pour le nouveau bloc
  int8_t blockIndex = -1;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (!blocks[i].active) {
      blockIndex = i;
      break;
    }
  }
  
  if (blockIndex == -1) {
#if DEBUG_SERIAL
    Serial.println("Pas d'emplacement libre pour nouveau bloc");
#endif
    return;
  }
  
  // Calcul de la longueur en fonction de la durée
  // Augmenter la longueur des blocs pour qu'ils restent visibles plus longtemps
  uint8_t length = note.duration; // Doublé (était note.duration / 2)
  if (length < 2) length = 1;     // Longueur minimale de 1
  
  // Position verticale en fonction de la fréquence
  uint8_t posY = getPositionYFromFrequency(note.frequency);
  
  // Éviter la création si la même note est déjà active
  if (note.frequency == lastNoteFrequency && lastNoteStillActive) {
#if DEBUG_SERIAL
    Serial.println("Note déjà active, création évitée");
#endif
    return;
  }
  
  // Vérification améliorée pour les positions verticales
  // Vérifier non seulement la position exacte mais aussi les positions adjacentes
  bool positionConflict = false;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      // Conflit si même position ou position adjacente
      if (blocks[i].y == posY || 
          (posY > 0 && blocks[i].y == posY - 1) || 
          (posY < MATRIX_HEIGHT-1 && blocks[i].y == posY + 1)) {
        positionConflict = true;
        break;
      }
    }
  }
  
  if (positionConflict) {
#if DEBUG_SERIAL
    Serial.println("Conflit de position Y détecté, création annulée");
#endif
    return; // Annuler la création plutôt que de chercher une position alternative
  }
  
  // S'assurer que le bloc ne dépasse pas les limites de la matrice
  if (posY + BLOCK_HEIGHT > MATRIX_HEIGHT) {
    posY = MATRIX_HEIGHT - BLOCK_HEIGHT;
  }
  
  // Position horizontale toujours à droite de l'écran
  int16_t startX = MATRIX_WIDTH;  // Modifié en int16_t
  
  // Vérification des superpositions horizontales (colonnes)
  bool columnOccupied = false;
  for (int16_t testX = startX; testX < startX + length; testX++) {
    if (isColumnOccupied(testX)) {
      columnOccupied = true;
      break;
    }
  }
  
  if (columnOccupied) {
#if DEBUG_SERIAL
    Serial.println("Colonne déjà occupée, attente avant création du bloc");
#endif
    // Ne pas créer ce bloc maintenant, on attendra un moment plus propice
    return;
  }
  
  // Vérification des superpositions 
  bool positionOccupied = false;
  do {
    positionOccupied = false;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active && i != blockIndex) {
        if ((startX <= blocks[i].x + blocks[i].length) && 
            (startX + length >= blocks[i].x)) {
          positionOccupied = true;
          startX = blocks[i].x - length - 1;
          break;
        }
      }
    }
  } while (positionOccupied && startX >= MATRIX_WIDTH/2);
  
  // Création du bloc si l'espace est disponible
  if (startX >= MATRIX_WIDTH/2) {
    blocks[blockIndex].x = startX;
    blocks[blockIndex].y = posY;
    blocks[blockIndex].length = length;
    blocks[blockIndex].color = BLOCK_COLOR;  // Utilisation de la couleur définie
    blocks[blockIndex].active = 1;
    
    blockPriorityCounter = (blockPriorityCounter + 1) & 0x1F;
    blocks[blockIndex].priority = blockPriorityCounter;
    
#if DEBUG_SERIAL //suivi des blocs créés
    Serial.print("Nouveau bloc: x=");
    Serial.print(startX);
    Serial.print(", y=");
    Serial.print(posY);
    Serial.print(", len=");
    Serial.println(length);
#endif
  }
  else {
    blocks[blockIndex].active = 0;
  }
  
  // Mettre à jour les informations de la dernière note
  lastNoteFrequency = note.frequency;
  lastNoteStillActive = true;
  
  // Délai entre les notes considérablement augmenté pour donner plus de temps
  uint8_t beatsPerMinute = 140;  
  uint16_t beatDuration = 60000 / beatsPerMinute;
  currentNoteDelay = beatDuration * 4 / note.duration ;  // 500ms de délai supplémentaire
  
  lastNoteTime = millis();
}

// Fonction pour dessiner un bloc sur la matrice
void drawBlock(Block block) {
  if (!block.active) return;
  
  // variables locales 
  int16_t xStart = block.x;
  int16_t xEnd = xStart + block.length;
  uint8_t yPos = block.y;  // Position Y de départ

  // Vérifier si le bloc est au moins partiellement visible
  if (xEnd <= 0 || xStart >= MATRIX_WIDTH || yPos >= MATRIX_HEIGHT) {
    return;  // Le bloc n'est pas visible sur la matrice
  }

  // Limiter aux frontières de la matrice
  if (xStart < 0) xStart = 0;
  if (xEnd > MATRIX_WIDTH) xEnd = MATRIX_WIDTH;

  // Correction : dessiner le bloc sur exactement 2 lignes (yPos et yPos+1)
  for (int16_t x = xStart; x < xEnd; x++) {
    if (x >= 0 && x < MATRIX_WIDTH) {
      if (yPos < MATRIX_HEIGHT) {
        ht1632_plot(x, yPos, block.color);      // Première ligne
      }
      if (yPos + 1 < MATRIX_HEIGHT) {
        ht1632_plot(x, yPos + 1, block.color);  // Deuxième ligne
      }
      // Ne rien dessiner sur d'autres lignes
    }
  }
}

// Variable globale pour garder trace de la dernière note créée
uint8_t lastNotePosition = 255; // Valeur impossible pour indiquer aucune note créée

// Fonction pour passer à la prochaine note de la chanson
void nextNote() {
  const MusicNote* currentSong;
  uint8_t currentSongSize;
  
  // Vérification pour éviter la création multiple de la même note
  if (songPosition == lastNotePosition && currentSongPart == currentSongPart) {
#if DEBUG_SERIAL
    Serial.println("Note déjà créée, évitement de duplication");
#endif
    return;
  }
  
  // Mémoriser cette position pour éviter la duplication
  lastNotePosition = songPosition;
  
  // Déterminer quelle partie de la chanson nous jouons
  switch (currentSongPart) {
    case 0:  // Intro
      currentSong = intro;
      currentSongSize = INTRO_SIZE;
      break;
    case 1:  // Verse
      currentSong = verse;
      currentSongSize = VERSE_SIZE;
      break;
    case 2:  // Chorus
      currentSong = chorus;
      currentSongSize = CHORUS_SIZE;
      break;
    case 3:  // Hook
      currentSong = hookPart;
      currentSongSize = HOOK_SIZE;
      break;
    default:
      songFinished = 1;
      return;
  }
  
  if (songPosition < currentSongSize) {
    createNewBlock(currentSong, songPosition);
    songPosition++;
  } else {
    currentSongPart++;
    songPosition = 0;
    
    if (currentSongPart > 3) {
      songFinished = 1;
    } else {
      nextNote();
    }
  }
}


void setup() {
  // Réactiver Serial pour le débogage
#if DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("Système initialisé avec débogage");
#endif
  
  // Initialisation de la matrice LED
  Wire.begin();
  ht1632_setup();
  setup7Seg();
  ht1632_clear();
  
  // Initialiser les blocs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    blocks[i].active = 0;
    blocks[i].color = BLOCK_COLOR;  // Initialiser la couleur explicitement
  }
  
  // Initialiser lastMoveTime pour garantir un déplacement immédiat
  lastMoveTime = 0;  // Forcer le premier déplacement rapidement
  
  // Créer le premier bloc
  nextNote();
  
#if MUSIQUE
  pinMode(BUZZER_PIN, OUTPUT);
#endif
  
#if DEBUG_SERIAL
  Serial.println("Premier bloc créé, prêt pour animation");
#endif
}

void loop() {
  unsigned long currentTime = millis();
  
  // Effacer la matrice
  ht1632_clear();

  // Remplir les colonnes 2 et 3 de vert (color=1)
  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    ht1632_plot(2, y, 1); // colonne 2
    ht1632_plot(3, y, 1); // colonne 3
  }

  // --- Curseur 2x2 contrôlé par le potentiomètre ---
  int potValue = analogRead(potPin); // 0 à 1023
  // Correction : map() doit être sur int, et clamp la valeur
  int cursorY = map(potValue, 0, 1023, 0, MATRIX_HEIGHT - 2);
  if (cursorY < 0) cursorY = 0;
  if (cursorY > MATRIX_HEIGHT - 2) cursorY = MATRIX_HEIGHT - 2;
  for (uint8_t dx = 2; dx <= 3; dx++) {
    for (uint8_t dy = 0; dy < 2; dy++) {
      ht1632_plot(dx, cursorY + dy, 2); // rouge
    }
  }
  // --------------------------------------------------
  
  // Variables pour éviter les créations multiples de notes
  static unsigned long lastNoteCreationTime = 0;
  static bool noteCreationInProgress = false;
  static uint16_t lastPlayedFrequency = 0;
  static unsigned long noteStartTime = 0;
  static uint16_t noteDurationMs = 0;
  
  // Vérifier s'il est temps de créer une nouvelle note
  if (!songFinished && currentTime - lastNoteTime >= currentNoteDelay && !noteCreationInProgress) {
    // Éviter les créations multiples même pendant les appels successifs à loop()
    if (currentTime - lastNoteCreationTime >= 1000) { // Augmenté à 1 seconde minimum
      lastNoteCreationTime = currentTime;
      noteCreationInProgress = true;
      
#if DEBUG_SERIAL
      Serial.println("Création d'une nouvelle note...");
#endif
      
      nextNote();
      noteCreationInProgress = false;
    }
  }
  
#if MUSIQUE
  // Lecture de la note courante sur le buzzer
  // On joue la note correspondant à la dernière note créée
  if (!songFinished) {
    const MusicNote* currentSong;
    uint8_t currentSongSize;
    switch (currentSongPart) {
      case 0: currentSong = intro; currentSongSize = INTRO_SIZE; break;
      case 1: currentSong = verse; currentSongSize = VERSE_SIZE; break;
      case 2: currentSong = chorus; currentSongSize = CHORUS_SIZE; break;
      case 3: currentSong = hookPart; currentSongSize = HOOK_SIZE; break;
      default: currentSong = intro; currentSongSize = INTRO_SIZE; break;
    }
    if (songPosition > 0 && songPosition <= currentSongSize) {
      MusicNote note;
      getNote(currentSong, songPosition - 1, &note);
      // Si la note a changé ou si la durée est écoulée, on joue la nouvelle note
      if (note.frequency != lastPlayedFrequency || (millis() - noteStartTime > noteDurationMs)) {
        if (note.frequency > 0) {
          // Calcul de la durée de la note en ms (ajuste selon ton tempo)
          noteDurationMs = 60000 / 140 * 4 / note.duration;
          tone(BUZZER_PIN, note.frequency, noteDurationMs);
          noteStartTime = millis();
        } else {
          noTone(BUZZER_PIN);
        }
        lastPlayedFrequency = note.frequency;
      }
    } else {
      noTone(BUZZER_PIN);
      lastPlayedFrequency = 0;
    }
  } else {
    noTone(BUZZER_PIN);
    lastPlayedFrequency = 0;
  }
#endif
  
  // Déplacer les blocs actifs 
  if (currentTime - lastMoveTime > moveDelay) {
    // Ajouter un message de débogage pour voir si cette section est exécutée
#if DEBUG_SERIAL
    Serial.println("Déplacement des blocs...");
#endif
    
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        // Position avant déplacement
        int16_t oldX = blocks[i].x;
        
        // Déplacement: ne se déplacer que d'un pixel à chaque itération
        blocks[i].x--;
        
#if DEBUG_SERIAL
        // Débogage du déplacement (seulement pour quelques blocs pour éviter trop de messages)
        if (i < 3) {  // Limiter à 3 blocs pour le débogage
          Serial.print("Bloc ");
          Serial.print(i);
          Serial.print(" déplacé: ");
          Serial.print(oldX);
          Serial.print(" -> ");
          Serial.println(blocks[i].x);
        }
#endif
        
        // Désactiver le bloc s'il est complètement sorti de l'écran
        if (blocks[i].x + blocks[i].length < 0) {
          blocks[i].active = 0;
#if DEBUG_SERIAL
          Serial.print("Bloc ");
          Serial.print(i);
          Serial.println(" désactivé (hors écran)");
#endif
        }
      }
    }
    lastMoveTime = currentTime;
  }
  
  // Simplifier l'affichage - Ne pas trier les blocs, les afficher simplement
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      drawBlock(blocks[i]);
    }
  }
  
  // Mettre à jour lastNoteStillActive
  lastNoteStillActive = false;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].x > MATRIX_WIDTH/2) {
      if (getPositionYFromFrequency(lastNoteFrequency) == blocks[i].y) {
        lastNoteStillActive = true;
        break;
      }
    }
  }
  
  // Redémarrer la séquence si terminée
  if (songFinished) {
    bool allBlocksInactive = true;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        allBlocksInactive = false;
        break;
      }
    }
    
    if (allBlocksInactive) {
      songFinished = 0;
      currentSongPart = 0;
      songPosition = 0;
      noteCreationInProgress = false;
      lastNoteCreationTime = 0;
      
#if DEBUG_SERIAL
      Serial.println("Redémarrage de la séquence musicale");
#endif
      
      nextNote();
    }
  }
  
  delay(10);
}