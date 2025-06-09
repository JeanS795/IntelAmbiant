#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"
#include "TimerOne.h"
#include "definitions.h"
// je suis michel

// CORRECTION CRITIQUE: Déclaration de la variable persistante pour le niveau
extern uint8_t persistentSelectedLevel;

// Variable globale pour signaler la réinitialisation du bouton après changement d'état
bool needButtonReset = false;

// Variable globale pour forcer la réinitialisation du menu
bool forceMenuReinit = false;

//======== SETUP ========
void setup() {
  // Réactiver Serial pour le débogage
#if DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("=== TROMBOSS ===");
#endif
  
  // Initialisation de la matrice LED
  Wire.begin();
  ht1632_setup();
  setup7Seg();  ht1632_clear();
    // Initialiser l'état du jeu
  initGameState();
  
  // Initialiser l'état du menu
  initMenuState();
  
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

//======== LOOP PRINCIPAL ========
void loop() {
  // Mise à jour de l'affichage 7 segments - optimisée pour réduire les blocages I2C
  static unsigned long last7SegUpdate = 0;
  unsigned long currentTime = millis();

  // Réduire la fréquence des vérifications pour les mises à jour 7seg pendant le jeu
  unsigned long updateInterval = (gameState.etat == GAME_STATE_LEVEL) ? 200 : 500; // 200ms en jeu, 500ms ailleurs
  
  if (currentTime - last7SegUpdate >= updateInterval) {
    update7SegDisplay(gameState.etat, gameScore.transformed, gameState.level);
    last7SegUpdate = currentTime;
  }
  
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

//======== FONCTION PÉRIODIQUE ========
void periodicFunction() {
  // Incrémenter le compteur périodique
  periodicCounter++;

  // Note: L'affichage 7 segments est maintenant géré dans loop() pour éviter le blocage I2C dans l'interruption

  // ===== CONTRÔLES COMMUNS À TOUS LES ÉTATS =====
    // Lecture du bouton (4 fois par seconde = tous les 10 cycles)
  if (periodicCounter % 10 == 0) {
    static bool lastButtonState = HIGH;
    bool buttonState = digitalRead(BUTTON_PIN);
    
    // CORRECTION : Gérer la réinitialisation du bouton après changement d'état
    if (needButtonReset) {
      lastButtonState = buttonState; // Synchroniser avec l'état actuel
      needButtonReset = false;
#if DEBUG_SERIAL
      Serial.println("Btn reset");
#endif
      return; // Ignorer ce cycle pour éviter la détection de changement
    }
    
    // Gestion du bouton avec anti-rebond logiciel
    if (buttonState != lastButtonState) {if (buttonState == LOW) {
        // Bouton pressé
        if (gameState.etat == GAME_STATE_MENU) {
          // Dans le menu, démarrer le mode validation avec clignotement
          if (!menuState.validationMode) {
            menuState.validationMode = true;
            menuState.validationStart = millis();
            menuState.lastBlinkTime = millis();
            menuState.boxVisible = false; // Commencer par invisible pour créer l'effet
            
#if DEBUG_SERIAL
            Serial.print("Val:");
            Serial.println(menuState.selectedLevel);
#endif
          }
        } else if (gameState.etat == GAME_STATE_LEVEL) {
          // Dans le jeu, activer le clignotement du curseur
          cursor.state = CURSOR_STATE_BLINKING;
        }
      } else {
        // Bouton relâché
        if (gameState.etat == GAME_STATE_LEVEL) {
          cursor.state = CURSOR_STATE_NORMAL;
          shouldShowCursor = true; // Toujours visible quand on relâche le bouton
        }
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
  }

  // ===== MACHINE À ÉTATS POUR LES TRAITEMENTS SPÉCIFIQUES =====
  switch (gameState.etat) {    case GAME_STATE_MENU:      // Gestion de la sélection de niveau via potentiomètre (4 fois par seconde = tous les 10 cycles)
      if (periodicCounter % 10 == 0 && !menuState.validationMode) {        // Lire la valeur du potentiomètre et la mapper sur les niveaux 1-9 (INVERSÉ)
        int potValue = analogRead(POT_PIN);
        
        // Mapping inversé équitable : 1024 valeurs réparties sur 9 niveaux (~114 valeurs par niveau)
        // potValue 0-113 = niveau 9, 114-227 = niveau 8, ..., 912-1023 = niveau 1
        uint8_t newLevel;
        if (potValue <= 113) {
          newLevel = 9; // niveau 9 pour 0-113 (114 valeurs)
        } else if (potValue <= 227) {
          newLevel = 8; // niveau 8 pour 114-227 (114 valeurs)
        } else if (potValue <= 341) {
          newLevel = 7; // niveau 7 pour 228-341 (114 valeurs)
        } else if (potValue <= 455) {
          newLevel = 6; // niveau 6 pour 342-455 (114 valeurs)
        } else if (potValue <= 569) {
          newLevel = 5; // niveau 5 pour 456-569 (114 valeurs)
        } else if (potValue <= 683) {
          newLevel = 4; // niveau 4 pour 570-683 (114 valeurs)
        } else if (potValue <= 797) {
          newLevel = 3; // niveau 3 pour 684-797 (114 valeurs)
        } else if (potValue <= 911) {
          newLevel = 2; // niveau 2 pour 798-911 (114 valeurs)
        } else {
          newLevel = 1; // niveau 1 pour 912-1023 (112 valeurs)
        }
          // Mettre à jour seulement si le niveau a changé
        if (newLevel != menuState.selectedLevel) {
          // Effacer l'ancien chiffre
          eraseMenuDigit(menuState.selectedLevel);
          
          // Mettre à jour le niveau sélectionné dans les deux variables
          menuState.selectedLevel = newLevel;
          persistentSelectedLevel = newLevel; // CORRECTION: Sauvegarder dans la variable persistante
          gameState.level = newLevel; // Synchroniser avec l'état du jeu
          
          // Dessiner le nouveau chiffre
          drawMenuDigit(menuState.selectedLevel);        
#if DEBUG_SERIAL
          Serial.print("Pot:");
          Serial.print(potValue);
          Serial.print(" L:");
          Serial.println(menuState.selectedLevel);
#endif
        }
      }
      break;
      
    case GAME_STATE_LEVEL:
      // Gestion du clignotement du curseur (5 fois par seconde = tous les 8 cycles)
      if (periodicCounter % 8 == 0) {
        if (cursor.state == CURSOR_STATE_BLINKING) {
          shouldShowCursor = !shouldShowCursor; // Inverser l'état d'affichage du curseur
          displayNeedsUpdate = true;
          
          // Vérifier les collisions pendant le clignotement
          checkCursorCollision();
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
      }

      // Déplacement des blocs - fréquence selon le niveau de difficulté
      if (periodicCounter % blockMoveCycles == 0) {
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
            blocks[i].x--;

            // Nouvelle logique : ajouter 2 pixels au score max pour chaque colonne qui passe x=3
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
      }      // Création de nouvelles notes - fréquence selon le niveau de difficulté
      if (periodicCounter % noteCreationCycles == 0) {
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
      break;
      
    case GAME_STATE_WIN:
      // Pas de traitement spécifique périodique en mode victoire
      // Tout est géré dans handleWinState() dans loop()
      break;
      
    case GAME_STATE_LOSE:
      // Pas de traitement spécifique périodique en mode défaite
      // Tout est géré dans handleLoseState() dans loop()
      break;
      
    default:
      // État invalide, pas de traitement spécifique
      break;
  }
}

// ===== IMPLÉMENTATIONS DES FONCTIONS =====
void display7Seg(uint8_t address, uint8_t digit) {
    if (digit > 9) return; // Protection contre les valeurs invalides
    
    // Accéder au tableau Tab7Segts défini dans lib_magic.cpp
    extern unsigned char Tab7Segts[];
    
    Wire.beginTransmission(address);
    Wire.write(0x09);                   // select the GPIO register (cohérent avec setup7Seg)
    Wire.write(Tab7Segts[digit]);       // set register value to display digit
    Wire.endTransmission();
}

// Fonction pour afficher le score transformé (pourcentage) sur A4 et A3
void displayScore(uint8_t transformedScore) {
    // Si score = 100%, afficher 10 (1 sur A3, 0 sur A4)
    if (transformedScore >= 100) {
        display7Seg(A3_ADDR, 9);           
        Wire.beginTransmission(A4_ADDR); 
        Wire.write(0x09);
        Wire.write(0b10111111); 
        Wire.endTransmission();        
    } else {
        // Affichage normal pour score < 100%
        uint8_t unite = transformedScore % 10;
        uint8_t dizaine = (transformedScore / 10) % 10;
        
        display7Seg(A4_ADDR, unite);       // A4 = unité
        display7Seg(A3_ADDR, dizaine);     // A3 = dizaine
    }
}

// Fonction pour afficher le niveau sur A1
void displayLevel(uint8_t level) {
    if (level > 9) level = 9; // Protection : niveau max = 9
    display7Seg(A1_ADDR, level);
}

// Fonction pour afficher "MENU" sur les 4 afficheurs pendant le menu
void displayMENU() {
    Serial.println("displayMENU()");
    
    // Codes personnalisés pour MENU (de gauche à droite : A1, A2, A3, A4) dernier bit = virgule
    uint8_t codeM = 0b01101110; // M sur A1
    uint8_t codeE = 0b11111000; // E sur A2
    uint8_t codeN = 0b11000100; // N sur A3
    uint8_t codeU = 0b01110110; // U sur A4
    
    Serial.println("Envoi codes...");
    
    Wire.beginTransmission(A1_ADDR); // A1
    Wire.write(0x09);
    Wire.write(codeM); // M
    Wire.endTransmission();
    
    Wire.beginTransmission(A2_ADDR); // A2  
    Wire.write(0x09);
    Wire.write(codeE); // E
    Wire.endTransmission();
    
    Wire.beginTransmission(A3_ADDR); // A3
    Wire.write(0x09);
    Wire.write(codeN); // N
    Wire.endTransmission();

    
    Wire.beginTransmission(A4_ADDR); // A4
    Wire.write(0x09);    Wire.write(codeU); // U
    Wire.endTransmission();
    
    Serial.println("displayMENU() OK");
}

// Fonction pour éteindre tous les afficheurs 7 segments
void clear7Seg() {
    for (uint8_t addr = A4_ADDR; addr <= A1_ADDR; addr++) {
        Wire.beginTransmission(addr);
        Wire.write(0x09);
        Wire.write(0x09); // Éteint
        Wire.endTransmission();
    }
}

// Fonction optimisée pour mettre à jour l'affichage 7 segments selon l'état du jeu
void update7SegDisplay(uint8_t gameState, uint8_t transformedScore, uint8_t level) {
    // Optimisation : ne mettre à jour que si quelque chose a changé
    bool needsUpdate = false;
    
    // Vérifier si l'état du jeu a changé
    if (gameState != last7SegGameState) {
        needsUpdate = true;
        last7SegGameState = gameState;
          // Réinitialiser les flags pour forcer la mise à jour des autres valeurs
        if (gameState == GAME_STATE_MENU) {
            menu7SegInitialized = false;
        }
        last7SegScore = 255;  // Forcer la mise à jour du score
        last7SegLevel = 255;  // Forcer la mise à jour du niveau
    }
    
    switch (gameState) {
        case 0: // GAME_STATE_MENU
            // Initialiser le menu seulement une fois
            if (!menu7SegInitialized) {
                Serial.println("MENU init");
                displayMENU();
                menu7SegInitialized = true;
            }
            break;
            
        case 1: // GAME_STATE_LEVEL (jeu en cours)
            // Mettre à jour seulement si le score ou le niveau a changé
            if (transformedScore != last7SegScore) {
                displayScore(transformedScore);
                last7SegScore = transformedScore;
#if DEBUG_SERIAL
                Serial.print("S:");
                Serial.print(transformedScore);
                Serial.println("%");
#endif
            }
            if (level != last7SegLevel) {
                displayLevel(level);
                last7SegLevel = level;
#if DEBUG_SERIAL
                Serial.print("L:");
                Serial.println(level);
#endif
            }
            break;
            
        case 2: // GAME_STATE_WIN 
        case 3: // GAME_STATE_LOSE
            // Mettre à jour seulement si le score ou le niveau a changé
            if (transformedScore != last7SegScore) {
                displayScore(transformedScore);
                last7SegScore = transformedScore;
            }
            if (level != last7SegLevel) {
                displayLevel(level);
                last7SegLevel = level;
            }
            break;
            
        default:
            if (needsUpdate) {
                clear7Seg();
            }
            break;
    }
}

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

// Fonction pour obtenir le nombre de cycles de création de notes selon le niveau de difficulté
uint8_t getDifficultyNoteCreationCycles(uint8_t level) {
  switch (level) {
    case 1: return NOTE_CREATION_CYCLES_LEVEL_1;
    case 2: return NOTE_CREATION_CYCLES_LEVEL_2;
    case 3: return NOTE_CREATION_CYCLES_LEVEL_3;
    case 4: return NOTE_CREATION_CYCLES_LEVEL_4;
    case 5: return NOTE_CREATION_CYCLES_LEVEL_5;
    case 6: return NOTE_CREATION_CYCLES_LEVEL_6;
    case 7: return NOTE_CREATION_CYCLES_LEVEL_7;
    case 8: return NOTE_CREATION_CYCLES_LEVEL_8;
    case 9: return NOTE_CREATION_CYCLES_LEVEL_9;
    default: return NOTE_CREATION_CYCLES_LEVEL_1; // Par défaut niveau 1
  }
}

// Fonction pour définir le niveau de difficulté
void setDifficultyLevel(uint8_t level) {
  if (level >= MIN_DIFFICULTY_LEVEL && level <= MAX_DIFFICULTY_LEVEL) {
    currentDifficultyLevel = level;
    blockMoveCycles = getDifficultyBlockMoveCycles(level);
    noteCreationCycles = getDifficultyNoteCreationCycles(level);
    
#if DEBUG_SERIAL
    Serial.print("Lvl:");
    Serial.print(level);
    Serial.print(" BlcCyc:");
    Serial.print(blockMoveCycles);
    Serial.print(" NoteCyc:");
    Serial.println(noteCreationCycles);
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
  // Convertir les durées musicales (1-32) en longueurs visuelles (1-8 pixels)
  // Amélioration : meilleure répartition pour les nouveaux patterns rythmiques
  uint8_t length;
  if (note.duration >= 32) length = 8;        // Niveau 1: 32 -> 8 pixels
  else if (note.duration >= 24) length = 6;   // Niveau 2: 24 -> 6 pixels
  else if (note.duration >= 16) length = 5;   // Niveau 3: 16 -> 5 pixels
  else if (note.duration >= 12) length = 4;   // Niveau 4: 12 -> 4 pixels
  else if (note.duration >= 8) length = 3;    // Niveau 5: 8 -> 3 pixels
  else if (note.duration >= 6) length = 3;    // Niveau 6: 6 -> 3 pixels
  else if (note.duration >= 4) length = 2;    // Niveau 7: 4 -> 2 pixels
  else if (note.duration >= 3) length = 2;    // Niveau 8/9: 3 -> 2 pixels (AJOUT)
  else if (note.duration >= 2) length = 1;    // Niveau 8/9: 2 -> 1 pixel (MODIFIÉ)
  else length = 1;                             // Niveau 9: 1 -> 1 pixel
  
  if (length < 1) length = 1;     // Longueur minimale de 1
  
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
  // Commencer à la dernière colonne visible pour apparition progressive
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
  if (songPosition == lastNotePosition) {
#if DEBUG_SERIAL
    Serial.println("Note dupliquée");
#endif
    return;
  }
  
  // Mémoriser cette position pour éviter la duplication
  lastNotePosition = songPosition;
  
  // Déterminer quelle partie de la chanson nous jouons selon le niveau de difficulté
  switch (currentSongPart) {
    case 0:  // Intro
      switch (currentDifficultyLevel) {
        case 1: currentSong = level1_intro; currentSongSize = LEVEL1_INTRO_SIZE; break;
        case 2: currentSong = level2_intro; currentSongSize = LEVEL2_INTRO_SIZE; break;
        case 3: currentSong = level3_intro; currentSongSize = LEVEL3_INTRO_SIZE; break;
        case 4: currentSong = level4_intro; currentSongSize = LEVEL4_INTRO_SIZE; break;
        case 5: currentSong = level5_intro; currentSongSize = LEVEL5_INTRO_SIZE; break;
        case 6: currentSong = level6_intro; currentSongSize = LEVEL6_INTRO_SIZE; break;
        case 7: currentSong = level7_intro; currentSongSize = LEVEL7_INTRO_SIZE; break;
        case 8: currentSong = level8_intro; currentSongSize = LEVEL8_INTRO_SIZE; break;
        case 9: currentSong = level9_intro; currentSongSize = LEVEL9_INTRO_SIZE; break;
        default: currentSong = intro; currentSongSize = INTRO_SIZE; break; // Fallback
      }
      break;
    case 1:  // Verse
      switch (currentDifficultyLevel) {
        case 1: currentSong = level1_verse; currentSongSize = LEVEL1_VERSE_SIZE; break;
        case 2: currentSong = level2_verse; currentSongSize = LEVEL2_VERSE_SIZE; break;
        case 3: currentSong = level3_verse; currentSongSize = LEVEL3_VERSE_SIZE; break;
        case 4: currentSong = level4_verse; currentSongSize = LEVEL4_VERSE_SIZE; break;
        case 5: currentSong = level5_verse; currentSongSize = LEVEL5_VERSE_SIZE; break;
        case 6: currentSong = level6_verse; currentSongSize = LEVEL6_VERSE_SIZE; break;
        case 7: currentSong = level7_verse; currentSongSize = LEVEL7_VERSE_SIZE; break;
        case 8: currentSong = level8_verse; currentSongSize = LEVEL8_VERSE_SIZE; break;
        case 9: currentSong = level9_verse; currentSongSize = LEVEL9_VERSE_SIZE; break;
        default: currentSong = verse; currentSongSize = VERSE_SIZE; break; // Fallback
      }
      break;
    case 2:  // Chorus
      switch (currentDifficultyLevel) {
        case 1: currentSong = level1_chorus; currentSongSize = LEVEL1_CHORUS_SIZE; break;
        case 2: currentSong = level2_chorus; currentSongSize = LEVEL2_CHORUS_SIZE; break;
        case 3: currentSong = level3_chorus; currentSongSize = LEVEL3_CHORUS_SIZE; break;
        case 4: currentSong = level4_chorus; currentSongSize = LEVEL4_CHORUS_SIZE; break;
        case 5: currentSong = level5_chorus; currentSongSize = LEVEL5_CHORUS_SIZE; break;
        case 6: currentSong = level6_chorus; currentSongSize = LEVEL6_CHORUS_SIZE; break;
        case 7: currentSong = level7_chorus; currentSongSize = LEVEL7_CHORUS_SIZE; break;
        case 8: currentSong = level8_chorus; currentSongSize = LEVEL8_CHORUS_SIZE; break;
        case 9: currentSong = level9_chorus; currentSongSize = LEVEL9_CHORUS_SIZE; break;
        default: currentSong = chorus; currentSongSize = CHORUS_SIZE; break; // Fallback
      }
      break;
    case 3:  // Hook
      switch (currentDifficultyLevel) {
        case 1: currentSong = level1_hook; currentSongSize = LEVEL1_HOOK_SIZE; break;
        case 2: currentSong = level2_hook; currentSongSize = LEVEL2_HOOK_SIZE; break;
        case 3: currentSong = level3_hook; currentSongSize = LEVEL3_HOOK_SIZE; break;
        case 4: currentSong = level4_hook; currentSongSize = LEVEL4_HOOK_SIZE; break;
        case 5: currentSong = level5_hook; currentSongSize = LEVEL5_HOOK_SIZE; break;
        case 6: currentSong = level6_hook; currentSongSize = LEVEL6_HOOK_SIZE; break;
        case 7: currentSong = level7_hook; currentSongSize = LEVEL7_HOOK_SIZE; break;
        case 8: currentSong = level8_hook; currentSongSize = LEVEL8_HOOK_SIZE; break;
        case 9: currentSong = level9_hook; currentSongSize = LEVEL9_HOOK_SIZE; break;
        default: currentSong = hookPart; currentSongSize = HOOK_SIZE; break; // Fallback
      }
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
  static uint8_t lastGameState = 255; // Pour détecter le changement d'état
    // Détecter si on vient de changer d'état ou si une réinitialisation est forcée
  if (lastGameState != gameState.etat || forceMenuReinit) {
    menuInitialized = false; // Forcer la réinitialisation
    lastGameState = gameState.etat;
    forceMenuReinit = false; // Réinitialiser le flag
    
    // CORRECTION CRITIQUE: Double nettoyage pour éliminer la contamination de la mémoire shadow
    ht1632_clear();
    delay(10); // Petite pause pour laisser le temps au hardware
    ht1632_clear();
    
    // CORRECTION CRITIQUE: Nettoyage manuel complet de la shadowram
    for (int chip = 0; chip < 4; chip++) {
      for (int addr = 0; addr < 64; addr++) {
        ht1632_shadowram[addr][chip] = 0;
      }
    }
  }
  
  if (!menuInitialized) {
    ht1632_clear();
    
    // Initialiser l'état du menu
    initMenuState();
    
    // Afficher le menu complet avec système intelligent
    drawFullMenu();
    
#if DEBUG_SERIAL
    Serial.println("MENU");
    Serial.println("Pot:lvl Btn:start");
#endif
    menuInitialized = true;
  }
  
  // Gestion de la validation (clignotement)
  updateMenuValidation();
  
  // Note: La gestion du potentiomètre et du bouton est maintenant dans periodicFunction()
  // pour éviter les conflits entre les deux fonctions
}

// Gestion de l'état de jeu (niveau)
void handleLevelState() {
  static bool levelInitialized = false;
  static uint8_t lastGameState = 255; // Pour détecter le changement d'état
  
  // Détecter si on vient de changer d'état
  if (lastGameState != gameState.etat) {
    levelInitialized = false; // Forcer la réinitialisation
    lastGameState = gameState.etat;
    
    // CORRECTION CRITIQUE: Double nettoyage pour éliminer la contamination de la mémoire shadow
    ht1632_clear();
    delay(10); // Petite pause pour laisser le temps au hardware
    ht1632_clear();
    
    // CORRECTION CRITIQUE: Nettoyage manuel complet de la shadowram
    for (int chip = 0; chip < 4; chip++) {
      for (int addr = 0; addr < 64; addr++) {
        ht1632_shadowram[addr][chip] = 0;
      }
    }
  }
  
  if (!levelInitialized) {
    // CORRECTION CRITIQUE: Vérifier et restaurer le niveau correct si nécessaire
    if (gameState.level == 0 || gameState.level > 9) {
      gameState.level = (persistentSelectedLevel > 0 && persistentSelectedLevel <= 9) ? 
                        persistentSelectedLevel : 1;
#if DEBUG_SERIAL
      Serial.print("CORRECTION: Niveau restauré à ");
      Serial.println(gameState.level);
#endif
    }
    
    // Initialiser le niveau
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
      // Vérifier que le bouton n'est pas pressé pour éviter le skip automatique
      bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
      if (!buttonPressed) {
        // Niveau terminé - évaluer le score transformé
        if (gameScore.transformed >= 80) {
          changeGameState(GAME_STATE_WIN);
        } else {
          changeGameState(GAME_STATE_LOSE);
        }
        levelInitialized = false;
      }
      // Si le bouton est pressé, attendre qu'il soit relâché avant de changer d'état
    }
  }
  
  // Note: Les conditions de défaite seront définies plus tard selon les besoins du jeu
}

// Gestion de l'état de victoire
void handleWinState() {
  static bool winInitialized = false;
  static uint32_t winDisplayTime = 0;
  static uint8_t lastGameState = 255; // Pour détecter le changement d'état
    // Détecter si on vient de changer d'état
  if (lastGameState != gameState.etat) {
    winInitialized = false; // Forcer la réinitialisation
    lastGameState = gameState.etat;
    
    // CORRECTION CRITIQUE: Double nettoyage pour éliminer la contamination de la mémoire shadow
    ht1632_clear();
    delay(10); // Petite pause pour laisser le temps au hardware
    ht1632_clear();
    
    // CORRECTION CRITIQUE: Nettoyage manuel complet de la shadowram
    for (int chip = 0; chip < 4; chip++) {
      for (int addr = 0; addr < 64; addr++) {
        ht1632_shadowram[addr][chip] = 0;
      }
    }
  }
    if (!winInitialized) {
    // Afficher l'écran WINNER
    drawWinnerScreen();
    
#if DEBUG_SERIAL
    Serial.println("=== VICTOIRE ===");
    Serial.print("Score fin: ");
    Serial.print(gameScore.current);
    Serial.print("/");
    Serial.print(gameScore.maxPossible);
    Serial.print(" (");
    Serial.print(gameScore.transformed);
    Serial.println("%)");    Serial.print("Temps: ");
    Serial.print(gameState.timeElapsed / 1000);
    Serial.println("s");
    Serial.println("Appuyez sur le bouton pour retourner au menu");
#endif
      winDisplayTime = millis();
    winInitialized = true;
  }
  
  // Gestion de l'interaction avec le bouton (comme pour la phase LOSE)
  static bool buttonWasPressed = false;
  bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
  
  if (buttonPressed && !buttonWasPressed && millis() - winDisplayTime > 500) {
    // Bouton pressé et tempo de sécurité écoulée
    
    // CORRECTION : Nettoyer l'écran WINNER avant de changer d'état
    eraseWinnerScreen();
    
#if DEBUG_SERIAL
    Serial.println("Transition WIN -> MENU : écran nettoyé");
#endif
    
    // Toujours retourner au menu (comme demandé)
    changeGameState(GAME_STATE_MENU);
    winInitialized = false;
    buttonWasPressed = false;
  }
  
  buttonWasPressed = buttonPressed;
}

// Gestion de l'état de défaite
void handleLoseState() {
  static bool loseInitialized = false;
  static uint32_t loseDisplayTime = 0;
  static uint8_t lastGameState = 255; // Pour détecter le changement d'état
    // Détecter si on vient de changer d'état
  if (lastGameState != gameState.etat) {
    loseInitialized = false; // Forcer la réinitialisation
    lastGameState = gameState.etat;
    
    // CORRECTION CRITIQUE: Double nettoyage pour éliminer la contamination de la mémoire shadow
    ht1632_clear();
    delay(10); // Petite pause pour laisser le temps au hardware
    ht1632_clear();
    
    // CORRECTION CRITIQUE: Nettoyage manuel complet de la shadowram
    for (int chip = 0; chip < 4; chip++) {
      for (int addr = 0; addr < 64; addr++) {
        ht1632_shadowram[addr][chip] = 0;
      }
    }
  }
    if (!loseInitialized) {
    // Afficher l'écran LOSER
    drawLoserScreen();
    
#if DEBUG_SERIAL
    Serial.println("=== DÉFAITE ===");
    Serial.print("Score fin: ");
    Serial.print(gameScore.current);
    Serial.print("/");
    Serial.print(gameScore.maxPossible);
    Serial.print(" (");
    Serial.print(gameScore.transformed);
    Serial.println("%)");
    Serial.println("Appuyez sur le bouton pour retourner au menu");
#endif
    
    loseDisplayTime = millis();
    loseInitialized = true;
  }
  
  // Retourner au menu si le bouton est pressé (après une petite tempo pour éviter les rebonds)
  static bool buttonWasPressed = false;
  bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
    if (buttonPressed && !buttonWasPressed && millis() - loseDisplayTime > 500) {
    // Bouton pressé et tempo de sécurité écoulée
    
    // CORRECTION : Nettoyer l'écran LOSER avant de changer d'état
    eraseLoserScreen();
    
#if DEBUG_SERIAL
    Serial.println("Transition LOSE -> MENU : écran nettoyé");
#endif
    
    changeGameState(GAME_STATE_MENU);
    loseInitialized = false;
    buttonWasPressed = false;
  }
  
  buttonWasPressed = buttonPressed;
}

// Fonction pour changer l'état du jeu
void changeGameState(uint8_t newState) {
  if (newState >= GAME_STATE_MENU && newState <= GAME_STATE_LOSE) {
    gameState.etat = newState;
    clear7Seg();
    
    // CORRECTION CRITIQUE: Nettoyage complet de la matrice LED lors du changement d'état
    // Triple nettoyage pour s'assurer que la shadow memory est complètement effacée
    ht1632_clear();
    delay(10);
    ht1632_clear();
    delay(10);
    ht1632_clear();
    
    // CORRECTION CRITIQUE: Nettoyage manuel forcé de toute la shadowram
    for (int chip = 0; chip < 4; chip++) {
      for (int addr = 0; addr < 64; addr++) {
        ht1632_shadowram[addr][chip] = 0;
      }
    }
    
    // Réinitialiser les flags d'optimisation 7seg pour forcer la mise à jour
    last7SegGameState = 255;
    last7SegScore = 255;
    last7SegLevel = 255;
    menu7SegInitialized = false;
      // Réinitialiser l'état du menu si on quitte le menu
    if (newState != GAME_STATE_MENU) {
      menuState.validationMode = false;
      menuState.boxVisible = true;
    } else {
      // CORRECTION : Forcer la réinitialisation du menu quand on y retourne
      forceMenuReinit = true;
    }
    
    // CORRECTION : Signaler qu'il faut réinitialiser l'état du bouton
    // pour éviter la validation instantanée du menu
    needButtonReset = true;

#if DEBUG_SERIAL
    Serial.print("État: ");
    switch (newState) {
      case GAME_STATE_MENU: Serial.println("MENU"); break;
      case GAME_STATE_LEVEL: Serial.println("NIV"); break;
      case GAME_STATE_WIN: Serial.println("WIN"); break;
      case GAME_STATE_LOSE: Serial.println("LOSE"); break;
    }
    Serial.println("Écran nettoyé - Réinitialisation bouton programmée");
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
  gameScore.current += points;  updateTransformedScore();
  
#if DEBUG_SERIAL
  Serial.print("S+");
  Serial.print(points);
  Serial.print("=");
  Serial.print(gameScore.current);
  Serial.print("/");
  Serial.print(gameScore.maxPossible);
  Serial.print("(");
  Serial.print(gameScore.transformed);
  Serial.println("%)");
#endif
}

// Ajouter des points au score maximum possible
void addMaxScore(uint16_t points) {  gameScore.maxPossible += points;
  updateTransformedScore();
  
#if DEBUG_SERIAL
  Serial.print("Mx+");
  Serial.print(points);
  Serial.print("=");
  Serial.print(gameScore.maxPossible);
  Serial.print("(tr:");
  Serial.print(gameScore.transformed);
  Serial.println("%)");
#endif
}

// Calculer et mettre à jour le score transformé
void updateTransformedScore() {
  uint8_t oldTransformed = gameScore.transformed;
  
  if (gameScore.maxPossible > 0) {
    // Calcul du pourcentage arrondi vers le bas
    uint32_t percentage = ((uint32_t)gameScore.current * 100) / gameScore.maxPossible;
    gameScore.transformed = (uint8_t)percentage;
  } else {
    gameScore.transformed = 0;
  }
  
  // Déclencher une mise à jour 7seg si le score a changé
  if (oldTransformed != gameScore.transformed && gameState.etat == GAME_STATE_LEVEL) {
    // Le score a changé pendant le jeu, forcer la mise à jour lors du prochain cycle
    last7SegScore = 255; // Invalider le cache pour forcer la mise à jour
  }
}

// ===== FONCTIONS DE DÉTECTION DE COLLISION =====

// Calculer quels pixels du bloc sont touchés par le curseur
uint32_t getBlockPixelsHitByCursor(uint8_t blockIndex) {
  if (!blocks[blockIndex].active) {
    return 0;
  }
  
  Block& block = blocks[blockIndex];
  uint32_t pixelsHit = 0;
  
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
  // On encode les pixels touchés dans un masque de bits 32 bits
  for (uint8_t pixelY = 0; pixelY < BLOCK_HEIGHT; pixelY++) {
    uint8_t absoluteY = block.y + pixelY;
    
    // Ce pixel Y intersecte-t-il avec le curseur ?
    if (absoluteY >= cursorY1 && absoluteY <= cursorY2) {
      // Vérifier chaque pixel X du bloc (suppression de la limite artificielle)
      for (uint8_t pixelX = 0; pixelX < block.length && pixelX < 32; pixelX++) { // Limite à 32 pour le masque
        int16_t absoluteX = block.x + pixelX;
        
        // Ce pixel X intersecte-t-il avec le curseur (colonnes 2-3) ?
        if (absoluteX >= cursorX1 && absoluteX <= cursorX2) {
          // Calculer l'index du pixel dans le masque (row-major order)
          uint8_t pixelIndex = pixelY * block.length + pixelX;
          
          // Vérifier que l'index ne dépasse pas 31 (masque 32 bits)
          if (pixelIndex < 32) {
            pixelsHit |= (1UL << pixelIndex);
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
    Serial.print(__builtin_popcountl(pixelsHit));
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
    uint32_t pixelsHit = getBlockPixelsHitByCursor(i);
    
    if (pixelsHit == 0) {
      continue; // Pas de collision pour ce bloc
    }
    
    // Calculer quels sont les nouveaux pixels (pas encore touchés)
    uint32_t newPixelsHit = pixelsHit & (~blocks[i].hitPixels);
    
    if (newPixelsHit == 0) {
      continue; // Tous ces pixels ont déjà été comptés
    }
    
    // Compter le nombre de nouveaux pixels touchés
    uint8_t newPixelCount = __builtin_popcountl(newPixelsHit);
    
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


void handleLevelLoop() {
  // Variables pour détecter les changements
  static uint32_t lastAudioUpdate = 0;
  static bool prevShouldShowCursor = shouldShowCursor;
  uint32_t currentTime = periodicCounter * TIMER_PERIOD / 1000; // Temps basé on the compteur sans utiliser millis()
  
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

// ===== IMPLÉMENTATIONS DES FONCTIONS MENU =====

// Initialiser l'état du menu
void initMenuState() {
  // CORRECTION CRITIQUE: Récupérer le niveau depuis la variable persistante si disponible
  uint8_t levelToUse = (persistentSelectedLevel > 0 && persistentSelectedLevel <= 9) ? 
                       persistentSelectedLevel : 1;
  
  menuState.selectedLevel = levelToUse;
  persistentSelectedLevel = levelToUse; // S'assurer que les deux sont synchronisées
  gameState.level = levelToUse; // Synchroniser avec l'état du jeu
  
  menuState.boxVisible = true;
  menuState.lastBlinkTime = 0;
  menuState.validationMode = false;
  menuState.validationStart = 0;
  
#if DEBUG_SERIAL
  Serial.print("Menu init: niveau sélectionné = ");
  Serial.println(levelToUse);
#endif
}

// Dessiner le texte "MENU" statique
void drawMenuText() {
  for (uint8_t i = 0; i < menuTextCoordsCount; i++) {
    uint8_t x = pgm_read_byte(&menuTextCoords[i * 2]);
    uint8_t y = pgm_read_byte(&menuTextCoords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_GREEN);
  }
}

// Dessiner la boîte du niveau (rectangle)
void drawMenuBox() {
  for (uint8_t i = 0; i < menuBoxCoordsCount; i++) {
    uint8_t x = pgm_read_byte(&menuBoxCoords[i * 2]);
    uint8_t y = pgm_read_byte(&menuBoxCoords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_ORANGE);
  }
}

// Effacer la boîte du niveau
void eraseMenuBox() {
  for (uint8_t i = 0; i < menuBoxCoordsCount; i++) {
    uint8_t x = pgm_read_byte(&menuBoxCoords[i * 2]);
    uint8_t y = pgm_read_byte(&menuBoxCoords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_OFF);
  }
}

// Dessiner un chiffre dans la boîte (1-9)
void drawMenuDigit(uint8_t digit) {
  if (digit < 1 || digit > 9) return;
  
  uint8_t count = pgm_read_byte(&menuDigitCounts[digit]);
  const uint8_t* coords = (const uint8_t*)pgm_read_word(&menuDigits[digit]);
  
  for (uint8_t i = 0; i < count; i++) {
    uint8_t x = pgm_read_byte(&coords[i * 2]);
    uint8_t y = pgm_read_byte(&coords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_RED);
  }
}

// Effacer un chiffre dans la boîte (1-9)
void eraseMenuDigit(uint8_t digit) {
  if (digit < 1 || digit > 9) return;
  
  uint8_t count = pgm_read_byte(&menuDigitCounts[digit]);
  const uint8_t* coords = (const uint8_t*)pgm_read_word(&menuDigits[digit]);
  
  for (uint8_t i = 0; i < count; i++) {
    uint8_t x = pgm_read_byte(&coords[i * 2]);
    uint8_t y = pgm_read_byte(&coords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_OFF);
  }
}

// Affichage complet du menu
void drawFullMenu() {
  ht1632_clear();
  drawMenuText();
  if (menuState.boxVisible) {
    drawMenuBox();
    drawMenuDigit(menuState.selectedLevel);
  }
}

// Gestion du clignotement de validation
void updateMenuValidation() {
  if (!menuState.validationMode) return;
  
  uint32_t currentTime = millis();
    // Clignoter pendant 1 seconde puis changer d'état
  if (currentTime - menuState.validationStart > 1000) {
    // CORRECTION CRITIQUE: S'assurer que le niveau sélectionné est transféré correctement
    // Utiliser la variable persistante comme référence de vérité
    uint8_t levelToUse = (menuState.selectedLevel > 0 && menuState.selectedLevel <= 9) ? 
                         menuState.selectedLevel : persistentSelectedLevel;
    gameState.level = levelToUse;
    
#if DEBUG_SERIAL
    Serial.print("Transition MENU->LEVEL: niveau ");
    Serial.print(levelToUse);
    Serial.print(" (menu:");
    Serial.print(menuState.selectedLevel);
    Serial.print(" persistent:");
    Serial.print(persistentSelectedLevel);
    Serial.print(") transféré vers gameState.level = ");
    Serial.println(gameState.level);
#endif
    
    changeGameState(GAME_STATE_LEVEL);
    menuState.validationMode = false;
    return;
  }
  
  // Clignotement de la boîte
  if (currentTime - menuState.lastBlinkTime > MENU_BOX_BLINK_INTERVAL) {
    menuState.boxVisible = !menuState.boxVisible;
    
    if (menuState.boxVisible) {
      drawMenuBox();
      drawMenuDigit(menuState.selectedLevel);
    } else {
      eraseMenuBox();
      eraseMenuDigit(menuState.selectedLevel);
    }
    
    menuState.lastBlinkTime = currentTime;
  }
}

// ===== FONCTIONS AFFICHAGE LOSER =====

// Dessiner l'écran LOSER complet
void drawLoserScreen() {
  ht1632_clear();
  
#if DEBUG_SERIAL
  Serial.print("LOSER: ");
  Serial.println(loserEmptyCoordsCount);
#endif
  
  // Remplir tout l'écran en rouge (32x16 = 512 pixels)
  for (uint8_t y = 0; y < 16; y++) {
    for (uint8_t x = 0; x < 32; x++) {
      ht1632_plot(x, y, COLOR_RED);
    }
  }
  
  // Puis enlever les pixels vides pour former le motif "LOSER"
  for (uint16_t i = 0; i < loserEmptyCoordsCount; i++) {
    uint8_t x = pgm_read_byte(&loserEmptyCoords[i * 2]);
    uint8_t y = pgm_read_byte(&loserEmptyCoords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_OFF);
  }
  
#if DEBUG_SERIAL
  Serial.println("LOSER OK");
#endif
}

// Effacer l'écran LOSER
void eraseLoserScreen() {
  // Simple effacement de tout l'écran
  ht1632_clear();
  
#if DEBUG_SERIAL
  Serial.println("LOSER OFF");
#endif
}

// ===== FONCTIONS AFFICHAGE WINNER =====

// Dessiner l'écran WINNER complet
void drawWinnerScreen() {
  ht1632_clear();
  
#if DEBUG_SERIAL
  Serial.print("WINNER: ");
  Serial.println(winnerEmptyCoordsCount);
#endif
  
  // Remplir tout l'écran en vert (32x16 = 512 pixels)
  for (uint8_t y = 0; y < 16; y++) {
    for (uint8_t x = 0; x < 32; x++) {
      ht1632_plot(x, y, COLOR_GREEN);
    }
  }
  
  // Puis enlever les pixels vides pour former le motif "WINNER"
  for (uint16_t i = 0; i < winnerEmptyCoordsCount; i++) {
    uint8_t x = pgm_read_byte(&winnerEmptyCoords[i * 2]);
    uint8_t y = pgm_read_byte(&winnerEmptyCoords[i * 2 + 1]);
    ht1632_plot(x, y, COLOR_OFF);
  }
  
#if DEBUG_SERIAL
  Serial.println("WINNER OK");
#endif
}

// Effacer l'écran WINNER
void eraseWinnerScreen() {
  // Simple effacement de tout l'écran
  ht1632_clear();
  
#if DEBUG_SERIAL
  Serial.println("WINNER OFF");
#endif
}
