#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"

// Définition du pin du potentiomètre
const int potPin = A0;

// Constantes pour la matrice LED
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define BLOCK_HEIGHT 2
#define MAX_BLOCKS 12

// Structure pour stocker les informations d'un bloc 
typedef struct {
  int16_t x;              // Position horizontale, changé en int16_t pour éviter les dépassements
  uint8_t y;              // Position verticale (0 à 255), changé de int8_t à uint8_t
  uint8_t length;         // Longueur du bloc (0 à 255)
  uint8_t color : 2;      // Couleur du bloc sur 2 bits (0-3)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint8_t priority : 5;   // Priorité sur 5 bits (0-31)
} Block;

// Variables pour le contrôle des blocs
Block blocks[MAX_BLOCKS];
uint8_t nextBlockIndex = 0;
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;

// Variables de timing - RALENTISSEMENT SIGNIFICATIF
const uint16_t moveDelay = 800;     // Augmenté à 800ms (était 350ms)
unsigned long lastMoveTime = 0;
unsigned long lastNoteTime = 0;
uint16_t currentNoteDelay = 0;
uint8_t blockPriorityCounter = 0;

// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Fonction pour déterminer la position Y en fonction de la fréquence
uint8_t getPositionYFromFrequency(uint16_t frequency) {
  uint8_t position = 0;
  
  // Échelle de positions sur 8 niveaux pour une meilleure répartition
  if (frequency >= 261 && frequency <= 349)      // C4-F4
    position = 0;
  else if (frequency >= 350 && frequency <= 493) // F#4-B4
    position = 2;
  else if (frequency >= 494 && frequency <= 698) // C5-F5
    position = 4;
  else if (frequency >= 699 && frequency <= 987) // F#5-B5
    position = 6;
  else if (frequency >= 988 && frequency <= 1396) // C6-F6
    position = 8;
  else if (frequency >= 1397 && frequency <= 1975) // F#6-B6
    position = 10;
  else if (frequency >= 1976 && frequency <= 2793) // C7-F7
    position = 12;
  else // F#7 et au-delà
    position = 14;
  
#if DEBUG_SERIAL
  Serial.print("Frequence: ");
  Serial.print(frequency);
  Serial.print(" -> Position Y: ");
  Serial.println(position);
#endif

  return position;
}

// Fonction pour créer un nouveau bloc en fonction d'une note
void createNewBlock(const MusicNote* noteArray, uint8_t noteIndex) {
  // Récupérer les valeurs de la note depuis PROGMEM
  MusicNote note;
  getNote(noteArray, noteIndex, &note);
  
  // Trouver un emplacement libre ou réutiliser un bloc
  uint8_t blockIndex = nextBlockIndex;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (!blocks[i].active) {
      blockIndex = i;
      break;
    }
  }
  nextBlockIndex = (nextBlockIndex + 1) % MAX_BLOCKS;
  
  // Calcul de la longueur en fonction de la durée
  // Augmenter la longueur des blocs pour qu'ils restent visibles plus longtemps
  uint8_t length = note.duration; // Doublé (était note.duration / 2)
  if (length < 2) length = 1;     // Longueur minimale de 1
  
  // Position verticale en fonction de la fréquence
  uint8_t posY = getPositionYFromFrequency(note.frequency);
  
  // S'assurer que le bloc ne dépasse pas les limites de la matrice
  if (posY + BLOCK_HEIGHT > MATRIX_HEIGHT) {
    posY = MATRIX_HEIGHT - BLOCK_HEIGHT;
  }
  
  // Position horizontale toujours à droite de l'écran
  int16_t startX = MATRIX_WIDTH;  // Modifié en int16_t
  
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
    blocks[blockIndex].color = RED; 
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
  
  // Délai entre les notes considérablement augmenté pour donner plus de temps
  uint8_t beatsPerMinute = 80;  
  uint16_t beatDuration = 60000 / beatsPerMinute;
  currentNoteDelay = beatDuration * 4 / note.duration + 150;  // 500ms de délai supplémentaire
  
  lastNoteTime = millis();
}

// Fonction pour dessiner un bloc sur la matrice
void drawBlock(Block block) {
  if (!block.active) return;
  
  // variables locales = meilleurs perfs
  int16_t xStart = block.x;
  int16_t xEnd = xStart + block.length;
  uint8_t yStart = block.y;
  uint8_t yEnd = yStart + BLOCK_HEIGHT;
  
  // Vérifier si le bloc est au moins partiellement visible
  if (xEnd <= 0 || xStart >= MATRIX_WIDTH || yEnd <= 0 || yStart >= MATRIX_HEIGHT) {
#if DEBUG_SERIAL
    static uint8_t invisibleCounter = 0;
    if (invisibleCounter % 50 == 0) {
      Serial.print("Bloc invisible: x=");
      Serial.print(xStart);
      Serial.print("->"); 
      Serial.print(xEnd-1);
      Serial.print(", y=");
      Serial.print(yStart);
      Serial.print("->");
      Serial.println(yEnd-1);
    }
    invisibleCounter++;
#endif
    return;  // Le bloc n'est pas visible sur la matrice
  }
  
  // Limiter aux frontières de la matrice
  if (xStart < 0) xStart = 0;
  if (xEnd > MATRIX_WIDTH) xEnd = MATRIX_WIDTH;
  if (yStart < 0) yStart = 0;
  if (yEnd > MATRIX_HEIGHT) yEnd = MATRIX_HEIGHT;
  
#if DEBUG_SERIAL
  static uint8_t debugCounter = 0;
  if (debugCounter % 20 == 0) {
    Serial.print("Dessin bloc: x=");
    Serial.print(xStart);
    Serial.print("->");
    Serial.print(xEnd-1);
    Serial.print(", y=");
    Serial.print(yStart);
    Serial.print("->");
    Serial.println(yEnd-1);
  }
  debugCounter++;
#endif
  
  // Dessiner le bloc si visible
  if (xStart < xEnd && yStart < yEnd) {
    // Ajouter un cadre en couleur différente pour une meilleure visibilité
    uint8_t blockColor = block.color;
    
    // Dessiner le contour en jaune
    for (int16_t x = xStart; x < xEnd; x++) {
      if (x >= 0 && x < MATRIX_WIDTH) {
        if (yStart >= 0 && yStart < MATRIX_HEIGHT) {
          ht1632_plot(x, yStart, ORANGE);
        }
        if (yEnd-1 >= 0 && yEnd-1 < MATRIX_HEIGHT) {
          ht1632_plot(x, yEnd-1, ORANGE);
        }
      }
    }
    
    for (uint8_t y = yStart; y < yEnd; y++) {
      if (y >= 0 && y < MATRIX_HEIGHT) {
        if (xStart >= 0 && xStart < MATRIX_WIDTH) {
          ht1632_plot(xStart, y, ORANGE);
        }
        if (xEnd-1 >= 0 && xEnd-1 < MATRIX_WIDTH) {
          ht1632_plot(xEnd-1, y, ORANGE);
        }
      }
    }
    
    // Remplir l'intérieur du bloc
    for (int16_t x = xStart+1; x < xEnd-1; x++) {
      for (uint8_t y = yStart+1; y < yEnd-1; y++) {
        if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
          ht1632_plot(x, y, blockColor);
        }
      }
    }
    
#if DEBUG_SERIAL
    if (debugCounter % 50 == 0) {
      Serial.println("Bloc dessiné avec succès");
    }
#endif
  }
}

// Fonction pour passer à la prochaine note de la chanson
void nextNote() {
  const MusicNote* currentSong;
  uint8_t currentSongSize;
  
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
  }
  
  // Initialiser lastMoveTime pour garantir un déplacement immédiat
  lastMoveTime = 0;  // Forcer le premier déplacement rapidement
  
  // Créer le premier bloc
  nextNote();
  
#if DEBUG_SERIAL
  Serial.println("Premier bloc créé, prêt pour animation");
#endif
}

void loop() {
  unsigned long currentTime = millis();
  
  // Effacer la matrice
  ht1632_clear();
  
  // Variables pour éviter les créations multiples de notes
  static unsigned long lastNoteCreationTime = 0;
  static bool noteCreationInProgress = false;
  
  // Vérifier s'il est temps de créer une nouvelle note
  if (!songFinished && currentTime - lastNoteTime >= currentNoteDelay) {
    // Éviter les créations multiples en ajoutant une période de "refroidissement"
    if (currentTime - lastNoteCreationTime >= 800) {  // Attendre au moins 1 seconde entre les créations
      lastNoteCreationTime = currentTime;
      noteCreationInProgress = true;
      
#if DEBUG_SERIAL
      Serial.println("Création d'une nouvelle note...");
#endif
      
      nextNote();
      noteCreationInProgress = false;
    }
  }
  
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