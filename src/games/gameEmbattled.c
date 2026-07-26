#include "../cglp.h"

int* embattledTitle = "EMBATTLED";
int* embattledDescription = "[Tap]  Turn\n[Hold] Defense";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] embattledCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
};
int embattledCharactersCount = 2;

Options embattledOptions = {100, 100, 5, false};

#define EMBATTLED_TANK_ANGLE_VEL 0.02
#define EMBATTLED_TANK_TURRET_ANGLE_VEL 0.03

struct EmbattledTank {
  Vector pos;
  float angle;
  float speed;
  float turretAngle;
  bool hasTarget;
  Vector targetPos;
  float fireTicks;
  float fireInterval;
  int side;
  bool isAlive;
};
#define EMBATTLED_MAX_TANK_COUNT 32
EmbattledTank[EMBATTLED_MAX_TANK_COUNT] embattledTanks;
int embattledTankIndex;
float embattledNextTankTicks;
int embattledCurrentSide;
int embattledSideChangeCount;

struct EmbattledBullet {
  Vector pos;
  Vector vel;
  int side;
  bool isAlive;
};
#define EMBATTLED_MAX_BULLET_COUNT 64
EmbattledBullet[EMBATTLED_MAX_BULLET_COUNT] embattledBullets;
int embattledBulletIndex;

struct EmbattledPlayer {
  Vector pos;
  float vy;
  float pressedTicks;
};
EmbattledPlayer embattledPlayer;
int embattledMultiplier;

void embattledUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(embattledTanks);
    embattledTankIndex = 0;
    embattledNextTankTicks = 0;
    embattledCurrentSide = 0;
    embattledSideChangeCount = 0;
    INIT_UNALIVED_ARRAY_FAST(embattledBullets);
    embattledBulletIndex = 0;
    vectorSet(&embattledPlayer.pos, 50, 30);
    embattledPlayer.vy = 1;
    embattledPlayer.pressedTicks = 0;
    embattledMultiplier = 1;
  }
  if (input.isJustPressed || (embattledPlayer.pos.y < 3 && embattledPlayer.vy < 0) ||
      (embattledPlayer.pos.y > 97 && embattledPlayer.vy > 0)) {
    play(SELECT);
    embattledPlayer.vy *= -1;
  }
  bool pWall = embattledPlayer.pressedTicks > 10 / sqrt(difficulty);
  if (input.isPressed) {
    embattledPlayer.pressedTicks++;
  } else {
    embattledPlayer.pos.y += embattledPlayer.vy * difficulty * 0.5;
    embattledPlayer.pressedTicks = 0;
  }
  bool playerHasWall = embattledPlayer.pressedTicks > 10 / sqrt(difficulty);
  if (!pWall && playerHasWall) {
    play(POWER_UP);
  }
  if (playerHasWall) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  int[2] pc;
  pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  pc[1] = 0;
  if (embattledPlayer.vy < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  character(pc, embattledPlayer.pos.x, embattledPlayer.pos.y, &scratch);
  characterOptions.isMirrorX = false;
  float hwr = 1;
  if (playerHasWall) {
    box(embattledPlayer.pos.x - 5, embattledPlayer.pos.y, 5, 15, &scratch);
    box(embattledPlayer.pos.x + 6, embattledPlayer.pos.y, 5, 15, &scratch);
    hwr *= 2;
  }
  FOR_EACH(embattledBullets, i) {
    ASSIGN_ARRAY_ITEM(embattledBullets, i, EmbattledBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x * hwr, b->vel.y * hwr);
    if (b->side == 0) {
      color = RED;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc;
    bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel), &bc);
    if (bc.isColliding.rect[CYAN]) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
    if (bc.isColliding.character['a'] || bc.isColliding.character['b']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  COUNT_IS_ALIVE(embattledTanks, aliveTankCount);
  if (aliveTankCount == 0) {
    embattledNextTankTicks = 0;
  }
  embattledNextTankTicks -= hwr;
  if (embattledNextTankTicks < 0) {
    int side = embattledCurrentSide;
    embattledSideChangeCount--;
    if (embattledSideChangeCount <= 0) {
      if (embattledCurrentSide == 0) {
        embattledCurrentSide = 1;
      } else {
        embattledCurrentSide = 0;
      }
      embattledSideChangeCount = rndi(1, 4);
    }
    Vector npos;
    if (side == 0) {
      vectorSet(&npos, -5, rnd(0, 99));
    } else {
      vectorSet(&npos, 105, rnd(0, 99));
    }
    float angle = angleTo(&npos, embattledPlayer.pos.x, embattledPlayer.pos.y);
    float fireInterval = rnd(300, 400) / difficulty;
    ASSIGN_ARRAY_ITEM(embattledTanks, embattledTankIndex, EmbattledTank, nt);
    nt->pos = npos;
    nt->angle = angle;
    nt->speed = rnd(1, difficulty) * 0.02;
    nt->turretAngle = angle;
    nt->hasTarget = false;
    nt->fireTicks = rnd(0, fireInterval);
    nt->fireInterval = fireInterval;
    nt->side = side;
    nt->isAlive = true;
    embattledTankIndex = cgl_wrap(embattledTankIndex + 1, 0, EMBATTLED_MAX_TANK_COUNT);
    embattledNextTankTicks = rnd(60, 80) / difficulty;
  }
  FOR_EACH(embattledTanks, i) {
    ASSIGN_ARRAY_ITEM(embattledTanks, i, EmbattledTank, t);
    SKIP_IS_NOT_ALIVE(t);
    float md;
    if (playerHasWall) {
      md = distanceTo(&t->pos, embattledPlayer.pos.x, embattledPlayer.pos.y);
      t->targetPos = embattledPlayer.pos;
      t->hasTarget = true;
    } else {
      md = 99;
      t->hasTarget = false;
    }
    FOR_EACH(embattledTanks, j) {
      ASSIGN_ARRAY_ITEM(embattledTanks, j, EmbattledTank, ot);
      SKIP_IS_NOT_ALIVE(ot);
      if (t->side == ot->side) {
        continue;
      }
      float d = distanceTo(&t->pos, ot->pos.x, ot->pos.y);
      if (d < md) {
        md = d;
        t->targetPos = ot->pos;
        t->hasTarget = true;
      }
    }
    if (t->hasTarget) {
      float ta = angleTo(&t->pos, t->targetPos.x, t->targetPos.y);
      float oa = cgl_wrap(ta - t->turretAngle, -CGLP_PI, CGLP_PI);
      float tv = EMBATTLED_TANK_TURRET_ANGLE_VEL * difficulty * hwr;
      if (fabs(oa) < tv) {
        t->turretAngle = ta;
      } else if (oa > 0) {
        t->turretAngle += tv;
      } else {
        t->turretAngle -= tv;
      }
      oa = cgl_wrap(ta - t->angle, -CGLP_PI, CGLP_PI);
      tv = EMBATTLED_TANK_ANGLE_VEL * difficulty;
      if (fabs(oa) < tv) {
        t->angle = ta;
      } else if (oa > 0) {
        t->angle += tv;
      } else {
        t->angle -= tv;
      }
    }
    addWithAngle(&t->pos, t->angle, t->speed * hwr);
    t->fireTicks -= hwr;
    if (t->fireTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(embattledBullets, embattledBulletIndex, EmbattledBullet, nb);
      nb->pos = t->pos;
      vectorSet(&nb->vel, 0, 0);
      addWithAngle(&nb->vel, t->turretAngle, difficulty * 0.5);
      nb->side = t->side;
      nb->isAlive = true;
      embattledBulletIndex = cgl_wrap(embattledBulletIndex + 1, 0, EMBATTLED_MAX_BULLET_COUNT);
      t->fireTicks = t->fireInterval;
    }
    if (t->side == 0) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_PURPLE;
    }
    thickness = 6;
    barCenterPosRatio = 0.5;
    Collision tc;
    bar(t->pos.x, t->pos.y, 1, t->angle, &tc);
    int oppositeColor;
    if (t->side == 0) {
      oppositeColor = PURPLE;
    } else {
      oppositeColor = RED;
    }
    if (tc.isColliding.rect[oppositeColor]) {
      play(EXPLOSION);
      color = BLACK;
      particle(t->pos.x, t->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(embattledMultiplier, t->pos.x, t->pos.y);
      embattledMultiplier++;
      t->isAlive = false;
      continue;
    }
    if (tc.isColliding.character['a'] || tc.isColliding.character['b']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    color = BLACK;
    thickness = 3;
    barCenterPosRatio = 0;
    bar(t->pos.x, t->pos.y, 3, t->turretAngle, &scratch);
    bool inRect = t->pos.x >= -5 && t->pos.x < 105 && t->pos.y >= -5 && t->pos.y < 105;
    if (!inRect) {
      t->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(embattledBullets, i) {
    ASSIGN_ARRAY_ITEM(embattledBullets, i, EmbattledBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    int checkColor;
    if (b->side == 0) {
      checkColor = LIGHT_PURPLE;
    } else {
      checkColor = LIGHT_RED;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc2;
    bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel), &bc2);
    if (bc2.isColliding.rect[checkColor]) {
      b->isAlive = false;
      continue;
    }
  }
  if (ticks % 60 == 0 && embattledMultiplier > 1) {
    embattledMultiplier--;
  }
  color = BLACK;
  int[16] multText;
  strcpy(multText, "+");
  strcat(multText, intToChar(embattledMultiplier));
  text(multText, 3, 9, &scratch);
}

void addGameEmbattled() {
  addGame(embattledTitle, embattledDescription, embattledCharacters,
          embattledCharactersCount, &embattledOptions, false, &embattledUpdate);
}
