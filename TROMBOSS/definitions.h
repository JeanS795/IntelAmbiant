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
#define MAX_BLOCKS 18

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
#define BLOCK_MOVE_CYCLES_LEVEL_1 40
#define BLOCK_MOVE_CYCLES_LEVEL_2 35
#define BLOCK_MOVE_CYCLES_LEVEL_3 30
#define BLOCK_MOVE_CYCLES_LEVEL_4 25
#define BLOCK_MOVE_CYCLES_LEVEL_5 20
#define BLOCK_MOVE_CYCLES_LEVEL_6 15
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
  uint16_t hitPixels;     // Masque de bits pour les pixels déjà touchés (max 16 pixels)
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

// ===== STRUCTURE SCORE =====
typedef struct {
  uint16_t current;         // Score actuel du joueur
  uint16_t maxPossible;     // Score maximum actuellement atteignable (points qui ont passé la ligne)
  uint8_t transformed;      // Score transformé en pourcentage (0-100)
} Score;

// ===== STRUCTURE JEU =====
typedef struct {
  uint8_t etat;           // État du jeu: GAME_STATE_MENU, GAME_STATE_LEVEL, GAME_STATE_WIN, GAME_STATE_LOSE
  uint8_t level;          // Niveau de difficulté (1-9)
  uint32_t timeStart;     // Temps de début de niveau (en ms)
  uint32_t timeElapsed;   // Temps écoulé depuis le début
  bool gameOver;          // Flag de fin de jeu
  bool pauseGame;         // Flag de pause
} GameState;

// ===== ADRESSES DES AFFICHEURS 7 SEGMENTS =====
// Organisation des afficheurs :
// A1 (niveau) = 0x23
// A2 (non utilisé) = 0x22  
// A3 (dizaine score) = 0x21
// A4 (unité score) = 0x20
const uint8_t A1_ADDR = 0x23;
const uint8_t A2_ADDR = 0x22;
const uint8_t A3_ADDR = 0x21;
const uint8_t A4_ADDR = 0x20;

// ===== VARIABLES D'OPTIMISATION AFFICHAGE 7 SEGMENTS =====
uint8_t last7SegScore = 255;        // Dernier score affiché (255 = non initialisé)
uint8_t last7SegLevel = 255;        // Dernier niveau affiché (255 = non initialisé)
uint8_t last7SegGameState = 255;    // Dernier état de jeu affiché (255 = non initialisé)
bool menu7SegInitialized = false;   // Flag pour savoir si le menu a été initialisé sur 7seg

// ===== VARIABLES GLOBALES PARTAGÉES =====
volatile uint16_t periodicCounter = 0;

Cursor cursor;
Block blocks[MAX_BLOCKS];
volatile bool displayNeedsUpdate = false;
volatile bool shouldShowCursor = true;

// Variable principale de l'état du jeu
GameState gameState;

// Variable de score
Score gameScore;

// Variables pour la musique
uint8_t songPosition = 0;
uint8_t currentSongPart = 0;
uint8_t songFinished = 0;
uint8_t lastNoteFrequency = 0;
bool lastNoteStillActive = false;
bool blockNotePlaying[MAX_BLOCKS] = {0};

// Variable globale pour garder trace de la dernière note créée
uint8_t lastNotePosition = 255;

// Variables pour le système de niveaux
uint8_t currentDifficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
uint8_t blockMoveCycles = BLOCK_MOVE_CYCLES_LEVEL_6;

// ===== CONSTANTES MENU =====
#define MENU_LEVEL_MIN 1
#define MENU_LEVEL_MAX 9
#define MENU_BOX_BLINK_INTERVAL 300  // ms pour le clignotement de validation

// ===== STRUCTURES MENU =====
typedef struct {
  uint8_t selectedLevel;     // Niveau sélectionné (1-9)
  bool boxVisible;           // Visibilité de la boîte (pour clignotement)
  uint32_t lastBlinkTime;    // Dernier temps de clignotement
  bool validationMode;       // Mode validation (clignotement avant passage au jeu)
  uint32_t validationStart;  // Début de la validation
} MenuState;

// ===== VARIABLES GLOBALES MENU =====
MenuState menuState;

// ===== DONNÉES MENU COMPRESSÉES =====

// ===== FONCTIONS AFFICHAGE 7 SEGMENTS =====
// Fonction pour afficher un chiffre sur un afficheur 7 segments spécifique
void display7Seg(uint8_t address, uint8_t digit);
// Fonction pour afficher le score transformé (pourcentage) sur A4 et A3
void displayScore(uint8_t transformedScore);
// Fonction pour afficher le niveau sur A1
void displayLevel(uint8_t level);
// Fonction pour afficher "MENU" sur les 4 afficheurs pendant le menu
void displayMENU();
// Fonction pour éteindre tous les afficheurs 7 segments
void clear7Seg();
// Fonction optimisée pour mettre à jour l'affichage 7 segments selon l'état du jeu
void update7SegDisplay(uint8_t gameState, uint8_t transformedScore, uint8_t level);

// ===== FONCTIONS UTILITAIRES =====
// Fonction pour obtenir le nombre de cycles selon le niveau de difficulté
uint8_t getDifficultyBlockMoveCycles(uint8_t level);
// Fonction pour définir le niveau de difficulté
void setDifficultyLevel(uint8_t level);
// Fonction pour déterminer la position Y en fonction de la fréquence
uint8_t getPositionYFromFrequency(uint16_t frequency);
// Fonction pour vérifier si une position verticale est déjà occupée par un bloc actif
bool isVerticalPositionOccupied(uint8_t posY);
// Fonction pour vérifier si une colonne est déjà occupée par un bloc actif
bool isColumnOccupied(int16_t x);

// ===== FONCTIONS DE GESTION DES BLOCS =====
// Fonction pour créer un nouveau bloc en fonction d'une note
void createNewBlock(const MusicNote* noteArray, uint8_t noteIndex);
// Affiche uniquement la tête du bloc (nouvelle colonne)
void drawBlockHead(const Block& block);
// Fonction pour dessiner un bloc sur la matrice
void drawBlock(Block block);
// Efface uniquement la colonne et lignes concernées par la queue d'un bloc
void eraseBlockTail(const Block& block);
// Fonction pour passer à la prochaine note de la chanson
void nextNote();

// ===== FONCTIONS DE GESTION DU CURSEUR =====
// Fonction périodique pour lire le potentiomètre et calculer la position cible du curseur
void updateCursorFromPot();
// Efface le curseur 2x2 à une position donnée et restaure la colonne verte uniquement sur cette zone
void eraseCursor(uint8_t y);
// Affiche le curseur 2x2 rouge à une position donnée
void drawCursor(uint8_t y);
// Fonction périodique pour déplacer le curseur bloc par bloc avec clignotement si demandé
void periodicMoveCursor();

// ===== FONCTIONS D'AFFICHAGE =====
// Affiche les colonnes 2 et 3 en vert (statique, hors zone curseur et hors zone bloc)
void drawStaticColumnsExceptCursorAndBlocks();

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
// Fonction principale de gestion du niveau
void handleLevelLoop();

// ===== FONCTIONS DE GESTION DU SCORE =====
// Initialisation du score
void initScore();
// Ajouter des points au score actuel
void addScore(uint16_t points);
// Ajouter des points au score maximum possible
void addMaxScore(uint16_t points);
// Calculer et mettre à jour le score transformé
void updateTransformedScore();

// ===== FONCTIONS DE DÉTECTION DE COLLISION =====
// Vérifier si le curseur touche un bloc et marquer les points
void checkCursorCollision();
// Calculer quels pixels du bloc sont touchés par le curseur
uint8_t getBlockPixelsHitByCursor(uint8_t blockIndex);

// ===== FONCTIONS SYSTÈME =====
// Fonction principale setup
void setup();
// Fonction principale loop
void loop();
// Fonction séparée pour la gestion audio
void updateAudio();
// Fonction appelée périodiquement par TimerOne (toutes les 25ms)
void periodicFunction();

// ===== FONCTIONS MENU =====
// Initialiser l'état du menu
void initMenuState();
// Dessiner le texte "MENU" statique
void drawMenuText();
// Dessiner la boîte du niveau (rectangle)
void drawMenuBox();
// Effacer la boîte du niveau
void eraseMenuBox();
// Dessiner un chiffre dans la boîte (1-9)
void drawMenuDigit(uint8_t digit);
// Effacer un chiffre dans la boîte (1-9)
void eraseMenuDigit(uint8_t digit);
// Affichage complet du menu
void drawFullMenu();
// Gestion du clignotement de validation
void updateMenuValidation();

// ===== DONNÉES MENU COMPRESSÉES =====

// Coordonnées du texte "MENU" (format: x, y)
const uint8_t menuTextCoords[] PROGMEM = {
  1,2, 5,2, 7,2, 8,2, 9,2, 10,2, 21,2, 25,2, 27,2, 30,2,
  1,3, 2,3, 4,3, 5,3, 7,3, 21,3, 22,3, 25,3, 27,3, 30,3,
  1,4, 3,4, 5,4, 7,4, 21,4, 22,4, 25,4, 27,4, 30,4,
  1,5, 5,5, 7,5, 8,5, 9,5, 10,5, 21,5, 23,5, 25,5, 27,5, 30,5,
  1,6, 5,6, 7,6, 21,6, 24,6, 25,6, 27,6, 30,6,
  1,7, 5,7, 7,7, 21,7, 24,7, 25,7, 27,7, 30,7,
  1,8, 5,8, 7,8, 8,8, 9,8, 10,8, 21,8, 25,8, 27,8, 28,8, 29,8, 30,8
};
const uint8_t menuTextCoordsCount = sizeof(menuTextCoords) / 2;

// Coordonnées de la boîte (rectangle milieu)
const uint8_t menuBoxCoords[] PROGMEM = {
  12,5, 13,5, 14,5, 15,5, 16,5, 17,5, 18,5, 19,5,
  12,6, 19,6, 12,7, 19,7, 12,8, 19,8, 12,9, 19,9,
  12,10, 19,10, 12,11, 19,11, 12,12, 19,12,
  12,13, 13,13, 14,13, 15,13, 16,13, 17,13, 18,13, 19,13
};
const uint8_t menuBoxCoordsCount = sizeof(menuBoxCoords) / 2;

// Coordonnées des chiffres 1-9 (format: nombre de points, puis x,y,x,y...)
const uint8_t menuDigit1[] PROGMEM = {
  15,7, 16,7, 14,8, 15,8, 16,8, 15,9, 16,9, 15,10, 16,10, 14,11, 15,11, 16,11, 17,11
};
const uint8_t menuDigit2[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 17,8, 14,9, 15,9, 16,9, 17,9, 14,10, 14,11, 15,11, 16,11, 17,11
};
const uint8_t menuDigit3[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 17,8, 14,9, 15,9, 16,9, 17,9, 17,10, 14,11, 15,11, 16,11, 17,11
};
const uint8_t menuDigit4[] PROGMEM = {
  14,7, 16,7, 14,8, 16,8, 14,9, 15,9, 16,9, 17,9, 16,10, 16,11
};
const uint8_t menuDigit5[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 14,8, 14,9, 15,9, 16,9, 17,9, 17,10, 14,11, 15,11, 16,11, 17,11
};
const uint8_t menuDigit6[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 14,8, 14,9, 15,9, 16,9, 17,9, 14,10, 17,10, 14,11, 15,11, 16,11, 17,11
};
const uint8_t menuDigit7[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 17,8, 16,9, 15,10, 14,11
};
const uint8_t menuDigit8[] PROGMEM = {
  15,7, 16,7, 14,8, 17,8, 15,9, 16,9, 14,10, 17,10, 15,11, 16,11
};
const uint8_t menuDigit9[] PROGMEM = {
  14,7, 15,7, 16,7, 17,7, 14,8, 17,8, 14,9, 15,9, 16,9, 17,9, 17,10, 14,11, 15,11, 16,11, 17,11
};

const uint8_t menuDigitCounts[] PROGMEM = {0, 13, 14, 14, 10, 14, 15, 8, 10, 15};
const uint8_t* const menuDigits[] PROGMEM = {
  nullptr, menuDigit1, menuDigit2, menuDigit3, menuDigit4,
  menuDigit5, menuDigit6, menuDigit7, menuDigit8, menuDigit9
};

// ===== COORDONNÉES ÉCRAN LOSER =====
// Coordonnées pour afficher "LOSER" en plein écran (format: x, y)
const uint8_t loserCoords[] PROGMEM = {
  0,0, 1,0, 2,0, 3,0, 4,0, 5,0, 6,0, 7,0, 8,0, 9,0, 10,0, 11,0, 12,0, 13,0, 14,0, 15,0, 16,0, 17,0, 18,0, 19,0, 20,0, 21,0, 22,0, 23,0, 24,0, 25,0, 26,0, 27,0, 28,0, 29,0, 30,0, 31,0,
  0,1, 1,1, 2,1, 3,1, 4,1, 5,1, 6,1, 7,1, 8,1, 9,1, 10,1, 11,1, 12,1, 13,1, 14,1, 15,1, 16,1, 17,1, 18,1, 19,1, 20,1, 21,1, 22,1, 23,1, 24,1, 25,1, 26,1, 27,1, 28,1, 29,1, 30,1, 31,1,
  0,2, 1,2, 2,2, 3,2, 4,2, 5,2, 6,2, 7,2, 8,2, 9,2, 10,2, 11,2, 12,2, 13,2, 14,2, 15,2, 16,2, 17,2, 18,2, 19,2, 20,2, 21,2, 22,2, 23,2, 24,2, 25,2, 26,2, 27,2, 28,2, 29,2, 30,2, 31,2,
  0,3, 1,3, 2,3, 3,3, 4,3, 5,3, 6,3, 7,3, 8,3, 9,3, 10,3, 11,3, 12,3, 13,3, 14,3, 15,3, 16,3, 17,3, 18,3, 19,3, 20,3, 21,3, 22,3, 23,3, 24,3, 25,3, 26,3, 27,3, 28,3, 29,3, 30,3, 31,3,
  0,4, 2,4, 3,4, 4,4, 5,4, 6,4, 7,4, 12,4, 13,4, 19,4, 25,4, 30,4, 31,4,
  0,5, 2,5, 3,5, 4,5, 5,5, 6,5, 8,5, 9,5, 10,5, 11,5, 13,5, 15,5, 16,5, 17,5, 18,5, 19,5, 21,5, 22,5, 23,5, 24,5, 25,5, 27,5, 28,5, 29,5, 31,5,
  0,6, 2,6, 3,6, 4,6, 5,6, 6,6, 8,6, 9,6, 10,6, 11,6, 13,6, 15,6, 16,6, 17,6, 18,6, 19,6, 21,6, 22,6, 23,6, 24,6, 25,6, 27,6, 28,6, 29,6, 31,6,
  0,7, 2,7, 3,7, 4,7, 5,7, 6,7, 8,7, 9,7, 10,7, 11,7, 13,7, 19,7, 24,7, 25,7, 30,7, 31,7,
  0,8, 2,8, 3,8, 4,8, 5,8, 6,8, 8,8, 9,8, 10,8, 11,8, 13,8, 14,8, 15,8, 16,8, 17,8, 19,8, 21,8, 22,8, 23,8, 24,8, 25,8, 27,8, 29,8, 30,8, 31,8,
  0,9, 2,9, 3,9, 4,9, 5,9, 6,9, 8,9, 9,9, 10,9, 11,9, 13,9, 14,9, 15,9, 16,9, 17,9, 19,9, 21,9, 22,9, 23,9, 24,9, 25,9, 27,9, 28,9, 30,9, 31,9,
  0,10, 6,10, 7,10, 12,10, 13,10, 19,10, 25,10, 27,10, 28,10, 29,10, 31,10,
  0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11, 10,11, 11,11, 12,11, 13,11, 14,11, 15,11, 16,11, 17,11, 18,11, 19,11, 20,11, 21,11, 22,11, 23,11, 24,11, 25,11, 26,11, 27,11, 28,11, 29,11, 30,11, 31,11,
  0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12, 10,12, 11,12, 12,12, 13,12, 14,12, 15,12, 16,12, 17,12, 18,12, 19,12, 20,12, 21,12, 22,12, 23,12, 24,12, 25,12, 26,12, 27,12, 28,12, 29,12, 30,12, 31,12,
  0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13, 10,13, 11,13, 12,13, 13,13, 14,13, 15,13, 16,13, 17,13, 18,13, 19,13, 20,13, 21,13, 22,13, 23,13, 24,13, 25,13, 26,13, 27,13, 28,13, 29,13, 30,13, 31,13,
  0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14, 10,14, 11,14, 12,14, 13,14, 14,14, 15,14, 16,14, 17,14, 18,14, 19,14, 20,14, 21,14, 22,14, 23,14, 24,14, 25,14, 26,14, 27,14, 28,14, 29,14, 30,14, 31,14,  0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15, 10,15, 11,15, 12,15, 13,15, 14,15, 15,15, 16,15, 17,15, 18,15, 19,15, 20,15, 21,15, 22,15, 23,15, 24,15, 25,15, 26,15, 27,15, 28,15, 29,15, 30,15, 31,15
};
const uint16_t loserCoordsCount = sizeof(loserCoords) / 2;

// ===== FONCTIONS AFFICHAGE LOSER =====
// Dessiner l'écran LOSER complet
void drawLoserScreen();
// Effacer l'écran LOSER
void eraseLoserScreen();

#endif // DEFINITIONS_H
