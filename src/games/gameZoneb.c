#include "../cglp.h"

int* zonebTitle = "ZONE B";
int* zonebDescription = "[Tap]\n Turn\n[Hold]\n Shot";

int[4][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] zonebCharacters = {
    {
        "l     ",
        "ll    ",
        "ll    ",
        "l     ",
    },
    {
        "llll  ",
    },
    {
        "lll   ",
    },
    {
        "lll   ",
    },
};
int zonebCharactersCount = 4;

Options zonebOptions = {100, 100, 15, false};

int[4][2] zonebAngleVels = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
#define ZONEB_SHOWN_RANGE 30

#define ZONEB_MAX_ENEMY_COUNT 256
struct ZonebEnemy {
  Vector pos;
  int angle;
  float speed;
  float shotTicks;
  int burstCount;
  float turnTicks;
  bool isReflecting;
  bool isAlive;
};
ZonebEnemy[ZONEB_MAX_ENEMY_COUNT] zonebEnemies;

#define ZONEB_WALL_COUNT 19
struct ZonebWall {
  Vector pos;
  float width;
  int angle;
  bool isAlive;
};
ZonebWall[ZONEB_WALL_COUNT] zonebWalls;

#define ZONEB_SIDE_PLAYER 0
#define ZONEB_SIDE_ENEMY 1
struct ZonebBullet {
  Vector pos;
  int angle;
  float range;
  int side;
  bool isAlive;
};
// At the level cap (256 enemies), each fires ~5 bullets per ~185-tick burst cycle with a 20-tick range, giving ~256*5/185*20 =~ 140 concurrent enemy bullets alone - 128 overflowed.
#define ZONEB_MAX_BULLET_COUNT 256
ZonebBullet[ZONEB_MAX_BULLET_COUNT] zonebBullets;
int zonebBulletIndex;

struct ZonebPlayer {
  Vector pos;
  int angle;
  float speed;
  bool isReflecting;
  float shotTicks;
};
ZonebPlayer zonebPlayer;

int zonebLevel;
bool zonebIsLevelCleared;
float zonebLevelClearTicks;

struct ZonebCircleLike {
  Vector pos;
  float radius;
};
ZonebCircleLike zonebNextCircleTarget;
ZonebCircleLike zonebNextCircle;
ZonebCircleLike zonebCircle;
float zonebCircleTicks;
int zonebMultiplier;

void zonebCheckCircleReflect(Vector* pos, int* angle) {
  if (distanceTo(pos, zonebCircle.pos.x, zonebCircle.pos.y) < zonebCircle.radius) {
    return;
  }
  float a = angleTo(pos, zonebCircle.pos.x, zonebCircle.pos.y);
  if (a < (-CGLP_PI / 4) * 3 || a > (CGLP_PI / 4) * 3) {
    *angle = 2;
  } else if (a > CGLP_PI / 4) {
    *angle = 1;
  } else if (a < -CGLP_PI / 4) {
    *angle = 3;
  } else {
    *angle = 0;
  }
}

void zonebUpdate() {
  Collision scratch;
  if (!ticks) {
    zonebLevel = 0;
    zonebIsLevelCleared = true;
    zonebLevelClearTicks = 0;
  }
  if (zonebIsLevelCleared) {
    zonebCircleTicks = 99;
    vectorSet(&zonebCircle.pos, 50, 50);
    zonebCircle.radius = 120;
    vectorSet(&zonebNextCircle.pos, 50, 50);
    zonebNextCircle.radius = 60;
    vectorSet(&zonebNextCircleTarget.pos, 0, 0);
    zonebNextCircleTarget.radius = 0;
    int enemyCount = 9 + zonebLevel * 7;
    if (enemyCount > ZONEB_MAX_ENEMY_COUNT) {
      enemyCount = ZONEB_MAX_ENEMY_COUNT;
    }
    INIT_UNALIVED_ARRAY_FAST(zonebEnemies);
    TIMES(enemyCount, ei) {
      ZonebEnemy* e = &zonebEnemies[ei];
      vectorSet(&e->pos, rnd(0, 99), rnd(0, 99));
      e->angle = rndi(0, 4);
      e->speed = 0;
      e->shotTicks = rnd(200, 300);
      e->burstCount = 0;
      e->turnTicks = 0;
      e->isReflecting = false;
      e->isAlive = true;
    }
    TIMES(ZONEB_WALL_COUNT, wi) {
      ZonebWall* w = &zonebWalls[wi];
      vectorSet(&w->pos, rnd(9, 89), rnd(9, 89));
      w->width = rnd(5, 15);
      w->angle = rndi(0, 2);
      w->isAlive = true;
    }
    INIT_UNALIVED_ARRAY_FAST(zonebBullets);
    zonebBulletIndex = 0;
    vectorSet(&zonebPlayer.pos, 50, 80);
    zonebPlayer.angle = 3;
    zonebPlayer.speed = 0;
    zonebPlayer.isReflecting = false;
    zonebPlayer.shotTicks = 0;
    zonebLevel++;
    zonebIsLevelCleared = false;
    zonebMultiplier = 1;
  }
  zonebCircleTicks--;
  if (zonebCircleTicks == 9) {
    zonebNextCircleTarget.radius = zonebNextCircle.radius - 10;
    zonebNextCircleTarget.pos = zonebNextCircle.pos;
    TIMES(99, ci) {
      vectorSet(&zonebNextCircleTarget.pos, rnd(10, 90), rnd(10, 90));
      if (distanceTo(&zonebNextCircleTarget.pos, zonebNextCircle.pos.x, zonebNextCircle.pos.y) <
          zonebNextCircle.radius - zonebNextCircleTarget.radius) {
        break;
      }
    }
  }
  if (zonebCircleTicks < 0) {
    zonebCircleTicks = 600;
  }
  if (zonebCircleTicks < 9) {
    vectorAdd(&zonebNextCircle.pos,
              (zonebNextCircleTarget.pos.x - zonebNextCircle.pos.x) / (zonebCircleTicks + 1),
              (zonebNextCircleTarget.pos.y - zonebNextCircle.pos.y) / (zonebCircleTicks + 1));
    zonebNextCircle.radius +=
        (zonebNextCircleTarget.radius - zonebNextCircle.radius) / (zonebCircleTicks + 1);
  }
  if (zonebNextCircle.radius < 60) {
    color = LIGHT_BLACK;
    thickness = 2;
    arc(zonebNextCircle.pos.x, zonebNextCircle.pos.y, zonebNextCircle.radius, 0, CGLP_PI * 2,
        &scratch);
    if (zonebCircleTicks > 9) {
      vectorAdd(&zonebCircle.pos, (zonebNextCircle.pos.x - zonebCircle.pos.x) / zonebCircleTicks,
                (zonebNextCircle.pos.y - zonebCircle.pos.y) / zonebCircleTicks);
      zonebCircle.radius += (zonebNextCircle.radius - zonebCircle.radius) / zonebCircleTicks;
    }
    color = BLUE;
    thickness = 3;
    arc(zonebCircle.pos.x, zonebCircle.pos.y, zonebCircle.radius, 0, CGLP_PI * 2, &scratch);
  }
  color = YELLOW;
  FOR_EACH(zonebWalls, wi2) {
    ASSIGN_ARRAY_ITEM(zonebWalls, wi2, ZonebWall, w);
    SKIP_IS_NOT_ALIVE(w);
    Collision wc;
    if (w->angle == 0) {
      box(w->pos.x, w->pos.y, w->width, 2, &wc);
    } else {
      box(w->pos.x, w->pos.y, 2, w->width, &wc);
    }
    if (wc.isColliding.rect[BLUE]) {
      w->isAlive = false;
      continue;
    }
  }
  FOR_EACH(zonebBullets, bi) {
    ASSIGN_ARRAY_ITEM(zonebBullets, bi, ZonebBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    int avx = zonebAngleVels[b->angle][0];
    int avy = zonebAngleVels[b->angle][1];
    vectorAdd(&b->pos, avx, avy);
    bool isShown = b->side == ZONEB_SIDE_PLAYER ||
                   distanceTo(&b->pos, zonebPlayer.pos.x, zonebPlayer.pos.y) < ZONEB_SHOWN_RANGE;
    if (isShown) {
      if (b->side == ZONEB_SIDE_ENEMY) {
        color = PURPLE;
      } else {
        color = CYAN;
      }
    } else {
      color = TRANSPARENT;
    }
    characterOptions.rotation = b->angle;
    Collision bc;
    if (b->side == ZONEB_SIDE_ENEMY) {
      character("c", b->pos.x, b->pos.y, &bc);
    } else {
      character("d", b->pos.x, b->pos.y, &bc);
    }
    characterOptions.rotation = 0;
    bool outOfRect = !(b->pos.x >= 0 && b->pos.x < 99 && b->pos.y >= 0 && b->pos.y < 99);
    if (bc.isColliding.rect[YELLOW] || outOfRect) {
      b->isAlive = false;
      continue;
    }
    b->range--;
    if (b->range < 0) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(zonebEnemies, ei2) {
    ASSIGN_ARRAY_ITEM(zonebEnemies, ei2, ZonebEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    int eavx = zonebAngleVels[e->angle][0];
    int eavy = zonebAngleVels[e->angle][1];
    vectorAdd(&e->pos, eavx * e->speed, eavy * e->speed);
    bool isShown = distanceTo(&e->pos, zonebPlayer.pos.x, zonebPlayer.pos.y) < ZONEB_SHOWN_RANGE;
    if (isShown) {
      color = BLACK;
      characterOptions.rotation = e->angle;
      character("b", e->pos.x + eavx * 2, e->pos.y + eavy * 2, &scratch);
      characterOptions.rotation = 0;
    }
    if (isShown) {
      color = RED;
    } else {
      color = TRANSPARENT;
    }
    characterOptions.rotation = e->angle;
    Collision ec;
    character("a", e->pos.x, e->pos.y, &ec);
    characterOptions.rotation = 0;
    if (ec.isColliding.character['d']) {
      play(EXPLOSION);
      addScore(zonebMultiplier, e->pos.x, e->pos.y);
      zonebMultiplier++;
      e->isAlive = false;
      continue;
    }
    bool eOutOfRect = !(e->pos.x >= 0 && e->pos.x < 99 && e->pos.y >= 0 && e->pos.y < 99);
    if (ec.isColliding.rect[YELLOW] || eOutOfRect) {
      if (!e->isReflecting) {
        e->angle += 2;
        e->isReflecting = true;
      }
    } else {
      e->isReflecting = false;
    }
    if (e->shotTicks > 7) {
      e->turnTicks--;
    }
    if (e->turnTicks < 0) {
      e->angle++;
      e->turnTicks = rnd(200, 300);
    }
    e->angle = (int)cgl_wrap(e->angle, 0, 4);
    e->shotTicks--;
    if (e->shotTicks < 0) {
      e->burstCount--;
      if (e->burstCount < 0) {
        e->shotTicks = rnd(100, 200);
        e->burstCount = rndi(3, 7);
      } else {
        ASSIGN_ARRAY_ITEM(zonebBullets, zonebBulletIndex, ZonebBullet, nb);
        vectorSet(&nb->pos, e->pos.x + eavx * 5, e->pos.y + eavy * 5);
        nb->angle = e->angle;
        nb->range = 20;
        nb->side = ZONEB_SIDE_ENEMY;
        nb->isAlive = true;
        zonebBulletIndex = cgl_wrap(zonebBulletIndex + 1, 0, ZONEB_MAX_BULLET_COUNT);
        e->shotTicks += 7;
      }
    }
    float speedTarget;
    if (e->shotTicks < 7) {
      speedTarget = 0;
    } else {
      speedTarget = 0.2;
    }
    e->speed += (speedTarget - e->speed) * 0.1;
    zonebCheckCircleReflect(&e->pos, &e->angle);
    e->pos.x = clamp(e->pos.x, 0, 99);
    e->pos.y = clamp(e->pos.y, 0, 99);
  }
  int pavx = zonebAngleVels[zonebPlayer.angle][0];
  int pavy = zonebAngleVels[zonebPlayer.angle][1];
  float speedTarget2;
  if (input.isPressed) {
    speedTarget2 = 0;
  } else {
    speedTarget2 = 0.2;
  }
  zonebPlayer.speed += (speedTarget2 - zonebPlayer.speed) * 0.1;
  if (input.isJustReleased) {
    if (zonebPlayer.speed > 0.04) {
      play(LASER);
      zonebPlayer.angle = (int)cgl_wrap(zonebPlayer.angle + 1, 0, 4);
    }
  }
  zonebPlayer.shotTicks--;
  if (input.isPressed && zonebPlayer.speed < 0.04) {
    if (zonebPlayer.shotTicks < 0) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(zonebBullets, zonebBulletIndex, ZonebBullet, npb);
      vectorSet(&npb->pos, zonebPlayer.pos.x + pavx * 5, zonebPlayer.pos.y + pavy * 5);
      npb->angle = zonebPlayer.angle;
      npb->range = 20;
      npb->side = ZONEB_SIDE_PLAYER;
      npb->isAlive = true;
      zonebBulletIndex = cgl_wrap(zonebBulletIndex + 1, 0, ZONEB_MAX_BULLET_COUNT);
      zonebPlayer.shotTicks = 7;
    }
  }
  vectorAdd(&zonebPlayer.pos, pavx * zonebPlayer.speed, pavy * zonebPlayer.speed);
  zonebCheckCircleReflect(&zonebPlayer.pos, &zonebPlayer.angle);
  color = BLACK;
  characterOptions.rotation = zonebPlayer.angle;
  character("b", zonebPlayer.pos.x + pavx * 2, zonebPlayer.pos.y + pavy * 2, &scratch);
  characterOptions.rotation = 0;
  color = BLUE;
  characterOptions.rotation = zonebPlayer.angle;
  Collision pc;
  character("a", zonebPlayer.pos.x, zonebPlayer.pos.y, &pc);
  characterOptions.rotation = 0;
  if (pc.isColliding.character['c'] || zonebCircle.radius < 1 ||
      distanceTo(&zonebPlayer.pos, zonebCircle.pos.x, zonebCircle.pos.y) >
          zonebCircle.radius * 1.05) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  zonebPlayer.pos.x = clamp(zonebPlayer.pos.x, 0, 99);
  zonebPlayer.pos.y = clamp(zonebPlayer.pos.y, 0, 99);
  // Upstream clamps player.pos before this yellow/out-of-rect check, so the
  // "out of rect" half is unreachable here (position is always in-bounds by
  // this point) - kept as-is to match the original logic exactly.
  if (pc.isColliding.rect[YELLOW]) {
    if (!zonebPlayer.isReflecting) {
      zonebPlayer.angle += 2;
      zonebPlayer.isReflecting = true;
    }
  } else {
    zonebPlayer.isReflecting = false;
  }
  zonebPlayer.angle = (int)cgl_wrap(zonebPlayer.angle, 0, 4);
  COUNT_IS_ALIVE(zonebEnemies, zonebAliveEnemyCount);
  if (zonebLevelClearTicks <= -60 && zonebAliveEnemyCount == 0) {
    play(POWER_UP);
    zonebLevelClearTicks = 60;
  }
  if (zonebLevelClearTicks > 0) {
    color = BLACK;
    text("WINNER!", 30, 50, &scratch);
    zonebLevelClearTicks--;
    if (zonebLevelClearTicks == 0) {
      zonebIsLevelCleared = true;
    }
  } else if (zonebLevelClearTicks > -60) {
    color = BLACK;
    int[16] levelText;
    strcpy(levelText, "LEVEL ");
    strcat(levelText, intToChar(zonebLevel));
    text(levelText, 30, 50, &scratch);
    zonebLevelClearTicks--;
  }
}

void addGameZoneb() {
  addGame(zonebTitle, zonebDescription, zonebCharacters, zonebCharactersCount, &zonebOptions,
          false, &zonebUpdate);
}
