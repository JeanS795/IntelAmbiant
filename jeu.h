// Paramètres du jeu
#define BARRE_X 2         // Position X de la barre (3ème pixel depuis la gauche)
#define BARRE_WIDTH 2     // Épaisseur de la barre (2 pixels)
#define MIN_NOTE_WIDTH 3  // Largeur minimum des notes
#define NOTE_HEIGHT 2     // Hauteur des notes
#define MAX_NOTES 15      // Nombre maximum de notes à l'écran
#define SPEED 150         // Vitesse en ms (plus petit = plus rapide)
#define X_SIZE 32         // Largeur de la matrice (axe horizontal)
#define Y_SIZE 16         // Hauteur de la matrice (axe vertical)

// Broche du bouton et du slider
#define BUTTON_PIN 3      // Broche du bouton
#define SLIDER_PIN A0     // Broche du potentiomètre analogique

// Couleurs pour HT1632
#define COLOR_BG BLACK    // Fond noir
#define COLOR_BARRE GREEN // Barre verte
#define COLOR_NOTE ORANGE // Notes oranges
#define COLOR_CURSOR RED  // Curseur rouge
#define COLOR_HIT ORANGE  // Orange pour note touchée

// Structure pour les notes
struct Note {
  int x;              // Position X
  int y;              // Position Y
  int width;          // Largeur de la note (nombre de pixels)
  bool active;        // Note active ou non
  bool hit;           // Note touchée ou non
  int hitPosition;    // Position où la note a été touchée (-1 si pas touchée)
};

Note notes[MAX_NOTES];
int score = 0;
int totalNotes = 0;
unsigned long lastMoveTime = 0;
int nextNoteIndex = 0;
unsigned long nextNoteTime = 0;

void setupGame();
void loopGame();
void addNoteFromFrequency(int frequency, int duration);
void addNote(int y, int duration);
void moveNotes();
void drawNotes();
void checkCollisions(int cursorY, boolean buttonPressed);

void setupGame() {
  Serial.begin(9600);
  
  // Initialisation du module HT1632
  ht1632_setup();
  
  // Configuration des broches
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialisation des notes
  for (int i = 0; i < MAX_NOTES; i++) {
    notes[i].active = false;
    notes[i].hit = false;
    notes[i].hitPosition = -1;
  }
  
  // Affichage d'un écran de démarrage
  ht1632_clear();
  
  // Bordure de l'écran
  for (int i = 0; i < X_SIZE; i++) {
    ht1632_plot(i, 0, GREEN);
    ht1632_plot(i, Y_SIZE-1, GREEN);
  }
  for (int j = 0; j < Y_SIZE; j++) {
    ht1632_plot(0, j, GREEN);
    ht1632_plot(X_SIZE-1, j, GREEN);
  }
  
  // Affichage du texte "GO!" au centre
  // G
  ht1632_plot(14, 6, ORANGE);
  ht1632_plot(15, 6, ORANGE);
  ht1632_plot(13, 7, ORANGE);
  ht1632_plot(13, 8, ORANGE);
  ht1632_plot(13, 9, ORANGE);
  ht1632_plot(14, 9, ORANGE);
  ht1632_plot(15, 9, ORANGE);
  ht1632_plot(15, 8, ORANGE);
  
  // O
  ht1632_plot(17, 6, ORANGE);
  ht1632_plot(18, 6, ORANGE);
  ht1632_plot(19, 6, ORANGE);
  ht1632_plot(17, 7, ORANGE);
  ht1632_plot(19, 7, ORANGE);
  ht1632_plot(17, 8, ORANGE);
  ht1632_plot(19, 8, ORANGE);
  ht1632_plot(17, 9, ORANGE);
  ht1632_plot(18, 9, ORANGE);
  ht1632_plot(19, 9, ORANGE);
  
  delay(2000);
  ht1632_clear();
  
  // Initialiser le temps de la prochaine note
  nextNoteTime = millis() + 1000; // Commence après 1 seconde
}

void loopGame() {
  // Effacer uniquement les zones nécessaires
  for (int x = BARRE_X; x < BARRE_X + BARRE_WIDTH; x++) {
    for (int y = 0; y < Y_SIZE; y++) {
      ht1632_plot(x, y, COLOR_BG);
    }
  }
  
  // Dessiner la barre à gauche
  for (int x = BARRE_X; x < BARRE_X + BARRE_WIDTH; x++) {
    for (int y = 0; y < Y_SIZE; y++) {
      ht1632_plot(x, y, COLOR_BARRE);
    }
  }
  
  // Lire la position du slider et dessiner le curseur
  int sliderValue = analogRead(SLIDER_PIN);
  int cursorY = map(sliderValue, 0, 1023, 0, Y_SIZE - NOTE_HEIGHT);
  ht1632_plot(BARRE_X, cursorY, COLOR_CURSOR);
  ht1632_plot(BARRE_X + 1, cursorY, COLOR_CURSOR);
  ht1632_plot(BARRE_X, cursorY + 1, COLOR_CURSOR);
  ht1632_plot(BARRE_X + 1, cursorY + 1, COLOR_CURSOR);
  
  // Bouton appuyé ?
  boolean buttonPressed = !digitalRead(BUTTON_PIN);
  
  // Ajouter une nouvelle note si le temps est venu
  unsigned long currentTime = millis();
  if (currentTime >= nextNoteTime) {
    addNote(melodies[currentMelody][nextNoteIndex].position, melodies[currentMelody][nextNoteIndex].duration);
    
    // Préparer la prochaine note
    nextNoteIndex = (nextNoteIndex + 1) % melodyLengths[currentMelody];
    
    // Temps variable entre les notes (entre 500ms et 1500ms)
    nextNoteTime = currentTime + random(500, 1500);
  }
  
  // Déplacer et dessiner les notes
  if (currentTime - lastMoveTime >= SPEED) {
    moveNotes();
    lastMoveTime = currentTime;
  }
  
  // Dessiner les notes
  drawNotes();
  
  // Vérifier les collisions avec la barre
  checkCollisions(cursorY, buttonPressed);
  
  // Afficher le score sur la console série
  Serial.print("Score: ");
  Serial.print(score);
  Serial.print("/");
  Serial.println(totalNotes);
  
  delay(10);
}

// Ajouter une nouvelle note avec une durée spécifique
void addNoteFromFrequency(int frequency, int duration) {
  int y = map(frequency, 0, 2000, 0, Y_SIZE - NOTE_HEIGHT); // Adapter la plage de fréquences selon vos besoins
  addNote(y, duration);
}

void addNote(int y, int duration) {
  for (int i = 0; i < MAX_NOTES; i++) {
    if (!notes[i].active) {
      notes[i].x = X_SIZE;  // Commence juste en dehors de l'écran à droite
      notes[i].y = y;
      notes[i].width = duration;  // La largeur dépend de la durée en pixels
      notes[i].active = true;
      notes[i].hit = false;
      notes[i].hitPosition = -1;
      totalNotes++;
      return;
    }
  }
}

// Déplacer toutes les notes actives
void moveNotes() {
  for (int i = 0; i < MAX_NOTES; i++) {
    if (notes[i].active) {
      notes[i].x -= 0.5; // Déplace la note vers la gauche de manière plus fluide
      
      // Supprimer les notes qui sortent complètement de l'écran
      if (notes[i].x + notes[i].width < 0) {
        notes[i].active = false;
      }
    }
  }
}

// Dessiner toutes les notes actives
void drawNotes() {
  for (int i = 0; i < MAX_NOTES; i++) {
    if (notes[i].active) {
      byte noteColor = notes[i].hit ? COLOR_HIT : COLOR_NOTE;
      
      // Pour les notes touchées, on ne dessine que jusqu'à la position touchée
      int endX = notes[i].hit ? notes[i].hitPosition : notes[i].x + notes[i].width;
      
      for (float dx = 0; dx < endX - notes[i].x; dx += 0.5) {
        for (int dy = 0; dy < NOTE_HEIGHT; dy++) {
          int xPos = notes[i].x + dx;
          int yPos = notes[i].y + dy;
          if (xPos >= 0 && xPos < X_SIZE &&
              yPos >= 0 && yPos < Y_SIZE) {
            ht1632_plot(xPos, yPos, noteColor);
          }
        }
      }
    }
  }
}

// Vérifier les collisions avec la barre
void checkCollisions(int cursorY, boolean buttonPressed) {
  for (int i = 0; i < MAX_NOTES; i++) {
    if (notes[i].active && !notes[i].hit) {
      // Vérifier si le début de la note est sur la barre
      if (notes[i].x <= BARRE_X && 
          notes[i].x + notes[i].width >= BARRE_X + BARRE_WIDTH) {
        // Vérifier si le curseur est bien positionné et si le bouton est appuyé
        if (buttonPressed && 
            cursorY >= notes[i].y - 1 && 
            cursorY <= notes[i].y + NOTE_HEIGHT) {
          notes[i].hit = true;
          notes[i].hitPosition = BARRE_X + BARRE_WIDTH;  // On marque la position de collision
          score++;
          
          // Animation de succès
          for (int y = 0; y < Y_SIZE; y++) {
            ht1632_plot(BARRE_X, y, COLOR_HIT);
            ht1632_plot(BARRE_X + 1, y, COLOR_HIT);
          }
          
          // Empêcher la note de passer la ligne verte
          notes[i].x = BARRE_X - notes[i].width;
        }
      }
      
      // Si la note a dépassé la barre sans être touchée, on la considère comme manquée
      if (notes[i].x + notes[i].width < BARRE_X && !notes[i].hit) {
        notes[i].hit = true;  // On marque la note comme "touchée" pour ne pas la compter plusieurs fois
        notes[i].hitPosition = notes[i].x + notes[i].width;  // Mais on ne donne pas de point
      }
    }
  }
}
