#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <Arduino.h>

// ===== CONSTANTES HARDWARE =====
#define POT_PIN A0
#define BUTTON_PIN 12
#define BUZZER_PIN 3

// ===== CONSTANTES MATRICE LED =====
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define BLOCK_HEIGHT 2
#define MAX_BLOCKS 12

// ===== CONSTANTES TIMING =====
#define TIMER_PERIOD 25000  // 25ms en microsecondes

// ===== CONSTANTES COULEURS =====
#define COLOR_OFF 0
#define COLOR_GREEN 1
#define COLOR_RED 2
#define COLOR_ORANGE 3

#define BLOCK_COLOR COLOR_ORANGE
#define BORDER_COLOR COLOR_ORANGE
#define CURSOR_COLOR COLOR_RED
#define GREEN_COLUMN_COLOR COLOR_GREEN

// ===== CONSTANTES ETATS JEU =====
#define GAME_STATE_MENU 0
#define GAME_STATE_LEVEL 1
#define GAME_STATE_WIN 2
#define GAME_STATE_LOSE 3

// ===== CONSTANTES CURSEUR =====
#define CURSOR_WIDTH 2
#define CURSOR_HEIGHT 2
#define CURSOR_COLUMN_START 2
#define CURSOR_BLINK_INTERVAL 200  // ms

// États du curseur
#define CURSOR_STATE_NORMAL 0
#define CURSOR_STATE_BLINKING 1

// Visibilité du curseur
#define CURSOR_VISIBLE 1
#define CURSOR_HIDDEN 0

// ===== CONSTANTES AUDIO =====
#define MUSIQUE 0

// ===== CONSTANTES DEBUG =====
#define DEBUG_SERIAL 1

// ===== CONSTANTES NIVEAUX DE DIFFICULTE =====
#define MIN_DIFFICULTY_LEVEL 1
#define MAX_DIFFICULTY_LEVEL 9
#define DEFAULT_DIFFICULTY_LEVEL 1

// Nombre de cycles entre chaque déplacement de bloc selon le niveau
// Niveau 1 (facile) : 40 cycles = 1 seconde
// Niveau 5 (moyen) : 20 cycles = 0.5 seconde  
// Niveau 9 (difficile) : 8 cycles = 0.2 seconde
#define BLOCK_MOVE_CYCLES_LEVEL_1 40
#define BLOCK_MOVE_CYCLES_LEVEL_2 35
#define BLOCK_MOVE_CYCLES_LEVEL_3 30
#define BLOCK_MOVE_CYCLES_LEVEL_4 25
#define BLOCK_MOVE_CYCLES_LEVEL_5 20
#define BLOCK_MOVE_CYCLES_LEVEL_6 16
#define BLOCK_MOVE_CYCLES_LEVEL_7 12
#define BLOCK_MOVE_CYCLES_LEVEL_8 10
#define BLOCK_MOVE_CYCLES_LEVEL_9 8

// ===== STRUCTURE BLOC =====
typedef struct {
  int16_t x;              // Position horizontale
  uint8_t y;              // Position verticale
  uint8_t length;         // Longueur du bloc
  uint8_t color : 3;      // Couleur du bloc sur 3 bits (0-7)
  uint8_t active : 1;     // Flag actif sur 1 bit
  uint16_t frequency;     // Fréquence de la note associée
  uint8_t needsUpdate : 1; // Flag pour indiquer si le bloc doit être mis à jour
  int16_t oldX;           // Ancienne position X pour effacer
} Block;

// ===== STRUCTURE CURSEUR =====
typedef struct {
  uint8_t y;              // Position Y cible (0-14)
  uint8_t yDisplayed;     // Position Y actuellement affichée
  uint8_t yLast;          // Dernière position affichée (pour effacement)
  uint8_t state;          // État: CURSOR_STATE_NORMAL ou CURSOR_STATE_BLINKING
  uint8_t visible;        // Visibilité: CURSOR_VISIBLE ou CURSOR_HIDDEN
  uint8_t color;          // Couleur du curseur
  uint16_t potValue;      // Valeur du potentiomètre (0-1023)
  uint32_t lastBlinkTime; // Dernier temps de clignotement
} Cursor;

// ===== STRUCTURE JEU =====
typedef struct {
  uint8_t etat;           // État du jeu: GAME_STATE_MENU, GAME_STATE_LEVEL, GAME_STATE_WIN, GAME_STATE_LOSE
  uint8_t level;          // Niveau de difficulté (1-9)
  uint16_t score;         // Score du joueur
  uint32_t timeStart;     // Temps de début de niveau (en ms)
  uint32_t timeElapsed;   // Temps écoulé depuis le début
  uint8_t lives;          // Nombre de vies restantes
  uint8_t blocksHit;      // Nombre de blocs touchés avec succès
  uint8_t blocksMissed;   // Nombre de blocs ratés
  bool gameOver;          // Flag de fin de jeu
  bool pauseGame;         // Flag de pause
} GameState;

// ===== VARIABLES GLOBALES PARTAGÉES =====
extern Cursor cursor;
extern Block blocks[MAX_BLOCKS];
extern volatile uint16_t periodicCounter;
extern volatile bool displayNeedsUpdate;
extern volatile bool shouldShowCursor;

// Variable principale de l'état du jeu
extern GameState gameState;

// Variables pour la musique
extern uint8_t songPosition;
extern uint8_t currentSongPart;
extern uint8_t songFinished;
extern uint8_t lastNoteFrequency;
extern bool lastNoteStillActive;
extern bool blockNotePlaying[MAX_BLOCKS];

// Variable globale pour garder trace de la dernière note créée
extern uint8_t lastNotePosition;

// Variables pour le système de niveaux
extern uint8_t currentDifficultyLevel;
extern uint8_t blockMoveCycles;

// ===== FONCTIONS UTILITAIRES =====
// Fonction pour obtenir le nombre de cycles selon le niveau de difficulté
uint8_t getDifficultyBlockMoveCycles(uint8_t level);
// Fonction pour définir le niveau de difficulté
void setDifficultyLevel(uint8_t level);

// ===== FONCTIONS DE GESTION DU JEU =====
// Initialisation de l'état du jeu
void initGameState();
// Gestion du menu principal
void handleMenuState();
// Gestion de l'état de jeu (niveau)
void handleLevelState();
// Gestion de l'état de victoire
void handleWinState();
// Gestion de l'état de défaite
void handleLoseState();
// Fonction pour changer l'état du jeu
void changeGameState(uint8_t newState);

#endif // DEFINITIONS_H
