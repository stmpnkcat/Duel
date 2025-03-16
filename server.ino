#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

const char* ssid = "Battle";
const char* password = "123456789";
const char* PARAM_INPUT_1 = "type";
const char* PARAM_INPUT_2 = "var";

class Bullet {
  public:
    int posX;
    int posY;
    int dir;
};

int x, y, z;

int posX = SCREEN_WIDTH / 2;
int posY = SCREEN_HEIGHT * 3 / 4;

bool is_start = false;
bool is_dead = false;

int counter = 0;

int num_bullets = 0;
Bullet bullets[200];
int dir[8] = {7,3,4,2,1,0,6,5};

const int FIRE_RATE = 5;
const int SPEED = 5;
const int BULLET_SPEED = 10;
const int PLAYER_SIZE = 30;
const int BULLET_WIDTH = 8;
const int BULLET_LENGTH = 20;

int store = 0;
int temp = 0;

AsyncWebServer server(80);

String statusreturn() {
  if(is_dead == true){
    return String("0|1");
  }
  else if(temp != 0){
    return String("1|" + String(temp));
  }
  else{
    return String("0|0");
  }
}

void setup() {

  Serial.begin(115200);

  randomize();

  
  Serial.println("Setting AP (Access Point)…");
  
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.on("/sendStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputType;
    String inputVar;
    
    if (request->hasParam(PARAM_INPUT_1)&& request->hasParam(PARAM_INPUT_2)) {
      temp = store;
      store = 0;
      inputType = request->getParam(PARAM_INPUT_1)->value();
      inputVar = request->getParam(PARAM_INPUT_2)->value();

      if (inputType.equals("1")) {

        create_bullet(inputVar.toInt(), 0, -1);

      } else if (inputType.equals("0") && inputVar.equals("1")) {

        reset();

         tft.fillScreen(TFT_WHITE);
      
        int centerX = SCREEN_WIDTH / 2;
        int centerY = SCREEN_HEIGHT / 2;

        tft.fillScreen(TFT_DARKGREY);
        tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        tft.fillRect(0, (SCREEN_HEIGHT/2) - 30, SCREEN_WIDTH, 60, TFT_DARKGREEN);

        tft.drawCentreString("YOU WIN!", centerX, centerY-20, FONT_SIZE);
        tft.drawCentreString("press anywhere to play again.", centerX, centerY, FONT_SIZE);

      }

    }
    else {
      inputType = "No message sent";
    }
    Serial.println("Got board:"+inputType);
     request->send_P(200, "text/plain", statusreturn().c_str());
  });
 
  // Start server
  server.begin();
  Serial.println("Starting Battle Server v…"+String(ssid));

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  touchscreen.setRotation(1);

  // Start the tft display
  tft.init();
  // Set the TFT display rotation in landscape mode
  tft.setRotation(1);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Set X and Y coordinates for center of display
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  tft.drawCentreString("Infinite Duel!", centerX, centerY - 30, FONT_SIZE);
  tft.drawCentreString("Touch screen to start", centerX, centerY, FONT_SIZE);

  
}

void randomize() {
  int hi;
  int hi2;
  int hi3;
  int hi4;
  for(int i = 0; i<100;i++){
    hi2 = random(7);
    hi = dir[hi2];
    hi3 = random(7);
    hi4 = dir[hi3];
    dir[hi2] = hi4;
    dir[hi3] = hi;
  }
}

void move() {

  if(x < SCREEN_WIDTH / 3 && y < SCREEN_HEIGHT / 3) move_player(dir[0]);
  else if (x > SCREEN_WIDTH * 2 / 3 && y < SCREEN_HEIGHT / 3) move_player(dir[2]);
  else if (x > SCREEN_WIDTH * 2 / 3 && y > SCREEN_HEIGHT * 2 / 3) move_player(dir[4]);
  else if (x < SCREEN_WIDTH / 3 && y > SCREEN_HEIGHT * 2 / 3) move_player(dir[6]);
  else if (y < SCREEN_HEIGHT / 3) move_player(dir[1]);
  else if (x > SCREEN_WIDTH * 2 / 3) move_player(dir[3]);
  else if (y > SCREEN_HEIGHT * 2 / 3) move_player(dir[5]);
  else if (x < SCREEN_WIDTH / 3) move_player(dir[7]);

}

void move_player(int dir) {
  if (dir == 0) {
    posX = min(posX + SPEED, SCREEN_WIDTH - PLAYER_SIZE);
    posY = max(posY - SPEED, 0);
  } else if (dir == 1) {
    posY = max(posY - SPEED, 0);
  } else if (dir == 2) {
    posX = max(posX - SPEED, 0);
    posY = max(posY - SPEED, 0);
  } else if (dir == 3) {
    posX = max(posX - SPEED, 0);
  } else if (dir == 4) {
    posX = max(posX - SPEED, 0);
    posY = min(posY + SPEED, SCREEN_HEIGHT - PLAYER_SIZE);
  } else if (dir == 5) {
    posY = min(posY + SPEED, SCREEN_HEIGHT - PLAYER_SIZE);
  } else if (dir == 6) {
    posX = min(posX + SPEED, SCREEN_WIDTH - PLAYER_SIZE);
    posY = min(posY + SPEED, SCREEN_HEIGHT - PLAYER_SIZE);
  } else if (dir == 7) {
    posX = min(posX + SPEED, SCREEN_WIDTH - PLAYER_SIZE);
  }
}

void create_bullet(int posX, int posY, int dir) {

  Bullet new_bullet;
  new_bullet.posX = posX;
  new_bullet.posY = posY;
  new_bullet.dir = dir;
  bullets[num_bullets] = new_bullet;
  num_bullets++;

}

void shoot() {

  if (counter > FIRE_RATE) {
    create_bullet(posX + 10, posY - BULLET_LENGTH - 1, 1);
    counter = 0;
  }

}

void draw(int touchX, int touchY, int touchZ) {
  
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  tft.fillRect(posX, posY, PLAYER_SIZE, PLAYER_SIZE, TFT_BLUE);

  for (int i = 0; i < num_bullets; i++) {
    tft.fillRect(bullets[i].posX, bullets[i].posY, BULLET_WIDTH, BULLET_LENGTH, TFT_RED);
    bullets[i].posY -= bullets[i].dir * BULLET_SPEED;
  }

}

void check() {

  for (int i = 0; i < num_bullets; i++) {
    Bullet bullet = bullets[i];
    Serial.println(bullet.posY);
    if (posX > bullet.posX - PLAYER_SIZE && posX < bullet.posX + BULLET_WIDTH && posY > bullet.posY - PLAYER_SIZE && posY < bullet.posY + BULLET_LENGTH) {
      is_dead = true;
    } else if (bullet.posY > SCREEN_HEIGHT * 2) {
      Serial.println("failed");
      bullets[i].posY = 0;
    } else if (bullet.posY <= BULLET_LENGTH * - 1 && bullet.posY > BULLET_LENGTH * - 1 - BULLET_SPEED){
      bullets[i].posY = SCREEN_HEIGHT * 2 - 1;
      store = bullet.posX;
    }

  }
  
}

void reset() {

  posX = SCREEN_WIDTH / 2;
  posY = SCREEN_HEIGHT * 3 / 4;
  counter = 0;
  is_start = false;
  memset(bullets, 0, sizeof(bullets));

}

void loop() {

  if (!is_start && touchscreen.tirqTouched() && touchscreen.touched()) {
    is_start = true;
  } else if (!is_start) return;

  if (is_dead) {

    reset();

    tft.fillScreen(TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_MAROON);
    tft.fillRect(0, (SCREEN_HEIGHT/2) - 30, SCREEN_WIDTH, 60, TFT_MAROON);
    
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    tft.drawCentreString("YOU LOST!", centerX, centerY-30, FONT_SIZE);
    tft.drawCentreString("press anywhere to play again.", centerX, centerY, FONT_SIZE);

    return;

  }
  
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    
    TS_Point p = touchscreen.getPoint();
    
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    move();

    if(x > SCREEN_WIDTH / 3 && x < SCREEN_WIDTH * 2 / 3 && y > SCREEN_HEIGHT / 3 && y < SCREEN_HEIGHT * 2 / 3) {
      shoot();
    }

  }

  check();
  draw(x, y, z);

  delay(100);
  counter++;

}
