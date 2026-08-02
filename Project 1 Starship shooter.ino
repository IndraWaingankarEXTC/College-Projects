#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

//---------------- PINS ----------------//
const byte BTN_LEFT  = 2;
const byte BTN_FIRE  = 3;
const byte BTN_RIGHT = 4;

//------------- SCREEN SIZE ------------//
const int SCREEN_W = 128;
const int SCREEN_H = 64;

//------------- PLAYER -----------------//
int shipX = 60;
const int shipY = 56;
const int shipW = 7;
const int shipH = 7;

bool playerBullet = false;
int bulletX;
int bulletY;

//------------- ENEMIES ----------------//

const byte MAX_ENEMY = 5;

struct Enemy
{
  bool alive;
  int x;
  int y;
  bool bullet;
  int bulletX;
  int bulletY;
};

Enemy enemy[MAX_ENEMY];

//------------- EXPLOSIONS ------------//

struct Explosion
{
  bool active;
  int x;
  int y;
  byte radius;
};

const byte MAX_EXP = 8;
Explosion explosions[MAX_EXP];

//------------- GAME ------------------//

bool gameOver = false;
unsigned long score = 0;
unsigned long lastSpawn = 0;

//----------- BUTTONS -----------------//

bool leftPressed()
{
  return digitalRead(BTN_LEFT)==LOW;
}

bool rightPressed()
{
  return digitalRead(BTN_RIGHT)==LOW;
}

bool firePressed()
{
  return digitalRead(BTN_FIRE)==LOW;
}

//----------- SHIP DRAW ---------------//

void drawShip()
{
  display.drawTriangle(
      shipX,
      shipY,
      shipX-3,
      shipY+6,
      shipX+3,
      shipY+6);

  display.drawLine(shipX-2,shipY+6,shipX-2,shipY+7);
  display.drawLine(shipX+2,shipY+6,shipX+2,shipY+7);
}
//------------ DRAW ENEMIES ------------//

void drawEnemies()
{
  for(byte i=0;i<MAX_ENEMY;i++)
  {
    if(enemy[i].alive)
    {
      display.drawBox(enemy[i].x,enemy[i].y,3,3);
    }

    if(enemy[i].bullet)
    {
      display.drawBox(enemy[i].bulletX,enemy[i].bulletY,1,2);
    }
  }
}

//----------- SPAWN ENEMY -------------//

void spawnEnemy()
{
  if(millis()-lastSpawn<700)
    return;

  lastSpawn=millis();

  for(byte i=0;i<MAX_ENEMY;i++)
  {
    if(!enemy[i].alive)
    {
      enemy[i].alive=true;
      enemy[i].x=random(3,124);
      enemy[i].y=0;
      enemy[i].bullet=false;
      return;
    }
  }
}

//----------- MOVE ENEMIES ------------//

void moveEnemies()
{
  for(byte i=0;i<MAX_ENEMY;i++)
  {
    if(enemy[i].alive)
    {
      enemy[i].y++;

      if(enemy[i].y>SCREEN_H)
      {
        gameOver=true;
      }

      if(random(0,100)==0 && !enemy[i].bullet)
      {
        enemy[i].bullet=true;
        enemy[i].bulletX=enemy[i].x+1;
        enemy[i].bulletY=enemy[i].y+3;
      }
    }

    if(enemy[i].bullet)
    {
      enemy[i].bulletY+=2;

      if(enemy[i].bulletY>SCREEN_H)
      {
        enemy[i].bullet=false;
      }
    }
  }
}

//----------- PLAYER BULLET -----------//

void movePlayerBullet()
{
  if(playerBullet)
  {
    bulletY-=3;

    if(bulletY<0)
    {
      playerBullet=false;
    }

    display.drawBox(bulletX,bulletY,1,2);
  }
}

//----------- SHOOT -------------------//

void shoot()
{
  if(firePressed() && !playerBullet)
  {
    playerBullet=true;
    bulletX=shipX;
    bulletY=shipY-2;
  }
}

//----------- PLAYER MOVE ------------//

void movePlayer()
{
  if(leftPressed())
    shipX--;

  if(rightPressed())
    shipX++;

  if(shipX<0)
    shipX=127;

  if(shipX>127)
    shipX=0;
}
//------------ EXPLOSIONS -------------//

void addExplosion(int x,int y)
{
  for(byte i=0;i<MAX_EXP;i++)
  {
    if(!explosions[i].active)
    {
      explosions[i].active=true;
      explosions[i].x=x;
      explosions[i].y=y;
      explosions[i].radius=1;
      return;
    }
  }
}

void drawExplosions()
{
  for(byte i=0;i<MAX_EXP;i++)
  {
    if(explosions[i].active)
    {
      display.drawCircle(explosions[i].x,explosions[i].y,explosions[i].radius);

      explosions[i].radius++;

      if(explosions[i].radius>4)
        explosions[i].active=false;
    }
  }
}

//---------- COLLISIONS --------------//

void collisions()
{
  // Player bullet vs enemy
  if(playerBullet)
  {
    for(byte i=0;i<MAX_ENEMY;i++)
    {
      if(enemy[i].alive)
      {
        if(bulletX>=enemy[i].x &&
           bulletX<=enemy[i].x+2 &&
           bulletY>=enemy[i].y &&
           bulletY<=enemy[i].y+2)
        {
          enemy[i].alive=false;
          playerBullet=false;
          addExplosion(enemy[i].x+1,enemy[i].y+1);
          score++;
        }
      }
    }
  }

  // Enemy bullet vs player
  for(byte i=0;i<MAX_ENEMY;i++)
  {
    if(enemy[i].bullet)
    {
      if(enemy[i].bulletX>=shipX-3 &&
         enemy[i].bulletX<=shipX+3 &&
         enemy[i].bulletY>=shipY &&
         enemy[i].bulletY<=shipY+7)
      {
        addExplosion(shipX,shipY);
        gameOver=true;
      }
    }
  }

  // Enemy touching player
  for(byte i=0;i<MAX_ENEMY;i++)
  {
    if(enemy[i].alive)
    {
      if(enemy[i].x+2>=shipX-3 &&
         enemy[i].x<=shipX+3 &&
         enemy[i].y+2>=shipY)
      {
        addExplosion(shipX,shipY);
        gameOver=true;
      }
    }
  }

  // Bullet vs Bullet
  if(playerBullet)
  {
    for(byte i=0;i<MAX_ENEMY;i++)
    {
      if(enemy[i].bullet)
      {
        if(abs(bulletX-enemy[i].bulletX)<=1 &&
           abs(bulletY-enemy[i].bulletY)<=2)
        {
          addExplosion(bulletX,bulletY);

          playerBullet=false;
          enemy[i].bullet=false;
        }
      }
    }
  }
}

//---------- GAME OVER SCREEN --------//

void drawGameOver()
{
  display.clearBuffer();

  display.setFont(u8g2_font_6x12_tf);

  display.drawStr(30,20,"GAME OVER");

  display.setCursor(25,40);
  display.print("Score: ");
  display.print(score);

  display.sendBuffer();

  while(1);
}
//------------ SETUP ----------------//

void setup()
{
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_FIRE, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  display.begin();

  randomSeed(analogRead(A0));

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(22,30,"STARSHIP SHOOTER");
  display.sendBuffer();

  delay(1500);

  for(byte i=0;i<MAX_ENEMY;i++)
  {
    enemy[i].alive=false;
    enemy[i].bullet=false;
  }

  for(byte i=0;i<MAX_EXP;i++)
  {
    explosions[i].active=false;
  }
}

//------------- LOOP ----------------//

void loop()
{
  if(gameOver)
  {
    drawGameOver();
  }

  movePlayer();

  shoot();

  spawnEnemy();

  moveEnemies();

  movePlayerBullet();

  collisions();

  display.clearBuffer();

  drawShip();

  drawEnemies();

  drawExplosions();

  if(playerBullet)
  {
    display.drawBox(bulletX,bulletY,1,2);
  }

  display.setFont(u8g2_font_5x8_tf);

  display.setCursor(0,8);
  display.print("Score:");

  display.setCursor(34,8);
  display.print(score);

  display.sendBuffer();

  delay(25);
}
