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

// Constantes pour les états du curseur
#define CURSOR_HIDDEN     0
#define CURSOR_VISIBLE    1
#define CURSOR_BLINKING   2

// Constantes pour les flags de changement
#define FLAG_UNCHANGED    0
#define FLAG_CHANGED      1

// Structure pour stocker les informations d'un bloc 
typedef struct {
  int16_t x;              // Position horizontale, changé en int16_t pour éviter les dépassements
  uint8_t y;              // Position verticale (0 à 255), changé de int8_t à uint8_t
  uint8_t length;         // Longueur du bloc (0 à 255)
  uint8_t color : 3;      // Couleur du bloc sur 2 bits (0-3)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint16_t frequency;     // Fréquence de la note associée
  int16_t oldX;           // Ancienne position X pour effacer uniquement ce qui est nécessaire
} Block;

// Structure pour gérer toutes les données du curseur
typedef struct {
  volatile int potValue;                    // Valeur du potentiomètre
  volatile uint8_t y;                       // Position Y cible du curseur
  uint8_t displayed;                        // Position Y actuellement affichée
  uint8_t last;                            // Dernière position affichée pour effacement
  uint8_t state;                           // État du curseur (HIDDEN/VISIBLE/BLINKING)
  uint8_t visibility;                      // Visibilité actuelle (0=caché, 1=visible)
  unsigned long lastBlinkTime;             // Temps du dernier clignotement
  uint8_t prevShouldShow;                  // État précédent pour détection de changement
  volatile uint8_t shouldShow;             // Doit-on afficher le curseur (0=non, 1=oui)
  volatile uint8_t stateChanged;           // Flag de changement d'état
  volatile uint8_t positionChanged;        // Flag de changement de position
} Cursor;

// Définition des couleurs
#define BLOCK_COLOR 3    // RED = 2 selon ht1632.h
#define BORDER_COLOR 3   // RED = 2 selon ht1632.h

// Variables pour le contrôle des blocs et la déduplication
Block blocks[MAX_BLOCKS];
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;
// Constantes pour les états des blocs
#define BLOCK_INACTIVE    0
#define BLOCK_ACTIVE      1

// Constantes pour les états audio
#define AUDIO_SILENT      0
#define AUDIO_PLAYING     1

// Constantes pour les flags de mise à jour
#define UPDATE_NONE       0
#define UPDATE_NEEDED     1

uint8_t lastNoteFrequency = 0;  // Pour suivre la dernière fréquence utilisée
uint8_t lastNoteStillActive = 0;  // Pour vérifier si la dernière note est encore active

// Instance globale du curseur
Cursor cursor = {0};

// Constante pour le clignotement du curseur
const unsigned long blinkInterval = 200; // ms


// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Pour suivre si la note du bloc est en cours de lecture
uint8_t blockNotePlaying[MAX_BLOCKS] = {0};

// Flag global pour indiquer quand l'affichage doit être mis à jour
volatile uint8_t displayNeedsUpdate = UPDATE_NONE;

// Variables pour détecter les changements des blocs (gérées par la fonction périodique)
volatile uint8_t anyBlockMoved = FLAG_UNCHANGED;



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

// Fonction unifiée pour vérifier les collisions (positions horizontales et verticales)
uint8_t isPositionOccupied(int16_t x, int8_t posY = -1) {
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active == BLOCK_ACTIVE) {
      // Vérification horizontale (colonne)
      uint8_t horizontalMatch = (x >= blocks[i].x && x < blocks[i].x + blocks[i].length) ? 1 : 0;
      
      // Si posY n'est pas spécifié (-1), on vérifie seulement horizontal
      if (posY == -1) {
        if (horizontalMatch) return 1;
      } else {
        // Vérification verticale et horizontale
        uint8_t verticalMatch = (blocks[i].y == posY) ? 1 : 0;
        if (horizontalMatch || verticalMatch) return 1;
      }
    }
  }
  return 0;
}

// Fonction pour créer un nouveau bloc en fonction d'une note
void createNewBlock(const MusicNote* noteArray, uint8_t noteIndex) {
  // Récupérer les valeurs de la note depuis PROGMEM
  MusicNote note;
  getNote(noteArray, noteIndex, &note);
    // Vérifier si une note similaire est déjà active
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active == BLOCK_ACTIVE && blocks[i].y == getPositionYFromFrequency(note.frequency)) {
      if (blocks[i].x > MATRIX_WIDTH/2) {  // Si le bloc est encore dans la moitié droite
        return;  // Ne pas créer de nouveau bloc
      }
    }
  }
    // Vérification globale: éviter de créer trop de blocs actifs simultanément
  uint8_t activeCount = 0;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active == BLOCK_ACTIVE) {
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
    if (blocks[i].active == BLOCK_INACTIVE) {
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
  uint8_t posY = getPositionYFromFrequency(note.frequency);  // Vérification pour éviter la création si la même note est déjà active
  // (Supprimé cette vérification trop restrictive qui empêche la création de blocs)
  
  // Vérification simplifiée pour les positions verticales - seulement pour la position exacte
  uint8_t positionConflict = BLOCK_INACTIVE;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active == BLOCK_ACTIVE) {
      // Conflit si même position exacte
      if (blocks[i].y == posY) {
        positionConflict = BLOCK_ACTIVE;
        break;
      }
    }
  }
  
  if (positionConflict == BLOCK_ACTIVE) {
#if DEBUG_SERIAL
    Serial.println("Conflit de position Y détecté, création annulée");
#endif
    return; // Annuler la création plutôt que de chercher une position alternative
  }
  
  // S'assurer que le bloc ne dépasse pas les limites de la matrice
  if (posY + BLOCK_HEIGHT > MATRIX_HEIGHT) {
    posY = MATRIX_HEIGHT - BLOCK_HEIGHT;
  }
  // Position horizontale : commencer juste au bord droit visible de l'écran
  int16_t startX = MATRIX_WIDTH - 1;  // Position au bord droit visible
  
  // Vérification simplifiée des superpositions horizontales
  // (Supprimé la vérification des colonnes occupées qui était trop restrictive)
  
  // Vérification des superpositions 
  uint8_t positionOccupied = BLOCK_INACTIVE;
  do {
    positionOccupied = BLOCK_INACTIVE;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE && i != blockIndex) {
        if ((startX <= blocks[i].x + blocks[i].length) && 
            (startX + length >= blocks[i].x)) {
          positionOccupied = BLOCK_ACTIVE;
          startX = blocks[i].x - length - 1;
          break;
        }
      }
    }
  } while (positionOccupied == BLOCK_ACTIVE && startX >= MATRIX_WIDTH/2);
  // Création du bloc si l'espace est disponible
  if (startX >= MATRIX_WIDTH/2) {    blocks[blockIndex].x = startX;
    blocks[blockIndex].oldX = startX; // Initialiser la position oldX à la position de départ
    blocks[blockIndex].y = posY;
    blocks[blockIndex].length = length;    blocks[blockIndex].color = BLOCK_COLOR;  // Utilisation de la couleur définie    blocks[blockIndex].active = BLOCK_ACTIVE;
    blocks[blockIndex].frequency = note.frequency;
    blockNotePlaying[blockIndex] = AUDIO_SILENT;
    
    #if DEBUG_SERIAL //suivi des blocs créés
    Serial.print("Nouveau bloc: x=");
    Serial.print(startX);
    Serial.print(", y=");
    Serial.print(posY);
    Serial.print(", len=");
    Serial.println(length);
    #endif
  }  else {
    blocks[blockIndex].active = BLOCK_INACTIVE;
  }  // Mettre à jour les informations de la dernière note
  lastNoteFrequency = note.frequency;
  lastNoteStillActive = BLOCK_ACTIVE;
}

// Affiche uniquement la tête du bloc (nouvelle colonne)
// Même logique que drawBlock pour la gestion du curseur et des colonnes vertes
void drawBlockHead(const Block& block) {
  int16_t headX = block.x;
  uint8_t yPos = block.y;
  
  // Ne rien faire si le bloc n'est pas actif
  if (block.active == BLOCK_INACTIVE) {
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
  uint8_t onGreenColumn = (headX == 2 || headX == 3) ? 1 : 0;
    // Déterminer si nous sommes sur la position du curseur
  uint8_t onCursorPosition = (yPos == cursor.displayed || yPos == cursor.displayed + 1) ? 1 : 0;
  
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
  if (block.active == BLOCK_INACTIVE) return;

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
  static uint8_t lastSongPart = 255; // Variable pour mémoriser la dernière partie de chanson
  if (songPosition == lastNotePosition && currentSongPart == lastSongPart) {
#if DEBUG_SERIAL
    Serial.println("Note déjà créée, évitement de duplication");
#endif
    return;
  }  
  // Mémoriser cette position pour éviter la duplication
  lastNotePosition = songPosition;
  lastSongPart = currentSongPart;
  
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
  cursor.potValue = analogRead(potPin);
  int y = map(cursor.potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
  if (y < 0) y = 0;
  if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
  cursor.y = (uint8_t)y;
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
        uint8_t blocPresent = BLOCK_INACTIVE;
        for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
          if (blocks[i].active == BLOCK_ACTIVE) {
            int16_t xStart = blocks[i].x;
            int16_t xEnd = xStart + blocks[i].length;
            if (x >= xStart && x < xEnd && 
                (yPos == blocks[i].y || yPos == blocks[i].y + 1)) {
              // Restaurer la couleur du bloc
              ht1632_plot(x, yPos, blocks[i].color);
              previousPixelState[dx][dy] = blocks[i].color;
              blocPresent = BLOCK_ACTIVE;
              break;
            }
          }
        }
        
        // Si aucun bloc n'est présent, restaurer la colonne verte
        if (blocPresent == BLOCK_INACTIVE) {
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
  if (cursor.state == CURSOR_BLINKING) {
    if (now - cursor.lastBlinkTime > blinkInterval) {
      cursor.visibility = (cursor.visibility == CURSOR_VISIBLE) ? CURSOR_HIDDEN : CURSOR_VISIBLE;
      if (cursor.visibility == CURSOR_VISIBLE) {
        drawCursor(cursor.displayed);
      } else {
        eraseCursor(cursor.displayed);
      }
      cursor.lastBlinkTime = now;
    }
  } else {
    // Si pas de clignotement, s'assurer que le curseur est visible
    if (cursor.visibility != CURSOR_VISIBLE) {
      drawCursor(cursor.displayed);
      cursor.visibility = CURSOR_VISIBLE;
    }
  }
  // Déplacement du curseur
  if (now - lastCursorMove > cursorDelay) {
    if (cursor.displayed < cursor.y) {
      eraseCursor(cursor.displayed);
      cursor.displayed++;
      if (cursor.visibility == CURSOR_VISIBLE) drawCursor(cursor.displayed);
    } else if (cursor.displayed > cursor.y) {
      eraseCursor(cursor.displayed);
      cursor.displayed--;
      if (cursor.visibility == CURSOR_VISIBLE) drawCursor(cursor.displayed);
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
  if (tailX == 2 || tailX == 3) {    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {      // Vérifier si ce n'est pas la position du curseur
      uint8_t notOnCursor = (y < cursor.displayed || y >= cursor.displayed + 2) ? 1 : 0;
      
      // Vérifier si ce n'est pas la position d'un autre bloc actif
      uint8_t notOnAnotherBlock = 1;
      for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
        if (blocks[i].active == BLOCK_ACTIVE && 
            tailX >= blocks[i].x && tailX < blocks[i].x + blocks[i].length && 
            (y == blocks[i].y || y == blocks[i].y + 1)) {
          notOnAnotherBlock = 0;
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
    }    // Gestion du buzzer : jouer la note du bloc prioritaire, arrêter sinon
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        uint8_t onGreen = ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) ? 1 : 0;
        if (onGreen && i == blockToPlay) {
          if (blockNotePlaying[i] == AUDIO_SILENT && blocks[i].frequency > 0) {
#if MUSIQUE
            tone(BUZZER_PIN, blocks[i].frequency);
#endif
            blockNotePlaying[i] = AUDIO_PLAYING;
          }
        } else {
          if (blockNotePlaying[i] == AUDIO_PLAYING) {
#if MUSIQUE
            noTone(BUZZER_PIN);
#endif
            blockNotePlaying[i] = AUDIO_SILENT;
          }
        }
      } else {
        blockNotePlaying[i] = AUDIO_SILENT;
      }
    }    // Déplacement des blocs
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        eraseBlockTail(blocks[i]);
        blocks[i].x--;
        drawBlockHead(blocks[i]);
        
        // Désactiver le bloc seulement quand il est complètement sorti de l'écran
        // (quand sa position x + sa longueur est <= 0)
        if (blocks[i].x + blocks[i].length < -1) {
          blocks[i].active = BLOCK_INACTIVE;
          blockNotePlaying[i] = AUDIO_SILENT;
        }
        // Continuer à déplacer les blocs même quand ils sont partiellement hors écran
      }
    }
    lastMoveTime = currentTime;
  }
}

// Affiche les colonnes 2 et 3 en vert (statique, hors zone curseur et hors zone bloc)
void drawStaticColumnsExceptCursorAndBlocks() {  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    uint8_t blocPresent = BLOCK_INACTIVE;
    // Vérifie si un bloc actif occupe la colonne 2 ou 3 à cette hauteur
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        if ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) {
          if (y == blocks[i].y || y == blocks[i].y + 1) {
            blocPresent = BLOCK_ACTIVE;
            break;
          }
        }
      }
    }    // Ne pas dessiner sur la zone du curseur affiché ni sur la zone d'un bloc
    if (blocPresent == BLOCK_INACTIVE && (y < cursor.displayed || y >= cursor.displayed + 2)) {
      ht1632_plot(2, y, 1);
      ht1632_plot(3, y, 1);
    }
  }
}

// Restaure uniquement les colonnes vertes aux positions spécifiques sans toucher au reste
void restoreGreenColumn(uint8_t x, uint8_t y) {  if ((x == 2 || x == 3) && 
      (y < cursor.displayed || y >= cursor.displayed + 2)) {
    // Vérifier qu'aucun bloc ne passe à cette position
    uint8_t blocPresent = BLOCK_INACTIVE;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        if (x >= xStart && x < xEnd && 
            (y == blocks[i].y || y == blocks[i].y + 1)) {
          blocPresent = BLOCK_ACTIVE;
          break;
        }
      }
    }
    if (blocPresent == BLOCK_INACTIVE) {
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
  pinMode(buttonPin, INPUT_PULLUP);  // Initialiser les blocs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    blocks[i].active = BLOCK_INACTIVE;
    blocks[i].color = BLOCK_COLOR;  // Initialiser la couleur explicitement
    blockNotePlaying[i] = AUDIO_SILENT;
  }
  
  // Initialiser le curseur
  cursor.potValue = 0;
  cursor.y = 0;
  cursor.displayed = 0;
  cursor.last = 0;
  cursor.state = CURSOR_VISIBLE;
  cursor.visibility = CURSOR_VISIBLE;
  cursor.lastBlinkTime = 0;
  cursor.prevShouldShow = CURSOR_VISIBLE;
  cursor.shouldShow = CURSOR_VISIBLE;
  cursor.stateChanged = FLAG_UNCHANGED;
  cursor.positionChanged = FLAG_UNCHANGED;
  
  // Initialiser les flags d'affichage
  displayNeedsUpdate = UPDATE_NEEDED; // Forcer un premier affichage
  
  // Affichage initial de la matrice
  ht1632_clear();
  drawStaticColumnsExceptCursorAndBlocks();
  drawCursor(cursor.displayed);
  
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
    if (blocks[i].active == BLOCK_ACTIVE && blocks[i].frequency > 0 && blockNotePlaying[i] == AUDIO_PLAYING) {
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
    static uint8_t lastButtonState = HIGH;
    uint8_t buttonState = digitalRead(buttonPin);
    // Gestion du bouton avec anti-rebond logiciel
    if (buttonState != lastButtonState) {
      if (buttonState == LOW) {
        cursor.state = CURSOR_BLINKING;
      } else {
        cursor.state = CURSOR_VISIBLE;
        cursor.shouldShow = CURSOR_VISIBLE; // Toujours visible quand on relâche le bouton
      }
      lastButtonState = buttonState;
      displayNeedsUpdate = UPDATE_NEEDED;
    }
  }
  
  // Lecture du potentiomètre (10 fois par seconde = tous les 4 cycles)
  if (periodicCounter % 4 == 0) {
    if (digitalRead(buttonPin) == HIGH) {
      // Lire la valeur actuelle du potentiomètre
      int newPotValue = analogRead(potPin);
        // Ne mettre à jour que si la valeur a suffisamment changé (évite les micro-variations)
      if (abs(newPotValue - cursor.potValue) > 5) {
        cursor.potValue = newPotValue;
        int y = map(cursor.potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
        if (y < 0) y = 0;
        if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
          // N'actualiser que si la position a réellement changé
        if (cursor.y != (uint8_t)y) {
          cursor.y = (uint8_t)y;
          displayNeedsUpdate = UPDATE_NEEDED;
        }
      }
    }
  }
  
  // Gestion du clignotement du curseur (5 fois par seconde = tous les 8 cycles)
  if (periodicCounter % 8 == 0) {
    if (cursor.state == CURSOR_BLINKING) {
      cursor.shouldShow = (cursor.shouldShow == CURSOR_VISIBLE) ? CURSOR_HIDDEN : CURSOR_VISIBLE; // Inverser l'état d'affichage du curseur
      displayNeedsUpdate = UPDATE_NEEDED;
    } else if (cursor.shouldShow != CURSOR_VISIBLE) {
      cursor.shouldShow = CURSOR_VISIBLE; // S'assurer que le curseur est visible si pas en mode clignotement
      displayNeedsUpdate = UPDATE_NEEDED;
    }
  }
  
  // Déplacement du curseur (tous les cycles, mais progressif)
  static uint8_t prevCursorY_displayed = 0;
  static uint8_t prevShouldShowCursor_internal = CURSOR_VISIBLE;
  
  cursor.positionChanged = (cursor.displayed != prevCursorY_displayed) ? FLAG_CHANGED : FLAG_UNCHANGED;  cursor.stateChanged = (cursor.shouldShow != prevShouldShowCursor_internal) ? FLAG_CHANGED : FLAG_UNCHANGED;
  
  if (cursor.displayed < cursor.y) {
    cursor.displayed++;
    displayNeedsUpdate = UPDATE_NEEDED;
  } else if (cursor.displayed > cursor.y) {
    cursor.displayed--;
    displayNeedsUpdate = UPDATE_NEEDED;
  }
  
  // Mettre à jour les valeurs précédentes
  prevCursorY_displayed = cursor.displayed;
  prevShouldShowCursor_internal = cursor.shouldShow;
  
  // Déplacement des blocs (2 fois par seconde = tous les 20 cycles)
  if (periodicCounter % 20 == 0) {
    // Déterminer le bloc prioritaire (le plus à gauche) qui occupe x=2 ou x=3
    int8_t blockToPlay = -1;
    int16_t minX = 1000;
      for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
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
      if (blocks[i].active == BLOCK_ACTIVE) {
        int16_t xStart = blocks[i].x;
        int16_t xEnd = xStart + blocks[i].length;
        uint8_t onGreen = ((2 >= xStart && 2 < xEnd) || (3 >= xStart && 3 < xEnd)) ? 1 : 0;
        
        if (onGreen && i == blockToPlay) {
          blockNotePlaying[i] = AUDIO_PLAYING;
        } else {
          blockNotePlaying[i] = AUDIO_SILENT;
        }
      } else {
        blockNotePlaying[i] = AUDIO_SILENT;
      }    }
    
    // Marquer les blocs pour déplacement et les déplacer immédiatement
    anyBlockMoved = FLAG_UNCHANGED; // Réinitialiser le flag
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        // Sauvegarder l'ancienne position avant de mettre à jour
        blocks[i].oldX = blocks[i].x;
        
        // Déplacer le bloc
        blocks[i].x--;
        anyBlockMoved = FLAG_CHANGED;
        
        // Vérifier si le bloc est complètement sorti de l'écran
        if (blocks[i].x + blocks[i].length < -1) {
          // Le bloc est complètement sorti de l'écran, le désactiver
          blocks[i].active = BLOCK_INACTIVE;
          blockNotePlaying[i] = AUDIO_SILENT;
        }
        
        displayNeedsUpdate = UPDATE_NEEDED; // Indiquer que l'affichage doit être mis à jour
      }
    }
  }
    // Création de nouvelles notes (1 fois par seconde = tous les 40 cycles)
  if (periodicCounter % 40 == 0) {
    if (songFinished == 0) {
      static uint8_t noteCreationInProgress = BLOCK_INACTIVE;
      
      if (noteCreationInProgress == BLOCK_INACTIVE) {
        noteCreationInProgress = BLOCK_ACTIVE;
        nextNote();
        noteCreationInProgress = BLOCK_INACTIVE;
        displayNeedsUpdate = UPDATE_NEEDED;
      }
    }    // Gestion de la fin de séquence musicale (toutes les 2 secondes = tous les 80 cycles)
    if (songFinished && periodicCounter % 80 == 0) {
      uint8_t allBlocksInactive = BLOCK_ACTIVE; // Commence par supposer qu'aucun bloc n'est actif
      
      for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
        if (blocks[i].active == BLOCK_ACTIVE) {
          allBlocksInactive = BLOCK_INACTIVE; // Il y a encore des blocs actifs
          break;
        }
      }
      
      if (allBlocksInactive == BLOCK_ACTIVE) { // Aucun bloc actif trouvé
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
  uint32_t currentTime = periodicCounter * TIMER_PERIOD / 1000; // Temps basé sur le compteur sans utiliser millis()  // Limiter les mises à jour uniquement si nécessaire
  if (displayNeedsUpdate == UPDATE_NONE && cursor.stateChanged == FLAG_UNCHANGED && cursor.positionChanged == FLAG_UNCHANGED && anyBlockMoved == FLAG_UNCHANGED) {
    // Si aucun changement visuel, mettre à jour uniquement l'audio si nécessaire
    if (currentTime - lastAudioUpdate >= 40) { // ~40ms entre mises à jour audio
      updateAudio();
      lastAudioUpdate = currentTime;
    }
    return; // Sortir sans mise à jour visuelle
  }
  
  // Réinitialiser le flag de mise à jour
  displayNeedsUpdate = UPDATE_NONE;
  lastAudioUpdate = currentTime;
  
  // Mise à jour de l'affichage de manière ciblée  // Mise à jour du curseur si nécessaire
  if (cursor.stateChanged == FLAG_CHANGED || cursor.positionChanged == FLAG_CHANGED) {
    // Effacer l'ancien curseur seulement s'il était visible
    if (cursor.prevShouldShow == CURSOR_VISIBLE) {
      eraseCursor(cursor.last);
    }
    
    // Dessiner le nouveau curseur s'il doit être visible
    if (cursor.shouldShow == CURSOR_VISIBLE) {
      drawCursor(cursor.displayed);
    }
    
    // Mettre à jour les variables d'état
    cursor.last = cursor.displayed;
    cursor.prevShouldShow = cursor.shouldShow;
  }// Mise à jour des blocs - seulement effacer et redessiner les pixels modifiés
  if (anyBlockMoved == FLAG_CHANGED) {
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active == BLOCK_ACTIVE) {
        if (blocks[i].oldX != blocks[i].x) {
          // Effacer l'ancienne queue du bloc (pixel précédent)
          eraseBlockTail(blocks[i]);
          
          // Dessiner la nouvelle tête du bloc
          drawBlockHead(blocks[i]);
        }
        
        // Vérifier spécifiquement si le bloc est en train de sortir de l'écran
        if (blocks[i].x < 0 && blocks[i].x + blocks[i].length > 0) {
          // Le bloc est partiellement visible, s'assurer que seules les colonnes
          // visibles sont affichées
          
          // Pour chaque colonne visible, l'afficher
          for (int16_t j = 0; j < blocks[i].length; j++) {
            int16_t colX = blocks[i].x + j;
            
            // N'afficher que les colonnes qui sont visibles à l'écran
            if (colX >= 0 && colX < MATRIX_WIDTH) {
              // Cette colonne est visible, l'afficher
              if (blocks[i].y < MATRIX_HEIGHT) {
                ht1632_plot(colX, blocks[i].y, blocks[i].color);
              }
              if (blocks[i].y + 1 < MATRIX_HEIGHT) {
                ht1632_plot(colX, blocks[i].y + 1, blocks[i].color);
              }
            }
            // Les colonnes qui sortent de l'écran (colX < 0) ne sont pas dessinées
            // mais le bloc continue à avancer jusqu'à ce que blocks[i].x + blocks[i].length <= 0
          }
        }
      }
    }
  }
  
  // Gestion audio des blocs
  updateAudio();
}

