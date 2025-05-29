#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"

//je suius michel
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

// Structure pour stocker les informations d'un bloc 
typedef struct {
  int16_t x;              // Position horizontale, changé en int16_t pour éviter les dépassements
  uint8_t y;              // Position verticale (0 à 255), changé de int8_t à uint8_t
  uint8_t length;         // Longueur du bloc (0 à 255)
  uint8_t color : 3;      // Couleur du bloc sur 2 bits (0-3)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint16_t frequency;     // Fréquence de la note associée
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
unsigned long lastNoteTime = 0;
bool lastNoteStillActive = false;  // Pour vérifier si la dernière note est encore active

// Variables de timing - RALENTISSEMENT SIGNIFICATIF
unsigned long lastMoveTime = 0;
uint16_t currentNoteDelay = 0;


// Variables globales pour le curseur
volatile int potValue = 0;
volatile uint8_t cursorY = 0;
uint8_t cursorY_displayed = 0;


bool cursorBlinking = false;
bool cursorVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 200; // ms


// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Pour suivre si la note du bloc est en cours de lecture
bool blockNotePlaying[MAX_BLOCKS] = {0};

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
    blocks[blockIndex].frequency = note.frequency; // <--- AJOUT
    blockNotePlaying[blockIndex] = false; // <--- AJOUT
    
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
  currentNoteDelay = beatDuration * 4 / note.duration ;  
  
  lastNoteTime = millis();
}

// Fonction pour dessiner un bloc sur la matrice
// Modifié : sur colonnes 2/3, le bloc passe devant la colonne verte sauf si il est sous le curseur (alors il passe derrière)
void drawBlock(Block block) {
  if (!block.active) return;

  int16_t xStart = block.x;
  int16_t xEnd = xStart + block.length;
  uint8_t yPos = block.y;

  if (xStart < 0) xStart = 0;
  if (xEnd > MATRIX_WIDTH) xEnd = xEnd;

  for (int16_t x = xStart; x < xEnd; x++) {
    if (x == 2 || x == 3) {
      // Si le bloc est à la hauteur du curseur, il passe derrière (ne dessine rien ici)
      if (yPos == cursorY_displayed || yPos == cursorY_displayed + 1) {
        // ne rien dessiner ici, la colonne verte reste devant
      } else {
        // sinon, le bloc passe devant la colonne verte
        if (yPos < MATRIX_HEIGHT) ht1632_plot(x, yPos, block.color);
        if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(x, yPos + 1, block.color);
      }
    } else if (x >= 0 && x < MATRIX_WIDTH) {
      if (yPos < MATRIX_HEIGHT) ht1632_plot(x, yPos, block.color);
      if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(x, yPos + 1, block.color);
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
  for (uint8_t dx = 2; dx <= 3; dx++) {
    for (uint8_t dy = 0; dy < 2; dy++) {
      if (y + dy < MATRIX_HEIGHT) {
        ht1632_plot(dx, y + dy, 1); // remet la colonne verte uniquement sous le curseur
      }
    }
  }
}

// Affiche le curseur 2x2 rouge à une position donnée
void drawCursor(uint8_t y) {
  for (uint8_t dx = 2; dx <= 3; dx++) {
    for (uint8_t dy = 0; dy < 2; dy++) {
      if (y + dy < MATRIX_HEIGHT) {
        ht1632_plot(dx, y + dy, 2);
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

// Efface uniquement la colonne et lignes concernées par la queue d'un bloc
void eraseBlockTail(const Block& block) {
  int16_t tailX = block.x + block.length; // colonne qui disparait
  if (tailX >= 0 && tailX < MATRIX_WIDTH) {
    if (block.y < MATRIX_HEIGHT) ht1632_plot(tailX, block.y, 0);
    if (block.y + 1 < MATRIX_HEIGHT) ht1632_plot(tailX, block.y + 1, 0);
  }
}

// Affiche uniquement la tête du bloc (nouvelle colonne)
// Même logique que drawBlock pour la gestion du curseur et des colonnes vertes
void drawBlockHead(const Block& block) {
  int16_t headX = block.x;
  uint8_t yPos = block.y;
  if (headX == 2 || headX == 3) {
    if (yPos == cursorY_displayed || yPos == cursorY_displayed + 1) {
      // ne rien dessiner ici, la colonne verte reste devant
    } else {
      if (yPos < MATRIX_HEIGHT) ht1632_plot(headX, yPos, block.color);
      if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(headX, yPos + 1, block.color);
    }
  } else if (headX >= 0 && headX < MATRIX_WIDTH) {
    if (yPos < MATRIX_HEIGHT) ht1632_plot(headX, yPos, block.color);
    if (yPos + 1 < MATRIX_HEIGHT) ht1632_plot(headX, yPos + 1, block.color);
  }
}

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
    }

    // Déplacement des blocs
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        eraseBlockTail(blocks[i]);
        blocks[i].x--;
        drawBlockHead(blocks[i]);
        if (blocks[i].x + blocks[i].length < 0) {
          blocks[i].active = 0;
          blockNotePlaying[i] = false;
        }
      }
    }
    lastMoveTime = currentTime;
  }
}

// Fonction périodique pour gérer la création des notes/blocs et la musique
void periodicNotesAndMusic() {
  static unsigned long lastNoteCreationTime = 0;
  static bool noteCreationInProgress = false;
  unsigned long currentTime = millis();

  // Création des notes/blocs
  if (!songFinished && currentTime - lastNoteTime >= currentNoteDelay && !noteCreationInProgress) {
    if (currentTime - lastNoteCreationTime >= 1000) {
      lastNoteCreationTime = currentTime;
      noteCreationInProgress = true;
      nextNote();
      noteCreationInProgress = false;
    }
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
  }
  
  // Initialiser lastMoveTime pour garantir un déplacement immédiat
  lastMoveTime = 0;  // Forcer le premier déplacement rapidement
  
  // Créer le premier bloc
  nextNote();
  
#if MUSIQUE
  pinMode(BUZZER_PIN, OUTPUT);
#endif
  
  pinMode(buttonPin, INPUT_PULLUP); // bouton avec résistance de pullup
}

void loop() {
  // Gestion utilisateur (curseur)
  static unsigned long lastPotRead = 0;
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(buttonPin);

  // Lecture bouton pour activer/désactiver le clignotement
  if (buttonState != lastButtonState) {
    delay(5); // anti-rebond simple
    if (buttonState == LOW) {
      cursorBlinking = true;
    } else {
      cursorBlinking = false;
    }
    lastButtonState = buttonState;
  }

  // Ne lire le potentiomètre et ne mettre à jour le curseur QUE si le bouton n'est pas appuyé
  if (millis() - lastPotRead > 5 && buttonState == HIGH) {
    updateCursorFromPot();
    lastPotRead = millis();
  }

  periodicMoveCursor();

  // Gestion périodique des blocs et de la musique
  periodicMoveBlocks();
  periodicNotesAndMusic();

  // Affichage statique (colonnes vertes) sauf sous le curseur et sauf sous un bloc
  drawStaticColumnsExceptCursorAndBlocks();

  // Ne pas appeler drawCursor ici, il est géré dans periodicMoveCursor()
  // Affichage des têtes de blocs actifs (pour le cas où ils sont nouveaux)
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active) {
      drawBlockHead(blocks[i]);
    }
  }

  // Gestion de la fin de séquence musicale
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
      // Réinitialise la musique si besoin
    }
  }

  delay(5); // boucle rapide
}