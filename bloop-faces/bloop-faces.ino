/**
 * @file    RobotEyes_OLED.ino
 * @brief   Expressive OLED Robot Eyes with RGB LED integration for Arduino Uno
 * @details Implements non-blocking blinking logic, modular drawing functions for 
 *          robot emotions, and synchronized RGB LED color states.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <SH1106.h> // Or <Adafruit_SSD1306.h> depending on your module

// --- Display Configuration ---
SH1106 display;

// --- RGB LED Configuration ---
#define RGB_RED_PIN   9
#define RGB_GREEN_PIN 10
#define RGB_BLUE_PIN  11

// --- Global Blink State Machine ---
unsigned long lastBlinkMs = 0;
unsigned long nextBlinkDelay = 2000;
bool isBlinking = false;
const unsigned long BLINK_DURATION = 150; ///< Duration the eyes remain closed (ms)

// --- Global Enum Emotion State --- 
enum EmotionState {
  IDLE, NEUTRAL, ANGRY, CONFUSED, HAPPY, EXCITED, SAD, SHOCKED
};
EmotionState currentEmotion = IDLE;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);
  
  // Initialize RGB LED pins
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setRGB(0, 0, 0); // Turn off LED on boot

  Serial.print("Initializing display... ");
  // Initialize I2C display (Defaults to A4 for SDA, A5 for SCL)
  display.begin();
  display.clearDisplay();
  display.display();
  Serial.println("Ready.");

  // Seed random number generator with analog noise for natural blinking
  randomSeed(analogRead(A0)); 
}

void loop() {
  // Maintain the non-blocking blink timer
  updateBlinkState();

  // Check for incoming Serial commands from Python
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove invisible \r or \n characters

    if (command == "idle") currentEmotion = IDLE;
    else if (command == "neutral") currentEmotion = NEUTRAL;
    else if (command == "angry") currentEmotion = ANGRY;
    else if (command == "confused") currentEmotion = CONFUSED;
    else if (command == "happy") currentEmotion = HAPPY;
    else if (command == "excited") currentEmotion = EXCITED;
    else if (command == "sad") currentEmotion = SAD;
    else if (command == "shocked") currentEmotion = SHOCKED;
  }

  // Clear the buffer before rendering the new frame
  display.clearDisplay();

  // Render the currently active emotion
  switch (currentEmotion) {
    case IDLE: renderIdle(); break;
    case NEUTRAL: renderNeutral(); break;
    case ANGRY: renderAngry(); break;
    case CONFUSED: renderConfused(); break;
    case HAPPY: renderHappy(); break;
    case EXCITED: renderExcited(); break;
    case SAD: renderSad(); break;
    case SHOCKED: renderShocked(); break;
  }

  // Push the buffer to the OLED screen
  display.display();
}

/**
 * @brief Sets the color of the RGB LED.
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 */
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_RED_PIN, r);
  analogWrite(RGB_GREEN_PIN, g);
  analogWrite(RGB_BLUE_PIN, b);
}

/**
 * @brief Updates the global blinking state variables.
 * @note  Call this once per loop. Generates a randomized delay between blinks.
 */
void updateBlinkState() {
  unsigned long now = millis();
  if (isBlinking) {
    if (now - lastBlinkMs > BLINK_DURATION) {
      isBlinking = false;
      lastBlinkMs = now;
      nextBlinkDelay = random(1500, 4500); 
    }
  } else {
    if (now - lastBlinkMs > nextBlinkDelay) {
      isBlinking = true;
      lastBlinkMs = now;
    }
  }
}

// ==========================================
//          RENDER FUNCTIONS
// ==========================================

/**
 * @brief Renders the default resting state.
 */
void renderNeutral() {
  setRGB(50, 50, 50); // Soft White

  if (isBlinking) {
    display.fillRect(18, 32, 28, 4, WHITE);
    display.fillRect(82, 32, 28, 4, WHITE);
  } else {
    display.fillRoundRect(18, 14, 28, 36, 8, WHITE);
    display.fillRoundRect(82, 14, 28, 36, 8, WHITE);
  }
}

/**
 * @brief Renders an angry or aggressive state.
 */
void renderAngry() {
  setRGB(255, 0, 0); // Solid Red

  if (isBlinking) {
    display.fillRect(18, 32, 28, 4, WHITE);
    display.fillRect(82, 32, 28, 4, WHITE);
  } else {
    display.fillRoundRect(18, 14, 28, 36, 8, WHITE);
    display.fillRoundRect(82, 14, 28, 36, 8, WHITE);
    
    display.fillTriangle(14, 10, 48, 6, 50, 26, BLACK);
    display.fillTriangle(78, 26, 80, 6, 114, 10, BLACK);

    display.drawLine(54, 14, 60, 22, WHITE);
    display.drawLine(55, 14, 61, 22, WHITE); 
    display.drawLine(74, 14, 68, 22, WHITE);
    display.drawLine(73, 14, 67, 22, WHITE); 
  }
}

/**
 * @brief Renders a confused or processing state.
 */
void renderConfused() {
  setRGB(128, 0, 128); // Purple/Magenta

  if (isBlinking) {
    display.fillRect(18, 32, 28, 4, WHITE);
    display.fillRect(82, 32, 28, 4, WHITE);
  } else {
    display.fillRoundRect(18, 14, 28, 36, 8, WHITE);
    display.fillTriangle(10, 10, 40, 14, 10, 24, BLACK); 

    display.fillRoundRect(82, 26, 28, 12, 4, WHITE);

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(110, 8);
    display.print("?");
  }
}

/**
 * @brief Renders a beaming, joyful state.
 */
void renderHappy() {
  setRGB(0, 255, 50); // Cheerful Green

  if (isBlinking) {
    display.fillRect(18, 30, 28, 4, WHITE);
    display.fillRect(82, 30, 28, 4, WHITE);
  } else {
    display.fillRoundRect(18, 14, 28, 36, 8, WHITE);
    display.fillRoundRect(82, 14, 28, 36, 8, WHITE);

    display.fillCircle(32, 50, 18, BLACK); 
    display.fillCircle(96, 50, 18, BLACK); 

    display.fillRoundRect(54, 38, 20, 14, 6, WHITE); 
    display.fillRect(52, 36, 24, 6, BLACK);          

    display.fillRoundRect(12, 46, 8, 4, 2, WHITE);
    display.fillRoundRect(108, 46, 8, 4, 2, WHITE);
  }
}

/**
 * @brief Renders an intensely excited or amazed state.
 */
void renderExcited() {
  setRGB(255, 150, 0); // Bright Yellow-Orange (Gold)

  if (isBlinking) {
    display.drawLine(20, 24, 38, 32, WHITE);
    display.drawLine(20, 25, 38, 33, WHITE); 
    display.drawLine(38, 32, 20, 40, WHITE);
    display.drawLine(38, 33, 20, 41, WHITE); 
    
    display.drawLine(108, 24, 90, 32, WHITE);
    display.drawLine(108, 25, 90, 33, WHITE); 
    display.drawLine(90, 32, 108, 40, WHITE);
    display.drawLine(90, 33, 108, 41, WHITE); 
  } else {
    display.fillTriangle(32, 10, 26, 32, 38, 32, WHITE); 
    display.fillTriangle(32, 54, 26, 32, 38, 32, WHITE); 
    display.fillTriangle(12, 32, 32, 26, 32, 38, WHITE); 
    display.fillTriangle(52, 32, 32, 26, 32, 38, WHITE); 
    
    display.drawLine(46, 14, 46, 22, WHITE);
    display.drawLine(42, 18, 50, 18, WHITE);

    display.fillTriangle(96, 10, 90, 32, 102, 32, WHITE); 
    display.fillTriangle(96, 54, 90, 32, 102, 32, WHITE); 
    display.fillTriangle(76, 32, 96, 26, 96, 38, WHITE); 
    display.fillTriangle(116, 32, 96, 26, 96, 38, WHITE); 
    
    display.drawLine(110, 14, 110, 22, WHITE);
    display.drawLine(106, 18, 114, 18, WHITE);
  }
}

/**
 * @brief Renders a sad or disappointed state.
 */
void renderSad() {
  setRGB(0, 0, 255); // Deep Blue

  if (isBlinking) {
    display.fillRect(18, 32, 28, 4, WHITE);
    display.fillRect(82, 32, 28, 4, WHITE);
  } else {
    display.fillRoundRect(18, 14, 28, 36, 8, WHITE);
    display.fillRoundRect(82, 14, 28, 36, 8, WHITE);

    display.fillTriangle(10, 8, 38, 8, 10, 28, BLACK);    
    display.fillTriangle(90, 8, 118, 8, 118, 28, BLACK);  

    display.fillCircle(96, 56, 4, WHITE);                      
    display.fillTriangle(92, 56, 100, 56, 96, 48, WHITE);      
  }
}

/**
 * @brief Renders a shocked, stunned, or terrified state.
 */
void renderShocked() {
  setRGB(255, 255, 255); // Bright White (Flash)

  if (isBlinking) {
    display.fillRect(26, 20, 12, 4, WHITE);
    display.fillRect(90, 20, 12, 4, WHITE);
  } else {
    display.fillCircle(32, 22, 7, WHITE);
    display.fillCircle(96, 22, 7, WHITE);

    display.drawRoundRect(52, 34, 24, 26, 10, WHITE);
    display.drawRoundRect(53, 35, 22, 24, 9, WHITE); 

    display.drawLine(20, 4, 20, 12, WHITE);
    display.drawLine(32, 2, 32, 9, WHITE);
    display.drawLine(44, 4, 44, 12, WHITE);
    
    display.drawLine(84, 4, 84, 12, WHITE);
    display.drawLine(96, 2, 96, 9, WHITE);
    display.drawLine(108, 4, 108, 12, WHITE);
  }
}

/**
 * @brief Renders an idle state with slower, more realistic glances.
 */
void renderIdle() {
  setRGB(0, 150, 200); // Calming Cyan/Light Blue

  // --- Static Timers for Eyes ---
  static unsigned long lastEyeActionMs = 0;
  static unsigned long eyeActionDelay = 3000;
  static int eyeOffsetX = 0;
  static int eyeOffsetY = 0;

  // --- Static Timers for Mouth ---
  static unsigned long lastMouthActionMs = 0;
  static unsigned long mouthActionDelay = 5000;
  static int currentMouth = 0; // 0 = none, 1 = w, 2 = yawn, 3 = dash

  unsigned long now = millis();

  // ==========================================
  // 1. EYE MOVEMENT LOGIC
  // ==========================================
  if (now - lastEyeActionMs > eyeActionDelay) {
    lastEyeActionMs = now;
    
    int action = random(0, 10); 
    
    if (action <= 5) { 
      eyeOffsetX = 0;
      eyeOffsetY = 0;
      eyeActionDelay = random(3000, 7000); 
    } 
    else if (action == 6) { 
      eyeOffsetX = -10;
      eyeOffsetY = 0;
      eyeActionDelay = random(1500, 3000); 
    } 
    else if (action == 7) { 
      eyeOffsetX = 10;
      eyeOffsetY = 0;
      eyeActionDelay = random(1500, 3000); 
    } 
    else if (action == 8) { 
      eyeOffsetX = 0;
      eyeOffsetY = -8;
      eyeActionDelay = random(2000, 4000); 
    } 
    else { 
      eyeOffsetX = 0;
      eyeOffsetY = 8;
      eyeActionDelay = random(2000, 4000); 
    }
  }

  // ==========================================
  // 2. MOUTH MOVEMENT LOGIC
  // ==========================================
  if (now - lastMouthActionMs > mouthActionDelay) {
    lastMouthActionMs = now;
    
    if (currentMouth == 0) {
      currentMouth = random(1, 4); 
      mouthActionDelay = random(2000, 4000); 
    } else {
      currentMouth = 0;
      mouthActionDelay = random(5000, 12000); 
    }
  }

  // ==========================================
  // 3. DRAW EYES
  // ==========================================
  if (isBlinking) {
    display.fillRect(18 + eyeOffsetX, 32 + eyeOffsetY, 28, 4, WHITE);
    display.fillRect(82 + eyeOffsetX, 32 + eyeOffsetY, 28, 4, WHITE);
  } else {
    if (currentMouth == 2) {
      display.fillRoundRect(18 + eyeOffsetX, 24 + eyeOffsetY, 28, 16, 6, WHITE);
      display.fillRoundRect(82 + eyeOffsetX, 24 + eyeOffsetY, 28, 16, 6, WHITE);
    } else {
      display.fillRoundRect(18 + eyeOffsetX, 14 + eyeOffsetY, 28, 36, 8, WHITE);
      display.fillRoundRect(82 + eyeOffsetX, 14 + eyeOffsetY, 28, 36, 8, WHITE);
    }
  }

  // ==========================================
  // 4. DRAW MOUTH (With Parallax)
  // ==========================================
  int mouthX = 64 + (eyeOffsetX / 2);
  int mouthY = 56 + (eyeOffsetY / 2);

  if (currentMouth == 1) {
    display.drawLine(mouthX - 6, mouthY - 2, mouthX - 3, mouthY + 2, WHITE);
    display.drawLine(mouthX - 3, mouthY + 2, mouthX, mouthY - 1, WHITE);
    display.drawLine(mouthX, mouthY - 1, mouthX + 3, mouthY + 2, WHITE);
    display.drawLine(mouthX + 3, mouthY + 2, mouthX + 6, mouthY - 2, WHITE);
  } 
  else if (currentMouth == 2) {
    display.fillRoundRect(mouthX - 6, mouthY - 2, 12, 12, 4, WHITE);
    display.fillRoundRect(mouthX - 4, mouthY, 8, 8, 2, BLACK); 
  } 
  else if (currentMouth == 3) {
    display.drawLine(mouthX - 4, mouthY, mouthX + 4, mouthY, WHITE);
    display.drawLine(mouthX - 4, mouthY + 1, mouthX + 4, mouthY + 1, WHITE);
  }
}