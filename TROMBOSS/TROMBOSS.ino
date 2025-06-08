#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"
#include "TimerOne.h"
#include "definitions.h"

// je suis michel

// Compteur pour la fonction périodique
volatile uint16_t periodicCounter = 0;

// Déclaration des variables globales définies dans definitions.h
Cursor cursor;
Block blocks[MAX_BLOCKS];
volatile bool displayNeedsUpdate = false;
volatile bool shouldShowCursor = true;

// Variables pour le contrôle des blocs et la déduplication
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;
uint8_t lastNoteFrequency = 0;
bool lastNoteStillActive = false;
bool blockNotePlaying[MAX_BLOCKS] = {0};
uint8_t lastNotePosition = 255;

// Variables pour le système de niveaux
uint8_t currentDifficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
uint8_t blockMoveCycles = BLOCK_MOVE_CYCLES_LEVEL_6;

// Variable principale de l'état du jeu
GameState gameState;

// Variable de score
Score gameScore;

// Fonction pour obtenir le nombre de cycles selon le niveau de difficulté
uint8_t getDifficultyBlockMoveCycles(uint8_t level) {
  switch (level) {
    case 1: return BLOCK_MOVE_CYCLES_LEVEL_1;
    case 2: return BLOCK_MOVE_CYCLES_LEVEL_2;
    case 3: return BLOCK_MOVE_CYCLES_LEVEL_3;
    case 4: return BLOCK_MOVE_CYCLES_LEVEL_4;
    case 5: return BLOCK_MOVE_CYCLES_LEVEL_5;
    case 6: return BLOCK_MOVE_CYCLES_LEVEL_6;
    case 7: return BLOCK_MOVE_CYCLES_LEVEL_7;
    case 8: return BLOCK_MOVE_CYCLES_LEVEL_8;
    case 9: return BLOCK_MOVE_CYCLES_LEVEL_9;
    default: return BLOCK_MOVE_CYCLES_LEVEL_1; // Par défaut niveau 1
  }
}

// Fonction pour définir le niveau de difficulté
void setDifficultyLevel(uint8_t level) {
  if (level >= MIN_DIFFICULTY_LEVEL && level <= MAX_DIFFICULTY_LEVEL) {
    currentDifficultyLevel = level;
    blockMoveCycles = getDifficultyBlockMoveCycles(level);
    
#if DEBUG_SERIAL
    Serial.print("Lvl:");
    Serial.print(level);
    Serial.print(" Cyc:");
    Serial.println(blockMoveCycles);
#endif
  }
}

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
  Serial.print("F:");
  Serial.print(frequency);
  Serial.print(" Y:");
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
    Serial.println("Max blocs");
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
    Serial.println("Pas libre");
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
    Serial.println("Note active");
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
    Serial.println("Conflit Y");
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
    Serial.println("Col occupée");
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
    blocks[blockIndex].active = 1;    blocks[blockIndex].frequency = note.frequency;
    blocks[blockIndex].needsUpdate = 0; // Initialement, pas besoin de mise à jour
    blocks[blockIndex].hitPixels = 0;   // Aucun pixel touché initialement
    blockNotePlaying[blockIndex] = false;
      #if DEBUG_SERIAL //suivi des blocs créés
    Serial.print("B x=");
    Serial.print(startX);
    Serial.print(" y=");
    Serial.print(posY);
    Serial.print(" l=");
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
  bool onCursorPosition = (yPos == cursor.yDisplayed || yPos == cursor.yDisplayed + 1);
  
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

// Fonction pour passer à la prochaine note de la chanson
void nextNote() {
  const MusicNote* currentSong;
  uint8_t currentSongSize;
    // Vérification pour éviter la création multiple de la même note
  if (songPosition == lastNotePosition && currentSongPart == currentSongPart) {
#if DEBUG_SERIAL
    Serial.println("Note dupliquée");
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
  cursor.potValue = analogRead(POT_PIN);
  int y = map(cursor.potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
  if (y < 0) y = 0;
  if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
  cursor.y = (uint8_t)y;
}

// Efface le curseur 2x2 à une position donnée et restaure la colonne verte uniquement sur cette zone
void eraseCursor(uint8_t y) {
  // Mémoriser l'état des pixels avant effacement pour meilleure restauration
  static uint8_t previousPixelState[2][2] = {{1, 1}, {1, 1}}; // Par défaut, colonne verte
  
  for (uint8_t dx = 0; dx < CURSOR_WIDTH; dx++) {
    for (uint8_t dy = 0; dy < CURSOR_HEIGHT; dy++) {
      uint8_t x = dx + CURSOR_COLUMN_START; // colonnes 2 et 3
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
          ht1632_plot(x, yPos, GREEN_COLUMN_COLOR); // Colonne verte
          previousPixelState[dx][dy] = GREEN_COLUMN_COLOR;
        }
      }
    }
  }
}

// Affiche le curseur 2x2 rouge à une position donnée
void drawCursor(uint8_t y) {
  for (uint8_t dx = 0; dx < CURSOR_WIDTH; dx++) {
    for (uint8_t dy = 0; dy < CURSOR_HEIGHT; dy++) {
      uint8_t x = dx + CURSOR_COLUMN_START; // colonnes 2 et 3
      uint8_t yPos = y + dy;
      
      if (yPos < MATRIX_HEIGHT) {
        ht1632_plot(x, yPos, CURSOR_COLOR); // Couleur définie par la constante
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
  if (cursor.state == CURSOR_STATE_BLINKING) {
    if (now - cursor.lastBlinkTime > CURSOR_BLINK_INTERVAL) {
      cursor.visible = (cursor.visible == CURSOR_VISIBLE) ? CURSOR_HIDDEN : CURSOR_VISIBLE;
      if (cursor.visible == CURSOR_VISIBLE) {
        drawCursor(cursor.yDisplayed);
      } else {
        eraseCursor(cursor.yDisplayed);
      }
      cursor.lastBlinkTime = now;
    }
  } else {
    // Si pas de clignotement, s'assurer que le curseur est visible
    if (cursor.visible != CURSOR_VISIBLE) {
      drawCursor(cursor.yDisplayed);
      cursor.visible = CURSOR_VISIBLE;
    }
  }

  // Déplacement du curseur
  if (now - lastCursorMove > cursorDelay) {
    if (cursor.yDisplayed < cursor.y) {
      eraseCursor(cursor.yDisplayed);
      cursor.yDisplayed++;
      if (cursor.visible == CURSOR_VISIBLE) drawCursor(cursor.yDisplayed);
    } else if (cursor.yDisplayed > cursor.y) {
      eraseCursor(cursor.yDisplayed);
      cursor.yDisplayed--;
      if (cursor.visible == CURSOR_VISIBLE) drawCursor(cursor.yDisplayed);
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
      bool notOnCursor = (y < cursor.yDisplayed || y >= cursor.yDisplayed + 2);
      
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

// Fonction supprimée - logique déplacée dans periodicFunction()

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
    }    // Ne pas dessiner sur la zone du curseur affiché ni sur la zone d'un bloc
    if (!blocPresent && (y < cursor.yDisplayed || y >= cursor.yDisplayed + 2)) {
      ht1632_plot(2, y, 1);
      ht1632_plot(3, y, 1);
    }
  }
}

// Restaure uniquement les colonnes vertes aux positions spécifiques sans toucher au reste
void restoreGreenColumn(uint8_t x, uint8_t y) {
  if ((x == 2 || x == 3) && 
      (y < cursor.yDisplayed || y >= cursor.yDisplayed + 2)) {
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

// ===== FONCTIONS DE GESTION DES ÉTATS DU JEU =====

// Initialisation de l'état du jeu
void initGameState() {
  gameState.etat = GAME_STATE_MENU;
  gameState.level = 1;
  gameState.timeStart = millis();
  gameState.timeElapsed = 0;
  gameState.gameOver = false;
  gameState.pauseGame = false;
  
  // Initialiser le score
  initScore();
  
  // Réinitialiser les variables de musique
  songPosition = 0;
  currentSongPart = 0;
  songFinished = 0;
    // Désactiver tous les blocs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    blocks[i].active = 0;
    blocks[i].hitPixels = 0;  // Réinitialiser les pixels touchés
    blockNotePlaying[i] = false;
  }
  
#if DEBUG_SERIAL
  Serial.println("État init");
#endif
}

// Gestion du menu principal
void handleMenuState() {
  static bool menuInitialized = false;
  
  if (!menuInitialized) {
    ht1632_clear();
    // Affichage du menu (simplifié pour l'instant)
    // TODO: Implémenter l'interface de sélection de niveau
    
#if DEBUG_SERIAL
    Serial.println("=== MENU ===");
    Serial.println("Bouton pour start");
#endif
    menuInitialized = true;
  }
  
  // Lecture du bouton pour démarrer le jeu
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Bouton pressé, démarrer le jeu
    changeGameState(GAME_STATE_LEVEL);
    menuInitialized = false;
  }
  lastButtonState = buttonState;
}

// Gestion de l'état de jeu (niveau)
void handleLevelState() {
  static bool levelInitialized = false;
    if (!levelInitialized) {    // Initialiser le niveau
    setDifficultyLevel(gameState.level);
    gameState.timeStart = millis();
    
    // Réinitialiser le score pour le nouveau niveau
    initScore();
    
    // Réinitialiser les variables de jeu
    songPosition = 0;
    currentSongPart = 0;
    songFinished = 0;
      // Effacer les blocs existants
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      blocks[i].active = 0;
      blocks[i].hitPixels = 0;  // Réinitialiser les pixels touchés
    }
    
    // Affichage initial
    ht1632_clear();
    drawStaticColumnsExceptCursorAndBlocks();
    drawCursor(cursor.yDisplayed);
    
#if DEBUG_SERIAL
    Serial.print("=== NIV ");
    Serial.print(gameState.level);
    Serial.println(" START ===");
#endif
    levelInitialized = true;
  }
  
  // Mettre à jour le temps écoulé
  gameState.timeElapsed = millis() - gameState.timeStart;
  
  // Logique de jeu existante (sera exécutée dans la fonction périodique)
    // Appeler la logique de jeu principale
  handleLevelLoop();
  
  // Conditions de fin de niveau
  if (songFinished) {
    bool allBlocksInactive = true;
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        allBlocksInactive = false;
        break;
      }
    }
      if (allBlocksInactive) {
      // Niveau terminé - évaluer le score transformé
      if (gameScore.transformed >= 80) {
        changeGameState(GAME_STATE_WIN);
      } else {
        changeGameState(GAME_STATE_LOSE);
      }
      levelInitialized = false;    }
  }
  
  // Note: Les conditions de défaite seront définies plus tard selon les besoins du jeu
}

// Gestion de l'état de victoire
void handleWinState() {
  static bool winInitialized = false;
  static uint32_t winDisplayTime = 0;
  
  if (!winInitialized) {
    ht1632_clear();
    // TODO: Affichage de victoire
    
#if DEBUG_SERIAL
    Serial.println("=== VICTOIRE ===");
    Serial.print("Score: ");
    Serial.print(gameScore.current);
    Serial.print("/");
    Serial.print(gameScore.maxPossible);
    Serial.print(" (");
    Serial.print(gameScore.transformed);
    Serial.println("%)");
    Serial.print("T: ");
    Serial.print(gameState.timeElapsed / 1000);
    Serial.println(" sec");
#endif
    
    winDisplayTime = millis();
    winInitialized = true;
  }
  
  // Retourner au menu après 3 secondes
  if (millis() - winDisplayTime > 3000) {
    if (gameState.level < 9) {
      // Passer au niveau suivant
      gameState.level++;
      changeGameState(GAME_STATE_LEVEL);
    } else {
      // Retourner au menu (jeu terminé)
      changeGameState(GAME_STATE_MENU);
    }
    winInitialized = false;
  }
}

// Gestion de l'état de défaite
void handleLoseState() {
  static bool loseInitialized = false;
  static uint32_t loseDisplayTime = 0;
  
  if (!loseInitialized) {
    ht1632_clear();
    // TODO: Affichage de défaite
    
#if DEBUG_SERIAL
    Serial.println("=== DÉFAITE ===");
    Serial.print("Score fin: ");
    Serial.print(gameScore.current);
    Serial.print("/");
    Serial.print(gameScore.maxPossible);
    Serial.print(" (");
    Serial.print(gameScore.transformed);
    Serial.println("%)");
#endif
    
    loseDisplayTime = millis();
    loseInitialized = true;
  }
  
  // Retourner au menu après 3 secondes
  if (millis() - loseDisplayTime > 3000) {
    changeGameState(GAME_STATE_MENU);
    loseInitialized = false;
  }
}

// Fonction pour changer l'état du jeu
void changeGameState(uint8_t newState) {
  if (newState >= GAME_STATE_MENU && newState <= GAME_STATE_LOSE) {
    gameState.etat = newState;
    
#if DEBUG_SERIAL
    Serial.print("État: ");
    switch (newState) {
      case GAME_STATE_MENU: Serial.println("MENU"); break;
      case GAME_STATE_LEVEL: Serial.println("NIV"); break;
      case GAME_STATE_WIN: Serial.println("WIN"); break;
      case GAME_STATE_LOSE: Serial.println("LOSE"); break;
    }
#endif
  }
}

// ===== FONCTIONS DE GESTION DU SCORE =====

// Initialisation du score
void initScore() {
  gameScore.current = 0;
  gameScore.maxPossible = 0;
  gameScore.transformed = 0;
  
#if DEBUG_SERIAL
  Serial.println("Score init");
#endif
}

// Ajouter des points au score actuel
void addScore(uint16_t points) {
  gameScore.current += points;
  updateTransformedScore();
  
#if DEBUG_SERIAL
  Serial.print("Score +");
  Serial.print(points);
  Serial.print(" = ");
  Serial.print(gameScore.current);
  Serial.print("/");
  Serial.print(gameScore.maxPossible);
  Serial.print(" (");
  Serial.print(gameScore.transformed);
  Serial.println("%)");
#endif
}

// Ajouter des points au score maximum possible
void addMaxScore(uint16_t points) {
  gameScore.maxPossible += points;
  updateTransformedScore();
  
#if DEBUG_SERIAL
  Serial.print("Smax +");
  Serial.print(points);
  Serial.print(" = ");
  Serial.print(gameScore.maxPossible);
  Serial.print(" (tr: ");
  Serial.print(gameScore.transformed);
  Serial.println("%)");
#endif
}

// Calculer et mettre à jour le score transformé
void updateTransformedScore() {
  if (gameScore.maxPossible > 0) {
    // Calcul du pourcentage arrondi vers le bas
    uint32_t percentage = ((uint32_t)gameScore.current * 100) / gameScore.maxPossible;
    gameScore.transformed = (uint8_t)percentage;
  } else {
    gameScore.transformed = 0;
  }
}

// ===== FONCTIONS DE DÉTECTION DE COLLISION =====

// Calculer quels pixels du bloc sont touchés par le curseur
uint8_t getBlockPixelsHitByCursor(uint8_t blockIndex) {
  if (!blocks[blockIndex].active) {
    return 0;
  }
  
  Block& block = blocks[blockIndex];
  uint8_t pixelsHit = 0;
  
  // Position du curseur (2x2 pixels sur colonnes 2-3)
  uint8_t cursorX1 = 2;
  uint8_t cursorX2 = 3;
  uint8_t cursorY1 = cursor.yDisplayed;
  uint8_t cursorY2 = cursor.yDisplayed + 1;
  
  // Vérifier si le bloc intersecte avec la zone du curseur
  int16_t blockX1 = block.x;
  int16_t blockX2 = block.x + block.length - 1;
  uint8_t blockY1 = block.y;
  uint8_t blockY2 = block.y + 1; // BLOCK_HEIGHT = 2
  
  // Vérifier l'intersection en X (bloc doit toucher colonnes 2 ou 3)
  bool xIntersect = (blockX1 <= cursorX2 && blockX2 >= cursorX1);
  
  // Vérifier l'intersection en Y
  bool yIntersect = (blockY1 <= cursorY2 && blockY2 >= cursorY1);
  
  if (!xIntersect || !yIntersect) {
    return 0; // Pas d'intersection
  }
  
  // Calculer les pixels individuels touchés
  // Chaque bloc fait 2 pixels de haut et "length" pixels de large
  // On encode les pixels touchés dans un masque de bits
  for (uint8_t pixelY = 0; pixelY < BLOCK_HEIGHT; pixelY++) {
    uint8_t absoluteY = block.y + pixelY;
    
    // Ce pixel Y intersecte-t-il avec le curseur ?
    if (absoluteY >= cursorY1 && absoluteY <= cursorY2) {
      // Vérifier chaque pixel X du bloc
      for (uint8_t pixelX = 0; pixelX < block.length && pixelX < 16; pixelX++) { // Limite à 16 pour le masque
        int16_t absoluteX = block.x + pixelX;
        
        // Ce pixel X intersecte-t-il avec le curseur (colonnes 2-3) ?
        if (absoluteX >= cursorX1 && absoluteX <= cursorX2) {
          // Calculer l'index du pixel dans le masque (row-major order)
          uint8_t pixelIndex = pixelY * block.length + pixelX;
          
          // Vérifier que l'index ne dépasse pas 15 (masque 16 bits)
          if (pixelIndex < 16) {
            pixelsHit |= (1 << pixelIndex);
          }
        }
      }
    }
  }
  
#if DEBUG_SERIAL
  if (pixelsHit > 0) {
    Serial.print("Bloc ");
    Serial.print(blockIndex);
    Serial.print(" - Pix: ");
    Serial.print(pixelsHit, BIN);
    Serial.print(" (");
    Serial.print(__builtin_popcount(pixelsHit));
    Serial.println(" pixels)");
  }
#endif
  
  return pixelsHit;
}

// Vérifier si le curseur touche un bloc et marquer les points
void checkCursorCollision() {
  // Vérifier seulement si le curseur clignote (bouton pressé)
  if (cursor.state != CURSOR_STATE_BLINKING) {
    return;
  }
  
  // Parcourir tous les blocs actifs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (!blocks[i].active) {
      continue;
    }
    
    // Calculer quels pixels du bloc sont touchés par le curseur
    uint8_t pixelsHit = getBlockPixelsHitByCursor(i);
    
    if (pixelsHit == 0) {
      continue; // Pas de collision pour ce bloc
    }
    
    // Calculer quels sont les nouveaux pixels (pas encore touchés)
    uint8_t newPixelsHit = pixelsHit & (~blocks[i].hitPixels);
    
    if (newPixelsHit == 0) {
      continue; // Tous ces pixels ont déjà été comptés
    }
    
    // Compter le nombre de nouveaux pixels touchés
    uint8_t newPixelCount = __builtin_popcount(newPixelsHit);
    
    // Marquer ces pixels comme touchés
    blocks[i].hitPixels |= newPixelsHit;
    
    // Ajouter les points au score (1 point par pixel)
    addScore(newPixelCount);
    
#if DEBUG_SERIAL
    Serial.print("COL! Bloc ");
    Serial.print(i);
    Serial.print(" - Nouv: ");
    Serial.print(newPixelCount);
    Serial.print(" - Sc: +");
    Serial.println(newPixelCount);
#endif
  }
}

void setup() {
  // Réactiver Serial pour le débogage
#if DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("=== TROMBOSS ===");
#endif
  
  // Initialisation de la matrice LED
  Wire.begin();
  ht1632_setup();
  setup7Seg();
  ht1632_clear();
    // Initialiser l'état du jeu
  initGameState();
  
  // Configuration du bouton en entrée avec pull-up interne
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialiser le curseur
  cursor.y = 0;
  cursor.yDisplayed = 0;
  cursor.yLast = 0;
  cursor.state = CURSOR_STATE_NORMAL;
  cursor.visible = CURSOR_VISIBLE;
  cursor.color = CURSOR_COLOR;
  cursor.potValue = 0;
  cursor.lastBlinkTime = 0;
    // Initialiser les blocs
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    blocks[i].active = 0;
    blocks[i].color = BLOCK_COLOR;
    blocks[i].needsUpdate = 0;
    blocks[i].hitPixels = 0;  // Aucun pixel touché initialement
    blockNotePlaying[i] = false;
  }
  
  // Initialiser les flags d'affichage
  displayNeedsUpdate = true;
  shouldShowCursor = true;

#if MUSIQUE
  pinMode(BUZZER_PIN, OUTPUT);
#endif
  
  // Initialiser le Timer1 pour appeler periodicFunction toutes les 25ms
  Timer1.initialize(TIMER_PERIOD);
  Timer1.attachInterrupt(periodicFunction);
  
  // Réinitialiser le compteur périodique
  periodicCounter = 0;
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
    bool buttonState = digitalRead(BUTTON_PIN);
    
    // Gestion du bouton avec anti-rebond logiciel
    if (buttonState != lastButtonState) {
      if (buttonState == LOW) {
        cursor.state = CURSOR_STATE_BLINKING;
      } else {
        cursor.state = CURSOR_STATE_NORMAL;
        shouldShowCursor = true; // Toujours visible quand on relâche le bouton
      }
      lastButtonState = buttonState;
      displayNeedsUpdate = true;
    }
  }
    // Lecture du potentiomètre (10 fois par seconde = tous les 4 cycles)
  if (periodicCounter % 4 == 0) {
    if (digitalRead(BUTTON_PIN) == HIGH) {
      // Lire la valeur actuelle du potentiomètre
      int newPotValue = analogRead(POT_PIN);
      
      // Ne mettre à jour que si la valeur a suffisamment changé (évite les micro-variations)
      if (abs(newPotValue - cursor.potValue) > 5) {
        cursor.potValue = newPotValue;
        int y = map(cursor.potValue, 0, 1024, 0, MATRIX_HEIGHT - 1);
        if (y < 0) y = 0;
        if (y > MATRIX_HEIGHT - 2) y = MATRIX_HEIGHT - 2;
        
        // N'actualiser que si la position a réellement changé
        if (cursor.y != (uint8_t)y) {
          cursor.y = (uint8_t)y;
          displayNeedsUpdate = true;
        }
      }
    }
  }    // Gestion du clignotement du curseur (5 fois par seconde = tous les 8 cycles)
  if (periodicCounter % 8 == 0) {
    if (cursor.state == CURSOR_STATE_BLINKING) {
      shouldShowCursor = !shouldShowCursor; // Inverser l'état d'affichage du curseur
      displayNeedsUpdate = true;
      
      // Vérifier les collisions pendant le clignotement (seulement dans l'état LEVEL)
      if (gameState.etat == GAME_STATE_LEVEL) {
        checkCursorCollision();
      }
    } else if (!shouldShowCursor) {
      shouldShowCursor = true; // S'assurer que le curseur est visible si pas en mode clignotement
      displayNeedsUpdate = true;
    }
  }
  
  // Déplacement du curseur (tous les cycles, mais progressif)
  if (cursor.yDisplayed < cursor.y) {
    cursor.yDisplayed++;
    displayNeedsUpdate = true;
  } else if (cursor.yDisplayed > cursor.y) {
    cursor.yDisplayed--;
    displayNeedsUpdate = true;
  }  // Déplacement des blocs - fréquence selon le niveau de difficulté
  // Seulement si on est dans l'état LEVEL
  if (gameState.etat == GAME_STATE_LEVEL && periodicCounter % blockMoveCycles == 0) {
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
      // Effectuer le déplacement des blocs (Phase 1 - calculs uniquement)
    for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
      if (blocks[i].active) {
        // Sauvegarder l'ancienne position avant de mettre à jour
        blocks[i].oldX = blocks[i].x;
        
        // Déplacer le bloc
        blocks[i].x--;        // Nouvelle logique : ajouter 2 pixels au score max pour chaque colonne qui passe x=3
        // (c'est-à-dire quand chaque colonne passe de x=4 à x=3)
        int16_t oldEnd = blocks[i].oldX + blocks[i].length - 1;  // Dernière colonne à l'ancienne position
        int16_t newEnd = blocks[i].x + blocks[i].length - 1;     // Dernière colonne à la nouvelle position
        
        // Pour chaque colonne du bloc, vérifier si elle vient de passer x=3
        for (int16_t col = blocks[i].oldX; col <= oldEnd; col++) {
          // Cette colonne était-elle à x=4 et est maintenant à x=3 ?
          int16_t newCol = col - 1; // Position après déplacement
          if (col == 4 && newCol == 3) {
            // Cette colonne vient de passer la deuxième colonne verte (x=3)
            // Ajouter 2 pixels (hauteur du bloc = 2) au score maximum
            addMaxScore(2);
#if DEBUG_SERIAL
            Serial.print("Col x=3 bloc ");
            Serial.print(i);
            Serial.println(" - Smax +2");
#endif
          }
        }
        
        // Vérifier si le bloc est complètement sorti de l'écran
        if (blocks[i].x + blocks[i].length < -1) {
          // Le bloc est complètement sorti de l'écran, le désactiver
          blocks[i].active = 0;
          blockNotePlaying[i] = false;
        }
        
        blocks[i].needsUpdate = 1; // Marquer le bloc pour affichage
        displayNeedsUpdate = true; // Indiquer que l'affichage doit être mis à jour
      }
    }
  }
    // Création de nouvelles notes (1 fois par seconde = tous les 40 cycles)
  // Seulement si on est dans l'état LEVEL
  if (gameState.etat == GAME_STATE_LEVEL && periodicCounter % 40 == 0) {
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

void handleLevelLoop() {
  // Variables pour détecter les changements
  static uint32_t lastAudioUpdate = 0;
  static bool prevShouldShowCursor = shouldShowCursor;
  uint32_t currentTime = periodicCounter * TIMER_PERIOD / 1000; // Temps basé sur le compteur sans utiliser millis()
  
  bool cursorStateChanged = (shouldShowCursor != prevShouldShowCursor);
  bool cursorPositionChanged = (cursor.yDisplayed != cursor.yLast);
  
  // Vérifier si des blocs ont été mis à jour
  bool anyBlockMoved = false;
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].needsUpdate) {
      anyBlockMoved = true;
      break;
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
      eraseCursor(cursor.yLast);
    }
    
    // Dessiner le nouveau curseur s'il doit être visible
    if (shouldShowCursor) {
      drawCursor(cursor.yDisplayed);
    }
    
    // Mettre à jour les variables d'état
    cursor.yLast = cursor.yDisplayed;
    prevShouldShowCursor = shouldShowCursor;
  }
  
  // Mise à jour des blocs - seulement effacer et redessiner les pixels modifiés
  for (uint8_t i = 0; i < MAX_BLOCKS; i++) {
    if (blocks[i].active && blocks[i].needsUpdate) {
      // Effacer l'ancienne queue du bloc (pixel précédent)
      eraseBlockTail(blocks[i]);
      
      // Dessiner la nouvelle tête du bloc
      drawBlockHead(blocks[i]);
      
      // Réinitialiser le flag de mise à jour
      blocks[i].needsUpdate = 0;
    }
    
    if (blocks[i].active) {
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
  
  // Gestion audio des blocs
  updateAudio();
}

void loop() {
  // Machine à états pour gérer les différents états du jeu
  switch (gameState.etat) {
    case GAME_STATE_MENU:
      handleMenuState();
      break;
      
    case GAME_STATE_LEVEL:
      handleLevelState();
      break;
      
    case GAME_STATE_WIN:
      handleWinState();
      break;
      
    case GAME_STATE_LOSE:
      handleLoseState();
      break;
      
    default:
      // État invalide, retourner au menu
      Serial.println("État invalide");
      changeGameState(GAME_STATE_MENU);
      break;
  }
}

