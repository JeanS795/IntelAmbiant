#include "ht1632.h"
#include "menu_music.h"
#include "jeu.h" // Inclure le fichier jeu.h pour accéder aux fonctions du jeu

// Define pins for slider and button
const int sliderPin = A0;  // Analog pin for reading slider position
const int buttonPin = 12;   // Digital pin for button

// Variables to track menu state
int currentSelection = 1;  // Default selection (1, 2, or 3)
boolean buttonPressed = false;
boolean menuActive = true;
int previousSliderPosition = -1;

// Define positions for the three options on screen
const int option1X = 3;   // Left position
const int option2X = 13;  // Center position
const int option3X = 23;  // Right position
const int optionY = 8;    // Vertical position for all options

// Déclarations des fonctions
void drawMenu();
void highlightSelectedOption(int option);
void draw_text(const char* text, int x, int y, int color);
void draw_number(int num, int x, int y, int color);
void draw_char(int x, int y, char c, int color);

void setupMenu() {
  // Initialize serial for debugging
  Serial.begin(9600);
  
  // Initialize display
  ht1632_setup();
  
  // Setup button pin as input with pullup
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Clear the display
  ht1632_clear();
  
  // Draw initial menu
  drawMenu();
}

void loopMenu() {
  if (menuActive) {
    // Play the music sequences in a non-blocking way
    static unsigned long lastNoteTime = 0;
    static int noteIndex = 0;
    static int sequenceIndex = 0;
    static struct MusicNote* currentSequence = intro;
    static int currentSequenceLength = sizeof(intro) / sizeof(intro[0]);

    unsigned long currentTime = millis();
    if (currentTime - lastNoteTime >= 1000 / currentSequence[noteIndex].duration * 1.30) {
      lastNoteTime = currentTime;
      if (currentSequence[noteIndex].pitch == 0) {
        noTone(BUZZER_PIN);
      } else {
        tone(BUZZER_PIN, currentSequence[noteIndex].pitch, 1000 / currentSequence[noteIndex].duration);
      }
      noteIndex++;
      if (noteIndex >= currentSequenceLength) {
        noteIndex = 0;
        sequenceIndex++;
        switch (sequenceIndex) {
          case 1:
            currentSequence = verse;
            currentSequenceLength = sizeof(verse) / sizeof(verse[0]);
            break;
          case 2:
            currentSequence = chorus;
            currentSequenceLength = sizeof(chorus) / sizeof(chorus[0]);
            break;
          case 3:
            currentSequence = hookPart;
            currentSequenceLength = sizeof(hookPart) / sizeof(hookPart[0]);
            break;
          case 4:
            currentSequence = verse;
            currentSequenceLength = sizeof(verse) / sizeof(verse[0]);
            sequenceIndex = 0; // Loop back to the beginning
            break;
        }
      }
    }

    // Read slider position to determine selection
    int sliderValue = analogRead(sliderPin);
    
    // Map slider value to selection (1, 2, or 3)
    int newSelection = map(sliderValue, 0, 1023, 1, 3);
    
    // If selection changed, update the display
    if (newSelection != currentSelection) {
      currentSelection = newSelection;
      drawMenu();
    }
    
    // Check button press
    if (digitalRead(buttonPin) == LOW && !buttonPressed) {
      buttonPressed = true;
      
      // Highlight selected option
      highlightSelectedOption(currentSelection);
      
      // Wait a moment to show the selection
      delay(500);
      
      // Transition to the game if level 1 is selected
      if (currentSelection == 1) {
        menuActive = false;
        currentMelody = 0; // Sélectionner la première mélodie
        setupGame(); // Initialiser le jeu
        while (true) {
          loopGame(); // Boucle du jeu
        }
      }
      
      // Reset button state
      buttonPressed = false;
    }
    
    // Reset button state if released
    if (digitalRead(buttonPin) == HIGH) {
      buttonPressed = false;
    }
  }
  
  // Small delay to prevent too rapid readings
  delay(50);
}

void drawMenu() {
  // Clear the display first
  ht1632_clear();
  
  // Draw the text "LEVELS" at the top
  draw_text("LEVELS", 10, 0, GREEN);
  
  // Draw the three options (numbers 1, 2, 3)
  // Option 1
  draw_number(1, option1X, optionY, (currentSelection == 1) ? ORANGE : GREEN);
  
  // Option 2
  draw_number(2, option2X, optionY, (currentSelection == 2) ? ORANGE : GREEN);
  
  // Option 3
  draw_number(3, option3X, optionY, (currentSelection == 3) ? ORANGE : GREEN);
}

// Function to draw text on the screen
void draw_text(const char* text, int x, int y, int color) {
  // Simple function to draw text without using ht1632_putchar
  while (*text) {
    draw_char(x, y, *text, color);
    x += 6;  // Move to the next character position
    text++;
  }
}

// Function to draw a character on the screen
void draw_char(int x, int y, char c, int color) {
  // Simple function to draw a character
  // You may need to adjust this depending on your display's specifics
  // Example for drawing 'L', 'E', 'V', 'S'
  switch (c) {
    case 'L':
      for (int i = 0; i < 5; i++) {
        ht1632_plot(x, y + i, color);  // Vertical line
      }
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + i, y + 4, color);  // Bottom horizontal line
      }
      break;
    case 'E':
      for (int i = 0; i < 5; i++) {
        ht1632_plot(x, y + i, color);  // Vertical line
      }
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + i, y, color);  // Top horizontal line
        ht1632_plot(x + i, y + 2, color);  // Middle horizontal line
        ht1632_plot(x + i, y + 4, color);  // Bottom horizontal line
      }
      break;
    case 'V':
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + i, y + i, color);  // Diagonal line
        ht1632_plot(x + 5 - i, y + i, color);  // Diagonal line
      }
      break;
    case 'S':
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + i, y, color);  // Top horizontal line
        ht1632_plot(x + i, y + 2, color);  // Middle horizontal line
        ht1632_plot(x + i, y + 4, color);  // Bottom horizontal line
      }
      ht1632_plot(x, y + 1, color);  // Top left vertical line
      ht1632_plot(x + 2, y + 3, color);  // Bottom right vertical line
      break;
    // Add cases for other characters as needed
  }
}

void highlightSelectedOption(int option) {
  // Highlight the selected option by making it flash
  for (int i = 0; i < 3; i++) {
    if (option == 1) {
      draw_number(1, option1X, optionY, RED);
    } else if (option == 2) {
      draw_number(2, option2X, optionY, RED);
    } else if (option == 3) {
      draw_number(3, option3X, optionY, RED);
    }
    delay(100);
    
    drawMenu();  // This will restore the normal display
    delay(100);
  }
}

// Function to draw a number on the screen
void draw_number(int num, int x, int y, int color) {
  // Draw a 7x7 square around the number
  for (int i = 0; i < 7; i++) {
    ht1632_plot(x - 1, y - 1 + i, color);      // Left vertical line
    ht1632_plot(x + 5, y - 1 + i, color);      // Right vertical line
    ht1632_plot(x - 1 + i, y - 1, color);      // Top horizontal line
    ht1632_plot(x - 1 + i, y + 5, color);      // Bottom horizontal line
  }

  // Draw the number inside the square
  switch (num) {
    case 1:
      for (int i = 0; i < 5; i++) {
        ht1632_plot(x + 2, y + i, color);  // Vertical line for 1
      }
      ht1632_plot(x + 1, y + 1, color);
      break;
    case 2:
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + 1 + i, y, color);      // Top horizontal
        ht1632_plot(x + 1 + i, y + 2, color);  // Middle horizontal
        ht1632_plot(x + 1 + i, y + 4, color);  // Bottom horizontal
      }
      ht1632_plot(x + 3, y + 1, color);    // Top right vertical
      ht1632_plot(x + 1, y + 3, color);    // Bottom left vertical
      break;
    case 3:
      for (int i = 0; i < 3; i++) {
        ht1632_plot(x + 1 + i, y, color);      // Top horizontal
        ht1632_plot(x + 1 + i, y + 2, color);  // Middle horizontal
        ht1632_plot(x + 1 + i, y + 4, color);  // Bottom horizontal
      }
      ht1632_plot(x + 3, y + 1, color);    // Top right vertical
      ht1632_plot(x + 3, y + 3, color);    // Bottom right vertical
      break;
  }
}
