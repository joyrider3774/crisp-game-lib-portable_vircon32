#include "../cglp.h"

int* blazethrustTitle = "BLAZE THRUST";
int* blazethrustDescription = "[Hold]\n Thrust upward";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] blazethrustCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int blazethrustCharactersCount = 0;

Options blazethrustOptions = {100, 100, 1, false};

struct BlazethrustFlame {
  Vector pos;
  Vector velocity;
  float size;
  float windEffect;
};
BlazethrustFlame blazethrustFlame;

struct BlazethrustWindCurrent {
  Vector pos;
  float width;
  float force;
  bool isAlive;
};
// Up to 20 are created at init, replenished up to a soft cap of 10 more
// while scrolling - sized with headroom above the initial population.
#define BLAZETHRUST_MAX_WIND_CURRENT_COUNT 40
BlazethrustWindCurrent[BLAZETHRUST_MAX_WIND_CURRENT_COUNT] blazethrustWindCurrents;
int blazethrustWindCurrentIndex;

struct BlazethrustWindParticle {
  Vector pos;
  Vector velocity;
  int age;
  bool isPurple;
  float waveOffset;
  float waveAmplitude;
  bool isAlive;
};
// Sized generously above the ~50 concurrent estimated from spawn chance x lifetime.
#define BLAZETHRUST_MAX_WIND_PARTICLE_COUNT 256
BlazethrustWindParticle[BLAZETHRUST_MAX_WIND_PARTICLE_COUNT] blazethrustWindParticles;
int blazethrustWindParticleIndex;

struct BlazethrustOxygenBubble {
  Vector pos;
  float size;
  float windEffect;
  float fallSpeed;
  bool isAlive;
};
// Hard-capped at 15 concurrent by the spawn gate itself.
#define BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT 16
BlazethrustOxygenBubble[BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT] blazethrustOxygenBubbles;
int blazethrustOxygenBubbleIndex;

float blazethrustScrollOffset;
float blazethrustLastWindY;
float blazethrustNextBubbleDistance;
float blazethrustWindAnimOffset;
int blazethrustBlinkTimer;
int blazethrustMultiplier;
float blazethrustHoldTime;

void blazethrustUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&blazethrustFlame.pos, 50, 90);
    vectorSet(&blazethrustFlame.velocity, 0, 0);
    blazethrustFlame.size = 9;
    blazethrustFlame.windEffect = 0.1;
    INIT_UNALIVED_ARRAY_FAST(blazethrustWindCurrents);
    blazethrustWindCurrentIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(blazethrustOxygenBubbles);
    blazethrustOxygenBubbleIndex = 0;
    blazethrustScrollOffset = 0;
    blazethrustLastWindY = -100;
    blazethrustNextBubbleDistance = rnd(10, 40);
    blazethrustWindAnimOffset = 0;
    INIT_UNALIVED_ARRAY_FAST(blazethrustWindParticles);
    blazethrustWindParticleIndex = 0;
    blazethrustBlinkTimer = 0;
    blazethrustMultiplier = 1;
    blazethrustHoldTime = 0;

    float currentY = 0;
    TIMES(20, wi) {
      float width = rnd(20, 40);
      ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, blazethrustWindCurrentIndex, BlazethrustWindCurrent, w);
      vectorSet(&w->pos, 0, currentY);
      w->width = width;
      w->force = rnd(0.1, 0.3) * RNDPM();
      w->isAlive = true;
      blazethrustWindCurrentIndex = cgl_wrap(blazethrustWindCurrentIndex + 1, 0, BLAZETHRUST_MAX_WIND_CURRENT_COUNT);
      currentY += width + rnd(20, 40);
      blazethrustLastWindY = currentY - rnd(20, 40);
    }
  }

  if (input.isPressed) {
    blazethrustFlame.velocity.y -= 0.2;
    blazethrustHoldTime++;
  } else {
    blazethrustHoldTime -= 3;
    if (blazethrustHoldTime < 0) {
      blazethrustHoldTime = 0;
    }
  }

  blazethrustFlame.velocity.y += 0.1;

  float scrollSpeed = 0;
  if (blazethrustFlame.pos.y < 60) {
    scrollSpeed = (60 - blazethrustFlame.pos.y) * 0.1;
    blazethrustFlame.pos.y += scrollSpeed;
    blazethrustScrollOffset += scrollSpeed;
  }

  vectorAdd(&blazethrustFlame.pos, blazethrustFlame.velocity.x, blazethrustFlame.velocity.y);
  vectorMul(&blazethrustFlame.velocity, 0.95);

  FOR_EACH(blazethrustWindCurrents, wi2) {
    ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi2, BlazethrustWindCurrent, w2);
    SKIP_IS_NOT_ALIVE(w2);
    w2->pos.y += scrollSpeed;
  }

  if (scrollSpeed > 0) {
    bool hasTopWind = false;
    int aliveWindCount = 0;
    FOR_EACH(blazethrustWindCurrents, wi3) {
      ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi3, BlazethrustWindCurrent, w3);
      SKIP_IS_NOT_ALIVE(w3);
      aliveWindCount++;
      if (w3->pos.y < 0) {
        hasTopWind = true;
      }
    }
    if (!hasTopWind && aliveWindCount < 10) {
      float width = rnd(20, 40);
      float gap = rnd(20, 40);
      float newY = blazethrustLastWindY - gap - width;
      ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, blazethrustWindCurrentIndex, BlazethrustWindCurrent, nw);
      vectorSet(&nw->pos, 0, newY);
      nw->width = width;
      nw->force = rnd(0.1, 0.3) * RNDPM();
      nw->isAlive = true;
      blazethrustWindCurrentIndex = cgl_wrap(blazethrustWindCurrentIndex + 1, 0, BLAZETHRUST_MAX_WIND_CURRENT_COUNT);
      blazethrustLastWindY = newY;
    }
  }

  FOR_EACH(blazethrustWindCurrents, wi4) {
    ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi4, BlazethrustWindCurrent, w4);
    SKIP_IS_NOT_ALIVE(w4);
    if (w4->pos.y >= 120) {
      w4->isAlive = false;
      continue;
    }
  }

  bool anyWindAlive = false;
  float topWindY = 0;
  FOR_EACH(blazethrustWindCurrents, wi5) {
    ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi5, BlazethrustWindCurrent, w5);
    SKIP_IS_NOT_ALIVE(w5);
    if (!anyWindAlive || w5->pos.y < topWindY) {
      topWindY = w5->pos.y;
      anyWindAlive = true;
    }
  }
  if (anyWindAlive) {
    blazethrustLastWindY = topWindY;
  }

  if (blazethrustFlame.pos.x < 5) {
    blazethrustFlame.pos.x = 10;
    blazethrustFlame.velocity.x = fabs(blazethrustFlame.velocity.x) * 0.5;
  }
  if (blazethrustFlame.pos.x > 95) {
    blazethrustFlame.pos.x = 90;
    blazethrustFlame.velocity.x = -fabs(blazethrustFlame.velocity.x) * 0.5;
  }
  if (blazethrustFlame.pos.y < 5) {
    blazethrustFlame.pos.y = 10;
    blazethrustFlame.velocity.y = fabs(blazethrustFlame.velocity.y) * 0.3;
  }
  if (blazethrustFlame.pos.y > 95) {
    blazethrustFlame.pos.y = 90;
    blazethrustFlame.velocity.y = -fabs(blazethrustFlame.velocity.y) * 0.3;
  }

  blazethrustWindAnimOffset += 2;

  color = CYAN;
  FOR_EACH(blazethrustWindCurrents, wi6) {
    ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi6, BlazethrustWindCurrent, w6);
    SKIP_IS_NOT_ALIVE(w6);
    rect(w6->pos.x, w6->pos.y, 100, w6->width, &scratch);
  }

  FOR_EACH(blazethrustWindParticles, wpi) {
    ASSIGN_ARRAY_ITEM(blazethrustWindParticles, wpi, BlazethrustWindParticle, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += scrollSpeed;
    p->pos.x += p->velocity.x;
    float waveX = cos(p->age * 0.1 + p->waveOffset) * p->waveAmplitude;
    float waveY = sin(p->age * 0.1 + p->waveOffset) * p->waveAmplitude;
    p->pos.x += waveX;
    p->pos.y += waveY * 0.5;
    p->age++;
  }
  FOR_EACH(blazethrustWindParticles, pi2) {
    ASSIGN_ARRAY_ITEM(blazethrustWindParticles, pi2, BlazethrustWindParticle, p2);
    SKIP_IS_NOT_ALIVE(p2);
    if (!(p2->pos.y < 120 && p2->age < 180 && p2->pos.x > -10 && p2->pos.x < 110)) {
      p2->isAlive = false;
      continue;
    }
  }

  if (rnd(0, 1) < 0.15) {
    FOR_EACH(blazethrustWindCurrents, wi7) {
      ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi7, BlazethrustWindCurrent, w7);
      SKIP_IS_NOT_ALIVE(w7);
      if (rnd(0, 1) < 0.25) {
        ASSIGN_ARRAY_ITEM(blazethrustWindParticles, blazethrustWindParticleIndex, BlazethrustWindParticle, np);
        vectorSet(&np->pos, 100, w7->pos.y + rnd(0, w7->width));
        vectorSet(&np->velocity, -rnd(1, 2), 0);
        np->age = 0;
        np->isPurple = true;
        np->waveOffset = rnd(0, CGLP_PI * 2);
        np->waveAmplitude = rnd(0.2, 0.5);
        np->isAlive = true;
        blazethrustWindParticleIndex = cgl_wrap(blazethrustWindParticleIndex + 1, 0, BLAZETHRUST_MAX_WIND_PARTICLE_COUNT);
      }
    }

    for (int y = 0; y < 100; y += 20) {
      if (rnd(0, 1) < 0.15) {
        float particleY = y + rnd(0, 15);
        bool isInCyan = false;
        FOR_EACH(blazethrustWindCurrents, wi8) {
          ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi8, BlazethrustWindCurrent, w8);
          SKIP_IS_NOT_ALIVE(w8);
          if (particleY >= w8->pos.y && particleY <= w8->pos.y + w8->width) {
            isInCyan = true;
          }
        }
        if (!isInCyan) {
          ASSIGN_ARRAY_ITEM(blazethrustWindParticles, blazethrustWindParticleIndex, BlazethrustWindParticle, np2);
          vectorSet(&np2->pos, 0, particleY);
          vectorSet(&np2->velocity, rnd(1, 2), 0);
          np2->age = 0;
          np2->isPurple = false;
          np2->waveOffset = rnd(0, CGLP_PI * 2);
          np2->waveAmplitude = rnd(0.2, 0.5);
          np2->isAlive = true;
          blazethrustWindParticleIndex = cgl_wrap(blazethrustWindParticleIndex + 1, 0, BLAZETHRUST_MAX_WIND_PARTICLE_COUNT);
        }
      }
    }
  }

  FOR_EACH(blazethrustWindParticles, pi3) {
    ASSIGN_ARRAY_ITEM(blazethrustWindParticles, pi3, BlazethrustWindParticle, p3);
    SKIP_IS_NOT_ALIVE(p3);
    if (p3->isPurple) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_GREEN;
    }
    rect(p3->pos.x, p3->pos.y, 6, 2, &scratch);
  }

  color = TRANSPARENT;
  Collision bgCol;
  box(blazethrustFlame.pos.x, blazethrustFlame.pos.y, 1, 1, &bgCol);
  bool isOnCyan = bgCol.isColliding.rect[CYAN];

  if (isOnCyan) {
    blazethrustFlame.velocity.x -= blazethrustFlame.windEffect;
  } else {
    blazethrustFlame.velocity.x += blazethrustFlame.windEffect;
  }

  float shrinkRate = 0.0016 * difficulty * (1 + blazethrustHoldTime * 0.01);
  blazethrustFlame.size *= 1 - shrinkRate;
  blazethrustFlame.windEffect = blazethrustFlame.size * 0.02;

  if (blazethrustFlame.size <= 2) {
    play(EXPLOSION);
    gameOver();
  }

  blazethrustBlinkTimer++;

  bool shouldBlink = false;
  if (blazethrustFlame.size <= 5) {
    int blinkSpeed = (int)floor(fmax(5, 20 - (5 - blazethrustFlame.size) * 4));
    shouldBlink = blazethrustBlinkTimer % blinkSpeed < blinkSpeed / 2.0;
    if (blazethrustBlinkTimer % blinkSpeed == 0) {
      play(LASER);
    }
  }

  if (blazethrustFlame.size <= 5 && shouldBlink) {
    color = LIGHT_RED;
  } else {
    color = RED;
  }
  arc(blazethrustFlame.pos.x, blazethrustFlame.pos.y, blazethrustFlame.size, 0, CGLP_PI * 2, &scratch);

  FOR_EACH(blazethrustOxygenBubbles, oi) {
    ASSIGN_ARRAY_ITEM(blazethrustOxygenBubbles, oi, BlazethrustOxygenBubble, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->pos.y += scrollSpeed;
    ob->pos.y += ob->fallSpeed;
    bool isInCyanWind = false;
    FOR_EACH(blazethrustWindCurrents, wi9) {
      ASSIGN_ARRAY_ITEM(blazethrustWindCurrents, wi9, BlazethrustWindCurrent, w9);
      SKIP_IS_NOT_ALIVE(w9);
      if (ob->pos.y >= w9->pos.y && ob->pos.y <= w9->pos.y + w9->width) {
        isInCyanWind = true;
      }
    }
    if (isInCyanWind) {
      ob->pos.x -= ob->windEffect;
    } else {
      ob->pos.x += ob->windEffect;
    }
    if (ob->pos.x < 5) {
      ob->pos.x = 5;
    }
    if (ob->pos.x > 95) {
      ob->pos.x = 95;
    }
  }

  color = PURPLE;
  FOR_EACH(blazethrustOxygenBubbles, oi2) {
    ASSIGN_ARRAY_ITEM(blazethrustOxygenBubbles, oi2, BlazethrustOxygenBubble, ob2);
    SKIP_IS_NOT_ALIVE(ob2);
    Collision obCol;
    arc(ob2->pos.x, ob2->pos.y, ob2->size, 0, CGLP_PI * 2, &obCol);
    if (obCol.isColliding.rect[RED]) {
      play(COIN);
      addScore(blazethrustMultiplier, ob2->pos.x, ob2->pos.y);
      if (blazethrustMultiplier < 16) {
        blazethrustMultiplier++;
      }
      blazethrustFlame.size = blazethrustFlame.size + ob2->size * 0.2 * difficulty;
      if (blazethrustFlame.size > 25) {
        blazethrustFlame.size = 25;
      }
      blazethrustFlame.windEffect = blazethrustFlame.size * 0.02;

      int particleCount = (int)floor(ob2->size * 3);
      float ringRadius = ob2->size + 2;
      for (int pci = 0; pci < particleCount; pci++) {
        float angle = ((float)pci / particleCount) * CGLP_PI * 2;
        float particleX = ob2->pos.x + cos(angle) * ringRadius;
        float particleY = ob2->pos.y + sin(angle) * ringRadius;
        particle(particleX, particleY, 1, 2, angle, 0.3);
      }
      ob2->isAlive = false;
      continue;
    }
    if (ob2->pos.y > 120) {
      if (blazethrustMultiplier > 1) {
        play(HIT);
      }
      blazethrustMultiplier = 1;
      ob2->isAlive = false;
      continue;
    }
  }

  if (scrollSpeed > 0) {
    int aliveBubbleCount = 0;
    FOR_EACH(blazethrustOxygenBubbles, oi3) {
      ASSIGN_ARRAY_ITEM(blazethrustOxygenBubbles, oi3, BlazethrustOxygenBubble, ob3);
      SKIP_IS_NOT_ALIVE(ob3);
      aliveBubbleCount++;
    }
    if (aliveBubbleCount < 15) {
      blazethrustNextBubbleDistance -= scrollSpeed;
      if (blazethrustNextBubbleDistance <= 0) {
        float size = rnd(3, 9);
        ASSIGN_ARRAY_ITEM(blazethrustOxygenBubbles, blazethrustOxygenBubbleIndex, BlazethrustOxygenBubble, nb);
        vectorSet(&nb->pos, rnd(10, 90), -size);
        nb->size = size;
        nb->windEffect = size * 0.02;
        nb->fallSpeed = 0.3 - (size - 3) * 0.015;
        nb->isAlive = true;
        blazethrustOxygenBubbleIndex = cgl_wrap(blazethrustOxygenBubbleIndex + 1, 0, BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT);
        blazethrustNextBubbleDistance = rnd(5, 55);
      }
    }
  }

  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(blazethrustMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameBlazethrust() {
  addGame(blazethrustTitle, blazethrustDescription, blazethrustCharacters,
          blazethrustCharactersCount, &blazethrustOptions, false, &blazethrustUpdate);
}
