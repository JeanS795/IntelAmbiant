#include <Wire.h>
#include "ht1632.h"
#include "song_patterns.h"
#include "fr_jacques_music.h"

// Définition du pin du potentiomètre
const int potPin = A0;

// Constantes pour la matrice LED
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define NUM_COLUMNS 4
#define TILE_SPEED 1 // Vitesse de déplacement des tuiles
#define TILE_HEIGHT 2
#define TILE_WIDTH 2 // Largeur des tuiles
#define TILE_SPAWN_INTERVAL 500 // Intervalle de création des tuiles en ms

// Structure pour les tuiles
struct Tile {
  int x; // Position horizontale
  int y; // Position verticale (ligne)
  bool active; // Indique si la tuile est active
};

Tile tiles[NUM_COLUMNS];
unsigned long lastTileSpawnTime = 0;
unsigned long lastUpdateTime = 0;
int score = 0;

// Débogage 1 = oui, 0 = non
#define DEBUG_SERIAL 1

// Synchronisation des tuiles avec la musique
#include "song_patterns.h"

// Variables pour suivre la progression de la musique
uint8_t currentNoteIndex = 0;
unsigned long lastNoteTime = 0;

// Ajout de la lecture de la musique via le buzzer pendant que le joueur déplace le slider
#include "song_patterns.h"

// Mise à jour pour utiliser la pin 3 pour le buzzer
const int buzzerPin = 3; // Pin du buzzer

void playMusic() {
  static unsigned long lastNoteTime = 0;
  static uint8_t currentNoteIndex = 0;
  static bool isPlaying = false;

  unsigned long currentTime = millis();

  if (!isPlaying) {
    // Démarrer la lecture de la musique
    currentNoteIndex = 0;
    isPlaying = true;
  }

  if (currentNoteIndex < INTRO_SIZE) {
    MusicNote note;
    getNote(intro, currentNoteIndex, &note);

    if (currentTime - lastNoteTime > (60000 / 120) * (4 / note.duration)) { // Basé sur un tempo de 120 BPM
      tone(buzzerPin, note.frequency, (60000 / 120) * (4 / note.duration));
      lastNoteTime = currentTime;
      currentNoteIndex++;
    }
  } else {
    noTone(buzzerPin); // Arrêter le buzzer lorsque la musique est terminée
    isPlaying = false;
  }
}

// Modification pour synchroniser les notes avec la musique "Frère Jacques"
void playMusicWithTiles() {
  static unsigned long lastNoteTime = 0;
  static uint8_t currentNoteIndex = 0;
  static bool musicStarted = false;

  unsigned long currentTime = millis();

  // Vérifier si une tuile atteint la ligne verte pour démarrer la musique
  if (!musicStarted) {
    for (int i = 0; i < NUM_COLUMNS; i++) {
      if (tiles[i].active && tiles[i].x <= 2) { // Ligne verte sur colonnes 2 et 3
        musicStarted = true;
        currentNoteIndex = 0;
        lastNoteTime = currentTime;
        break;
      }
    }
  }

  if (musicStarted && currentNoteIndex < FRERE_JACQUES_SIZE) {
    MusicNote note;
    getNote(frereJacques, currentNoteIndex, &note);

    // Jouer la note si le temps est écoulé
    if (currentTime - lastNoteTime >= (60000 / 120) * (4 / note.duration)) { // Basé sur un tempo de 120 BPM
      tone(buzzerPin, note.frequency, (60000 / 120) * (4 / note.duration));
      lastNoteTime = currentTime;
      currentNoteIndex++;
    }
  } else if (currentNoteIndex >= FRERE_JACQUES_SIZE) {
    noTone(buzzerPin); // Arrêter le buzzer lorsque la musique est terminée
    musicStarted = false; // Réinitialiser pour rejouer si nécessaire
  }
}

void initializeTiles() {
  for (int i = 0; i < NUM_COLUMNS; i++) {
    tiles[i].x = MATRIX_WIDTH; // Hors de l'écran au départ
    tiles[i].y = i;
    tiles[i].active = false;
  }
}

// Correction pour éviter le blocage après 3 blocs
void updateTiles() {
  for (int i = 0; i < NUM_COLUMNS; i++) {
    if (tiles[i].active) {
      tiles[i].x -= TILE_SPEED; // Déplacement des tuiles vers la gauche
      if (tiles[i].x < 0) {
        // Si une tuile atteint la ligne verte sans être validée, désactiver le bloc
        tiles[i].active = false;
        Serial.println("Tuile manquée !");
      }
    }
  }
}

void spawnTile() {
  int row = random(0, NUM_COLUMNS);
  if (!tiles[row].active) {
    tiles[row].x = MATRIX_WIDTH;
    tiles[row].y = row;
    tiles[row].active = true;
  }
}

void spawnTileFromMusic() {
  if (currentNoteIndex < INTRO_SIZE) { // Utilisation de l'intro comme exemple
    MusicNote note;
    getNote(intro, currentNoteIndex, &note);

    // Déterminer la colonne en fonction de la fréquence de la note
    int column = (note.frequency % NUM_COLUMNS);

    if (!tiles[column].active) {
      tiles[column].x = MATRIX_WIDTH;
      tiles[column].y = column;
      tiles[column].active = true;
    }

    currentNoteIndex++;
  } else {
    currentNoteIndex = 0; // Réinitialiser à la première note
  }
}

// Modification pour doubler l'épaisseur des tuiles
void drawTiles() {
  for (int i = 0; i < NUM_COLUMNS; i++) {
    if (tiles[i].active) {
      for (int x = 0; x < TILE_WIDTH; x++) {
        for (int y = 0; y < 2; y++) { // Double l'épaisseur des tuiles
          if (tiles[i].x + x >= 0 && tiles[i].x + x < MATRIX_WIDTH && tiles[i].y * (MATRIX_HEIGHT / NUM_COLUMNS) + y < MATRIX_HEIGHT) {
            ht1632_plot(tiles[i].x + x, tiles[i].y * (MATRIX_HEIGHT / NUM_COLUMNS) + y, GREEN);
          }
        }
      }
    }
  }
}

void validateTile(int row) {
  if (tiles[row].active && tiles[row].x <= 0) {
    tiles[row].active = false; // Désactiver le bloc validé
    score++;
    Serial.print("Score: ");
    Serial.println(score);
  }
}

// Modification pour élargir la ligne de validation à deux colonnes (2e et 3e colonnes)
void drawValidationLine() {
  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    ht1632_plot(1, y, GREEN); // Dessine une ligne verte sur la deuxième colonne
    ht1632_plot(2, y, GREEN); // Dessine une ligne verte sur la troisième colonne
  }
}

// Modification pour que le curseur soit en 2x2 pour correspondre aux blocs
void drawSliderCursor() {
  // Lire la valeur du potentiomètre (slider)
  int sliderValue = analogRead(potPin);

  // Mapper la valeur du slider à la hauteur de la matrice
  int cursorPosition = map(sliderValue, 0, 1023, 0, MATRIX_HEIGHT - 2); // Ajusté pour 2x2

  // Dessiner le curseur rouge en 2x2 sur les colonnes 2 et 3 (ligne verte)
  for (int y = 0; y < 2; y++) {
    ht1632_plot(1, cursorPosition + y, RED);
    ht1632_plot(2, cursorPosition + y, RED);
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
  
  // Initialiser les tuiles
  initializeTiles();
  
#if DEBUG_SERIAL
  Serial.println("Tuiles initialisées, prêt pour animation");
#endif
}

void loop() {
  unsigned long currentTime = millis();

  // Effacer la matrice
  ht1632_clear();

  // Dessiner la ligne de validation
  drawValidationLine();

  // Dessiner le curseur rouge contrôlé par le slider
  drawSliderCursor();

  // Mettre à jour les tuiles
  if (currentTime - lastUpdateTime > 50) {
    updateTiles();
    lastUpdateTime = currentTime;
  }

  // Dessiner les tuiles
  drawTiles();

  // Générer une nouvelle tuile synchronisée avec la musique
  if (currentTime - lastNoteTime > 500) { // Intervalle basé sur la durée des notes
    spawnTileFromMusic();
    lastNoteTime = currentTime;
  }

  // Simuler une validation (par exemple, en appuyant sur un bouton)
  if (digitalRead(2) == HIGH) validateTile(0);
  if (digitalRead(3) == HIGH) validateTile(1);
  if (digitalRead(4) == HIGH) validateTile(2);
  if (digitalRead(5) == HIGH) validateTile(3);

  // Jouer la musique via le buzzer
  playMusic();

  // Synchroniser les notes avec les tuiles
  playMusicWithTiles();

  delay(10);
}