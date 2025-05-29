#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"
#include "TimerOne.h"

// je suis michel
// Définition du pin du potentiomètre
const int potPin = A0;
// Ajout du bouton
const int buttonPin = 12;

// Constantes pour la matrice LED
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define BLOCK_HEIGHT 2
#define MAX_BLOCKS 12

#define MUSIQUE 0
#define BUZZER_PIN 3

// Constante pour la période d'interruption
#define TIMER_PERIOD 25000 // 25ms en microsecondes

// Compteur pour la fonction périodique
volatile uint16_t periodicCounter = 0;

// Structure pour stocker les informations d'un bloc 
typedef struct {
  int16_t x;              // Position horizontale, changé en int16_t pour éviter les dépassements
  uint8_t y;              // Position verticale (0 à 255), changé de int8_t à uint8_t
  uint8_t length;         // Longueur du bloc (0 à 255)
  uint8_t color : 3;      // Couleur du bloc sur 2 bits (0-3)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint16_t frequency;     // Fréquence de la note associée
  uint8_t needsUpdate : 1; // Flag pour indiquer si le bloc doit être mis à jour (déplacé)
  int16_t oldX;           // Ancienne position X pour effacer uniquement ce qui est nécessaire
} Block;

// Définition des couleurs
#define BLOCK_COLOR 3    // RED = 2 selon ht1632.h
#define BORDER_COLOR 3   // RED = 2 selon ht1632.h

// Variables pour le contrôle des blocs et la déduplication
Block blocks[MAX_BLOCKS];
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;
uint8_t lastNoteFrequency = 0;  // Pour suivre la dernière fréquence utilisée
bool lastNoteStillActive = false;  // Pour vérifier si la dernière note est encore active

// Variables globales pour le curseur
volatile int potValue = 0;
volatile uint8_t cursorY = 0;
uint8_t cursorY_displayed = 0;
uint8_t cursorY_last = 0;      // Dernière position affichée du curseur pour effacer correctement

bool cursorBlinking = false;
bool cursorVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 200; // ms

// État précédent du curseur pour détecter les changements
bool prevShouldShowCursor = true;


// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Pour suivre si la note du bloc est en cours de lecture
bool blockNotePlaying[MAX_BLOCKS] = {0};

// Flag global pour indiquer quand l'affichage doit être mis à jour
volatile bool displayNeedsUpdate = false;

// Flag pour indiquer si le curseur doit être affiché ou pas (pour clignotement)
volatile bool shouldShowCursor = true;



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
  if (startX >= MATRIX_WIDTH/2) {    blocks[blockIndex].x = startX;
    blocks[blockIndex].oldX = startX; // Initialiser la position oldX à la position de départ
    blocks[blockIndex].y = posY;
    blocks[blockIndex].length = length;
    blocks[blockIndex].color = BLOCK_COLOR;  // Utilisation de la couleur définie
    blocks[blockIndex].active = 1;
    blocks[blockIndex].frequency = note.frequency;
    blocks[blockIndex].needsUpdate = 0; // Initialement, pas besoin de mise à jour
    blockNotePlaying[blockIndex] = false;
    
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
}

// Affiche uniquement la tête du bloc (nouvelle colonne)
// Même logique que drawBlock pour la gestion du curseur et des colonnes vertes
void drawBlockHead(const Block& block) {
  int16_t headX = block.x;
  uint8_t yPos = block.y;
  
  // Ne rien faire si le bloc n'est pas actif
  if (!block.active) {
    return;
  }
    // Ne pas dessiner si la position est en dehors de l'écran à droite
  if (headX >= MATRIX_WIDTH) {
    return;
  }
    
  // Si la tête est à gauche de l'écran mais que le bloc est encore partiellement visible,
  // on ne dessine rien ici, mais le bloc reste actif pour que sa queue soit dessinée
  // C'est normal de ne rien dessiner quand headX < 0, mais le bloc doit continuer à se déplacer
  if (headX < 0) {
    return;
  }
  
  // Déterminer si nous sommes sur les colonnes vertes (2 ou 3)
  bool onGreenColumn = (headX == 2 || headX == 3);
  
  // Déterminer si nous sommes sur la position du curseur
  bool onCursorPosition = (yPos == cursorY_displayed || yPos == cursorY_displayed + 1);
  
  // Cas spécial: sur les colonnes vertes ET au niveau du curseur
  if (onGreenColumn && onCursorPosition) {
    // Ne rien dessiner - le curseur a priorité
    return;
  }
  
  // Cas spécial: sur les colonnes vertes mais pas au niveau du curseur
  if (onGreenColumn && !onCursorPosition) {
    // Le bloc passe devant la colonne verte
    if (yPos < MATRIX_HEIGHT) ht1632_plot(headX, yPos, block.color);
    if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(headX, yPos + 1, block.color);
    return;
  }
  
  // Cas normal: en dehors des colonnes vertes
  if (yPos < MATRIX_HEIGHT) ht1632_plot(headX, yPos, block.color);
  if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(headX, yPos + 1, block.color);
}

// Fonction pour dessiner un bloc sur la matrice
// Modifié : sur colonnes 2/3, le bloc passe devant la colonne verte sauf si il est sous le curseur (alors il passe derrière)
// Dessine le bloc complet (toutes les colonnes)
void drawBlock(Block block) {
  if (!block.active) return;

  int16_t xStart = block.x;
  int16_t xEnd = xStart + block.length;
  
  // Pour chaque colonne du bloc
  for (int16_t x = xStart; x < xEnd; x++) {
    // Simuler un drawBlockHead pour chaque colonne
    Block tempBlock = block;
    tempBlock.x = x;
    tempBlock.length = 1;
    drawBlockHead(tempBlock);
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



// Fonction périodique pour lire le potentiomètre et calculer la position cible du curseur
void updateCursorFromPot() {
  potValue = analogRead(potPin);
  int y = map(potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
  if (y < 0) y = 0;
  if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
  cursorY = (uint8_t)y;
}

// Efface le curseur 2x2 à une position donnée et restaure la colonne verte uniquement sur cette zone
void eraseCursor(uint8_t y) {
  // Mémoriser l'état des pixels avant effacement pour meilleure restauration
  static uint8_t previousPixelState[2][2] = {{1, 1}, {1, 1}}; // Par défaut, colonne verte
  
  for (uint8_t dx = 0; dx < 2; dx++) {
    for (uint8_t dy = 0; dy < 2; dy++) {
      uint8_t x = dx + 2; // colonnes 2 et 3
      uint8_t yPos = y + dy;
      
      if (yPos < MATRIX_HEIGHT) {
        // Vérifier si un bloc doit être affiché à cette position
        bool blocPresent = false;
        for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
          if (blocks[i].active) {
            int16_t xStart = blocks[i].x;
            int16_t xEnd = xStart + blocks[i].length;
            if (x >= xStart && x < xEnd && 
                (yPos == blocks[i].y || yPos == blocks[i].y + 1)) {
              // Restaurer la couleur du bloc
              ht1632_plot(x, yPos, blocks[i].color);
              previousPixelState[dx][dy] = blocks[i].color;
              blocPresent = true;
              break;
            }
          }
        }
        
        // Si aucun bloc n'est présent, restaurer la colonne verte
        if (!blocPresent) {
          ht1632_plot(x, yPos, 1); // Colonne verte (couleur 1)
          previousPixelState[dx][dy] = 1;
        }
      }
    }
  }
}

// Affiche le curseur 2x2 rouge à une position donnée
void drawCursor(uint8_t y) {
  for (uint8_t dx = 0; dx < 2; dx++) {
    for (uint8_t dy = 0; dy < 2; dy++) {
      uint8_t x = dx + 2; // colonnes 2 et 3
      uint8_t yPos = y + dy;
      
      if (yPos < MATRIX_HEIGHT) {
        ht1632_plot(x, yPos, 2); // Couleur rouge (2)
      }
    }
  }
}

// Fonction périodique pour déplacer le curseur bloc par bloc avec clignotement si demandé
void periodicMoveCursor() {
  static unsigned long lastCursorMove = 0;
  unsigned long now = millis();
  const unsigned long cursorDelay = 8; // ms, vitesse du curseur

  // Gestion du clignotement si activé
  if (cursorBlinking) {
    if (now - lastBlinkTime > blinkInterval) {
      cursorVisible = !cursorVisible;
      if (cursorVisible) {
        drawCursor(cursorY_displayed);
      } else {
        eraseCursor(cursorY_displayed);
      }
      lastBlinkTime = now;
    }
  } else {
    // Si pas de clignotement, s'assurer que le curseur est visible
    if (!cursorVisible) {
      drawCursor(cursorY_displayed);
      cursorVisible = true;
    }
  }

  // Déplacement du curseur
  if (now - lastCursorMove > cursorDelay) {
    if (cursorY_displayed < cursorY) {
      eraseCursor(cursorY_displayed);
      cursorY_displayed++;
      if (cursorVisible) drawCursor(cursorY_displayed);
    } else if (cursorY_displayed > cursorY) {
      eraseCursor(cursorY_displayed);
      cursorY_displayed--;
      if (cursorVisible) drawCursor(cursorY_displayed);
    }
    lastCursorMove = now;
  }
}

// Affiche uniquement la tête du bloc (nouvelle colonne)
// Efface uniquement la colonne et lignes concernées par la queue d'un bloc
void eraseBlockTail(const Block& block) {
  // Position de la queue (dernière colonne) du bloc
  int16_t tailX = block.oldX + block.length;
  
  // Si la queue est en dehors de l'écran, ne rien faire pour cette fonction
  // mais le bloc doit continuer à se déplacer
  if (tailX < 0 || tailX >= MATRIX_WIDTH) {
    return;
  }
  
  // Effacer les deux pixels occupés par la queue du bloc
  if (block.y < MATRIX_HEIGHT) {
    ht1632_plot(tailX, block.y, 0);
  }
  if (block.y + 1 < MATRIX_HEIGHT) {
    ht1632_plot(tailX, block.y + 1, 0);
  }
  
  // Si c'était sur les colonnes vertes, restaurer les colonnes selon les règles
  if (tailX == 2 || tailX == 3) {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
      // Vérifier si ce n'est pas la position du curseur
      bool notOnCursor = (y < cursorY_displayed || y >= cursorY_displayed + 2);
      
      // Vérifier si ce n'est pas la position d'un autre bloc actif
      bool notOnAnotherBlock = true;
      for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
        if (blocks[i].active && 
            tailX >= blocks[i].x && tailX < blocks[i].x + blocks[i].length && 
            (y == blocks[i].y || y == blocks[i].y + 1)) {
          notOnAnotherBlock = false;
          break;
        }
      }
      
      // Restaurer uniquement si ce n'est pas le curseur et pas un autre bloc
      if (notOnCursor && notOnAnotherBlock) {
        // Restaurer la colonne verte uniquement là où nécessaire
        ht1632_plot(tailX, y, 1);
      }
    }
  }
}

// Affiche uniquement la tête du bloc (nouvelle colonne)
// Fonction périodique pour déplacer les blocs et mettre à jour l'affichage pixel par pixel
void periodicMoveBlocks() {
  static unsigned long lastMoveTime = 0;
  unsigned long currentTime = millis();

  // Calcul dynamique du délai de déplacement en fonction du tempo
  // 1 temps = 1 noire = 60000 / TEMPO_BPM ms
  // Pour un effet plus fluide, on peut choisir 1 déplacement par croche (diviser par 2)
  uint16_t moveDelay = 60000 / TEMPO_BPM; // 1 déplacement par temps (noire)
  // uint16_t moveDelay = 60000 / TEMPO_BPM / 2; // 1 déplacement par croche (optionnel)

  if (currentTime - lastMoveTime > moveDelay) {
    // Recherche du bloc prioritaire (le plus à gauche) qui occupe x=2 ou x=3
    int8_t blockToPlay = -1;
    int16_t minX = 1000;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        // Le bloc occupe-t-il x=2 ou x=3 ?
        if ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) {
          if (xStart < minX) {
            minX = xStart;
            blockToPlay = i;
          }
        }
      }
    }
    // Gestion du buzzer : jouer la note du bloc prioritaire, arrêter sinon
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        bool onGreen = (2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd);
        if (onGreen && i == blockToPlay) {
          if (!blockNotePlaying[i] && blocks[i].frequency > 0) {
#if MUSIQUE
            tone(BUZZER_PIN, blocks[i].frequency);
#endif
            blockNotePlaying[i] = true;
          }
        } else {
          if (blockNotePlaying[i]) {
#if MUSIQUE
            noTone(BUZZER_PIN);
#endif
            blockNotePlaying[i] = false;
          }
        }
      } else {
        blockNotePlaying[i] = false;
      }
    }    // Déplacement des blocs
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        eraseBlockTail(blocks[i]);
        blocks[i].x--;
        drawBlockHead(blocks[i]);
        
        // Désactiver le bloc seulement quand il est complètement sorti de l'écran
        // (quand sa position x + sa longueur est <= 0)
        if (blocks[i].x + blocks[i].length < -1) {
          blocks[i].active = 0;
          blockNotePlaying[i] = false;
        }
        // Continuer à déplacer les blocs même quand ils sont partiellement hors écran
      }
    }
    lastMoveTime = currentTime;
  }
}

// Affiche les colonnes 2 et 3 en vert (statique, hors zone curseur et hors zone bloc)
void drawStaticColumnsExceptCursorAndBlocks() {
  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    bool blocPresent = false;
    // Vérifie si un bloc actif occupe la colonne 2 ou 3 à cette hauteur
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        if ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) {
          if (y == blocks[i].y || y == blocks[i].y + 1) {
            blocPresent = true;
            break;
          }
        }
      }
    }
    // Ne pas dessiner sur la zone du curseur affiché ni sur la zone d'un bloc
    if (!blocPresent && (y < cursorY_displayed || y >= cursorY_displayed + 2)) {
      ht1632_plot(2, y, 1);
      ht1632_plot(3, y, 1);
    }
  }
}

// Restaure uniquement les colonnes vertes aux positions spécifiques sans toucher au reste
void restoreGreenColumn(uint8_t x, uint8_t y) {
  if ((x == 2 || x == 3) && 
      (y < cursorY_displayed || y >= cursorY_displayed + 2)) {
    // Vérifier qu'aucun bloc ne passe à cette position
    bool blocPresent = false;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        if (x >= xStart && x < xEnd && 
            (y == blocks[i].y || y == blocks[i].y + 1)) {
          blocPresent = true;
          break;
        }
      }
    }
    if (!blocPresent) {
      ht1632_plot(x, y, 1); // Restaure en vert
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
  
  // Configuration du bouton en entrée avec pull-up interne
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initialiser les blocs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    blocks[i].active = 0;
    blocks[i].color = BLOCK_COLOR;  // Initialiser la couleur explicitement
    blocks[i].needsUpdate = 0;      // Pas de mise à jour nécessaire au départ
    blockNotePlaying[i] = false;
  }
  
  // Initialiser les flags d'affichage
  displayNeedsUpdate = true; // Forcer un premier affichage
  shouldShowCursor = true;   // Curseur visible initialement
  
  // Initialiser les états du curseur
  cursorY_last = cursorY_displayed;
  prevShouldShowCursor = shouldShowCursor;
  
  // Affichage initial de la matrice
  ht1632_clear();
  drawStaticColumnsExceptCursorAndBlocks();
  drawCursor(cursorY_displayed);
  
  // Créer le premier bloc
  nextNote();
  
#if MUSIQUE
  pinMode(BUZZER_PIN, OUTPUT);
#endif
  
  // Initialiser le Timer1 pour appeler periodicFunction toutes les 25ms
  Timer1.initialize(TIMER_PERIOD);
  Timer1.attachInterrupt(periodicFunction);
  
  // Réinitialiser le compteur périodique
  periodicCounter = 0;
}

// Fonction séparée pour la gestion audio
void updateAudio() {
  static uint8_t lastPlayingBlock = 255; // Aucun bloc
  uint8_t currentPlayingBlock = 255;
  
  // Trouver le bloc prioritaire à jouer (le plus à gauche sur colonnes 2-3)
  int16_t minX = MATRIX_WIDTH;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].frequency > 0 && blockNotePlaying[i]) {
      if (blocks[i].x < minX) {
        minX = blocks[i].x;
        currentPlayingBlock = i;
      }
    }
  }
  
  // Jouer uniquement si le bloc change pour éviter les démarrages/arrêts fréquents
  if (currentPlayingBlock != lastPlayingBlock) {
    if (currentPlayingBlock != 255) {
#if MUSIQUE
      tone(BUZZER_PIN, blocks[currentPlayingBlock].frequency);
#endif
    } else {
#if MUSIQUE
      noTone(BUZZER_PIN);
#endif
    }
    lastPlayingBlock = currentPlayingBlock;
  }
}

// Fonction appelée périodiquement par TimerOne (toutes les 25ms)
void periodicFunction() {
  // Incrémenter le compteur périodique
  periodicCounter++;
  
  // Lecture du bouton (4 fois par seconde = tous les 10 cycles)
  if (periodicCounter % 10 == 0) {
    static bool lastButtonState = HIGH;
    bool buttonState = digitalRead(buttonPin);
    
    // Gestion du bouton avec anti-rebond logiciel
    if (buttonState != lastButtonState) {
      if (buttonState == LOW) {
        cursorBlinking = true;
      } else {
        cursorBlinking = false;
        shouldShowCursor = true; // Toujours visible quand on relâche le bouton
      }
      lastButtonState = buttonState;
      displayNeedsUpdate = true;
    }
  }
  
  // Lecture du potentiomètre (10 fois par seconde = tous les 4 cycles)
  if (periodicCounter % 4 == 0) {
    if (digitalRead(buttonPin) == HIGH) {
      // Lire la valeur actuelle du potentiomètre
      int newPotValue = analogRead(potPin);
      
      // Ne mettre à jour que si la valeur a suffisamment changé (évite les micro-variations)
      if (abs(newPotValue - potValue) > 5) {
        potValue = newPotValue;
        int y = map(potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
        if (y < 0) y = 0;
        if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
        
        // N'actualiser que si la position a réellement changé
        if (cursorY != (uint8_t)y) {
          cursorY = (uint8_t)y;
          displayNeedsUpdate = true;
        }
      }
    }
  }
  
  // Gestion du clignotement du curseur (5 fois par seconde = tous les 8 cycles)
  if (periodicCounter % 8 == 0) {
    if (cursorBlinking) {
      shouldShowCursor = !shouldShowCursor; // Inverser l'état d'affichage du curseur
      displayNeedsUpdate = true;
    } else if (!shouldShowCursor) {
      shouldShowCursor = true; // S'assurer que le curseur est visible si pas en mode clignotement
      displayNeedsUpdate = true;
    }
  }
  
  // Déplacement du curseur (tous les cycles, mais progressif)
  if (cursorY_displayed < cursorY) {
    cursorY_displayed++;
    displayNeedsUpdate = true;
  } else if (cursorY_displayed > cursorY) {
    cursorY_displayed--;
    displayNeedsUpdate = true;
  }
  
  // Déplacement des blocs (2 fois par seconde = tous les 20 cycles)
  if (periodicCounter % 20 == 0) {
    // Déterminer le bloc prioritaire (le plus à gauche) qui occupe x=2 ou x=3
    int8_t blockToPlay = -1;
    int16_t minX = 1000;
    
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        
        // Le bloc occupe-t-il x=2 ou x=3 ?
        if ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) {
          if (xStart < minX) {
            minX = xStart;
            blockToPlay = i;
          }
        }
      }
    }
    
    // Gestion du buzzer : marquer les blocs qui doivent jouer une note
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        bool onGreen = (2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd);
        
        if (onGreen && i == blockToPlay) {
          blockNotePlaying[i] = true;
        } else {
          blockNotePlaying[i] = false;
        }
      } else {
        blockNotePlaying[i] = false;
      }
    }
    
    // Marquer les blocs pour déplacement (sera effectué dans loop())
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        blocks[i].needsUpdate = 1; // Marquer le bloc pour mise à jour
        displayNeedsUpdate = true; // Indiquer que l'affichage doit être mis à jour
      }
    }
  }
  
  // Création de nouvelles notes (1 fois par seconde = tous les 40 cycles)
  if (periodicCounter % 40 == 0) {
    if (!songFinished) {
      static bool noteCreationInProgress = false;
      
      if (!noteCreationInProgress) {
        noteCreationInProgress = true;
        nextNote();
        noteCreationInProgress = false;
        displayNeedsUpdate = true;
      }
    }
    
    // Gestion de la fin de séquence musicale (toutes les 2 secondes = tous les 80 cycles)
    if (songFinished && periodicCounter % 80 == 0) {
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
      }
    }
  }
}

void loop() {
  // Variables pour détecter les changements
  static uint32_t lastAudioUpdate = 0;
  uint32_t currentTime = periodicCounter * TIMER_PERIOD / 1000; // Temps basé sur le compteur sans utiliser millis()
  
  bool cursorStateChanged = (shouldShowCursor != prevShouldShowCursor);
  bool cursorPositionChanged = (cursorY_displayed != cursorY_last);
  // Phase 1 : Mettre à jour les blocs si nécessaire (sans affichage)
  bool anyBlockMoved = false;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].needsUpdate) {
      // Sauvegarder l'ancienne position avant de mettre à jour
      blocks[i].oldX = blocks[i].x;
      
      // Déplacer le bloc
      blocks[i].x--;
      blocks[i].needsUpdate = 0; // Réinitialiser le flag de mise à jour
      anyBlockMoved = true;      // Vérifier si le bloc est complètement sorti de l'écran
      if (blocks[i].x + blocks[i].length < -1) {
        // Le bloc est complètement sorti de l'écran, le désactiver
        blocks[i].active = 0;
        blockNotePlaying[i] = false;
      }
      // Laisser les blocs continuer à se déplacer même quand x < 0
      // Ne pas les bloquer à la position x=0
    }
  }
  
  // Limiter les mises à jour uniquement si nécessaire
  if (!displayNeedsUpdate && !cursorStateChanged && !cursorPositionChanged && !anyBlockMoved) {
    // Si aucun changement visuel, mettre à jour uniquement l'audio si nécessaire
    if (currentTime - lastAudioUpdate >= 40) { // ~40ms entre mises à jour audio
      updateAudio();
      lastAudioUpdate = currentTime;
    }
    return; // Sortir sans mise à jour visuelle
  }
  
  // Réinitialiser le flag de mise à jour
  displayNeedsUpdate = false;
  lastAudioUpdate = currentTime;
  
  // Mise à jour de l'affichage de manière ciblée
  
  // Mise à jour du curseur si nécessaire
  if (cursorStateChanged || cursorPositionChanged) {
    // Effacer l'ancien curseur seulement s'il était visible
    if (prevShouldShowCursor) {
      eraseCursor(cursorY_last);
    }
    
    // Dessiner le nouveau curseur s'il doit être visible
    if (shouldShowCursor) {
      drawCursor(cursorY_displayed);
    }
    
    // Mettre à jour les variables d'état
    cursorY_last = cursorY_displayed;
    prevShouldShowCursor = shouldShowCursor;
  }
    // Mise à jour des blocs - seulement effacer et redessiner les pixels modifiés
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      if (blocks[i].oldX != blocks[i].x) {
        // Effacer l'ancienne queue du bloc (pixel précédent)
        eraseBlockTail(blocks[i]);
        
        // Dessiner la nouvelle tête du bloc
        drawBlockHead(blocks[i]);
      }      // Vérifier spécifiquement si le bloc est en train de sortir de l'écran
      if (blocks[i].x < 0 && blocks[i].x + blocks[i].length > 0) {
        // Le bloc est partiellement visible, s'assurer que seules les colonnes
        // visibles sont affichées
        
        // Pour chaque colonne visible, l'afficher
        for (int16_t j = 0; j < blocks[i].length; j++) {
          int16_t colX = blocks[i].x + j;
          
          // N'afficher que les colonnes qui sont visibles à l'écran
          if (colX >= 0 && colX < MATRIX_WIDTH) {            // Cette colonne est visible, l'afficher
            if (blocks[i].y < MATRIX_HEIGHT) {
              ht1632_plot(colX, blocks[i].y, blocks[i].color);
            }
            if (blocks[i].y + 1 < MATRIX_HEIGHT) {
              ht1632_plot(colX, blocks[i].y + 1, blocks[i].color);
            }
          }          // Les colonnes qui sortent de l'écran (colX < 0) ne sont pas dessinées
          // mais le bloc continue à avancer jusqu'à ce que blocks[i].x + blocks[i].length <= 0
        }
      }
    }
  }
  
  // Gestion audio des blocs
  updateAudio();
}

