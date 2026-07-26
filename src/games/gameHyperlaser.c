#include "../cglp.h"

int* hyperlaserTitle = "HYPER LASER";
int* hyperlaserDescription = "[Hold]\n Laser";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hyperlaserCharacters = {
    {
        "LllllL",
        "lLLLLl",
        "lLLLLl",
        "lLLLLl",
        "lLLLLl",
        "LllllL",
    },
    {
        "  l   ",
        "  l   ",
        "ll ll ",
        "  l   ",
        "  l   ",
    },
    {
        " yy   ",
        "ylly  ",
        "ylly  ",
        " yy   ",
    },
};
int hyperlaserCharactersCount = 3;

Options hyperlaserOptions = {100, 100, 20, false};

int[4] hyperlaserLaserColors = {BLUE, PURPLE, CYAN, GREEN};

struct HyperlaserLaser {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
};
#define HYPERLASER_MAX_LASER_COUNT 32
HyperlaserLaser[HYPERLASER_MAX_LASER_COUNT] hyperlaserLasers;
int hyperlaserLaserIndex;

struct HyperlaserBlock {
  Vector pos;
  bool isAlive;
};
#define HYPERLASER_MAX_BLOCK_COUNT 64
HyperlaserBlock[HYPERLASER_MAX_BLOCK_COUNT] hyperlaserBlocks;
int hyperlaserBlockIndex;
float hyperlaserNextBlockDist;

struct HyperlaserPlayer {
  Vector pos;
  float angle;
  float laserTicks;
  bool hasShield;
};
HyperlaserPlayer hyperlaserPlayer;

struct HyperlaserEnemy {
  Vector pos;
  float angle;
  float targetAngle;
  float turretAngle;
  float burstTicks;
  int burstIndex;
  int burstCount;
  float shotTicks;
  float shotInterval;
  float turnTicks;
  bool isAlive;
};
#define HYPERLASER_MAX_ENEMY_COUNT 32
HyperlaserEnemy[HYPERLASER_MAX_ENEMY_COUNT] hyperlaserEnemies;
int hyperlaserEnemyIndex;
float hyperlaserNextEnemyDist;

struct HyperlaserShot {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define HYPERLASER_MAX_SHOT_COUNT 32
HyperlaserShot[HYPERLASER_MAX_SHOT_COUNT] hyperlaserShots;
int hyperlaserShotIndex;

int hyperlaserMultiplier;

void hyperlaserUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(hyperlaserLasers);
    hyperlaserLaserIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(hyperlaserBlocks);
    hyperlaserBlockIndex = 0;
    hyperlaserNextBlockDist = 0;
    vectorSet(&hyperlaserPlayer.pos, 50, 95);
    hyperlaserPlayer.angle = -CGLP_PI / 2;
    hyperlaserPlayer.laserTicks = 0;
    hyperlaserPlayer.hasShield = false;
    INIT_UNALIVED_ARRAY_FAST(hyperlaserEnemies);
    hyperlaserEnemyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(hyperlaserShots);
    hyperlaserShotIndex = 0;
    hyperlaserNextEnemyDist = 0;
    hyperlaserMultiplier = 1;
  }
  color = LIGHT_CYAN;
  rect(0, 0, 5, 100, &scratch);
  rect(95, 0, 5, 100, &scratch);
  float scr = difficulty * 0.1;
  hyperlaserNextBlockDist -= scr;
  if (hyperlaserNextBlockDist < 0) {
    ASSIGN_ARRAY_ITEM(hyperlaserBlocks, hyperlaserBlockIndex, HyperlaserBlock, nb);
    vectorSet(&nb->pos, rnd(8, 92), -9);
    nb->isAlive = true;
    hyperlaserBlockIndex = cgl_wrap(hyperlaserBlockIndex + 1, 0, HYPERLASER_MAX_BLOCK_COUNT);
    hyperlaserNextBlockDist = rnd(0, 16);
  }
  color = RED;
  Vector sp;
  vectorSet(&sp, clamp(input.pos.x, 1, 99), clamp(input.pos.y, 1, 99));
  character("b", sp.x, sp.y, &scratch);
  float ta = angleTo(&hyperlaserPlayer.pos, sp.x, sp.y);
  if (input.isPressed) {
    hyperlaserPlayer.hasShield = false;
    hyperlaserPlayer.laserTicks--;
    if (hyperlaserPlayer.laserTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(hyperlaserLasers, hyperlaserLaserIndex, HyperlaserLaser, nl);
      nl->pos = hyperlaserPlayer.pos;
      vectorSet(&nl->vel, 0, 0);
      addWithAngle(&nl->vel, ta, sqrt(difficulty) * 3);
      nl->ticks = 0;
      nl->isAlive = true;
      hyperlaserLaserIndex = cgl_wrap(hyperlaserLaserIndex + 1, 0, HYPERLASER_MAX_LASER_COUNT);
      hyperlaserPlayer.laserTicks = 9 / sqrt(difficulty);
    }
  } else {
    hyperlaserPlayer.hasShield = true;
    float x = clamp(input.pos.x, 7, 93);
    float s = sqrt(difficulty) * 0.3;
    if (hyperlaserPlayer.pos.x < x - 1) {
      hyperlaserPlayer.pos.x += s;
      hyperlaserPlayer.angle += cgl_wrap(-hyperlaserPlayer.angle, -CGLP_PI, CGLP_PI) * 0.1;
    }
    if (hyperlaserPlayer.pos.x > x + 1) {
      hyperlaserPlayer.pos.x -= s;
      hyperlaserPlayer.angle += cgl_wrap(CGLP_PI - hyperlaserPlayer.angle, -CGLP_PI, CGLP_PI) * 0.1;
    }
    color = LIGHT_BLUE;
    thickness = 2;
    arc(hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y, 5, ticks * 0.03, ticks * 0.1 + CGLP_PI,
        &scratch);
    arc(hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y, 5, ticks * 0.03 + CGLP_PI,
        ticks * 0.1 + CGLP_PI * 2, &scratch);
  }
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y, 1, hyperlaserPlayer.angle, &scratch);
  Vector p;
  vectorSet(&p, hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y);
  color = LIGHT_BLACK;
  addWithAngle(&p, hyperlaserPlayer.angle + CGLP_PI / 2, 2);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(p.x, p.y, 4, hyperlaserPlayer.angle, &scratch);
  addWithAngle(&p, hyperlaserPlayer.angle + CGLP_PI / 2, -4);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(p.x, p.y, 4, hyperlaserPlayer.angle, &scratch);
  color = RED;
  thickness = 2;
  barCenterPosRatio = -0.5;
  bar(hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y, 2, ta, &scratch);
  color = BLACK;
  FOR_EACH(hyperlaserBlocks, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserBlocks, i, HyperlaserBlock, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += scr;
    Collision bc;
    character("a", b->pos.x, b->pos.y, &bc);
    if (bc.isColliding.rect[BLACK]) {
      particle(b->pos.x, b->pos.y, 16, 1, 0, CGLP_PI * 2);
      b->isAlive = false;
      continue;
    }
    bool removeOffTop = bc.isColliding.character['a'] && b->pos.y < -3;
    if (removeOffTop || b->pos.y > 103) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(hyperlaserLasers, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserLasers, i, HyperlaserLaser, l);
    SKIP_IS_NOT_ALIVE(l);
    l->pos.y += scr;
    Vector pp;
    vectorSet(&pp, l->pos.x, l->pos.y);
    color = TRANSPARENT;
    vectorAdd(&l->pos, l->vel.x, l->vel.y);
    bool isColliding = false;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc1;
    bar(l->pos.x, pp.y, 5, vectorAngle(&l->vel), &lc1);
    if (lc1.isColliding.character['a'] || (l->pos.x < 5 && l->vel.x < 0) ||
        (l->pos.x > 95 && l->vel.x > 0)) {
      l->vel.x *= -1;
      isColliding = true;
    }
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc2;
    bar(pp.x, l->pos.y, 5, vectorAngle(&l->vel), &lc2);
    if (lc2.isColliding.character['a']) {
      l->vel.y *= -1;
      isColliding = true;
    }
    if (!isColliding) {
      thickness = 2;
      barCenterPosRatio = 0.5;
      Collision lc3;
      bar(l->pos.x, l->pos.y, 5, vectorAngle(&l->vel), &lc3);
      if (lc3.isColliding.character['a']) {
        l->vel.x *= -1;
        l->vel.y *= -1;
        isColliding = true;
      }
    }
    if (isColliding) {
      l->pos = pp;
      vectorAdd(&l->pos, l->vel.x, l->vel.y);
    }
    color = hyperlaserLaserColors[(int)l->ticks % 4];
    l->ticks++;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc4;
    bar(l->pos.x, l->pos.y, 5, vectorAngle(&l->vel), &lc4);
    if (lc4.isColliding.character['a']) {
      l->isAlive = false;
      continue;
    }
    if (l->pos.y < -5 || l->pos.y > 105 || l->ticks > 120 / sqrt(difficulty)) {
      l->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(hyperlaserEnemies, aliveEnemyCount0);
  if (aliveEnemyCount0 == 0) {
    // Matches the JS source exactly: it resets nextBlockDist (not
    // nextEnemyDist) here - likely an upstream quirk, preserved as-is.
    hyperlaserNextBlockDist = 0;
  }
  hyperlaserNextEnemyDist--;
  if (hyperlaserNextEnemyDist < 0) {
    float shotInterval = rnd(60, 120) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(hyperlaserEnemies, hyperlaserEnemyIndex, HyperlaserEnemy, ne);
    vectorSet(&ne->pos, rnd(10, 90), -9);
    ne->angle = CGLP_PI / 2;
    ne->targetAngle = CGLP_PI / 2;
    ne->turretAngle = CGLP_PI / 2;
    ne->burstTicks = 9999;
    ne->burstIndex = 0;
    ne->burstCount = rndi(3, 9);
    ne->shotTicks = rnd(0, shotInterval);
    ne->shotInterval = shotInterval;
    ne->turnTicks = 0;
    ne->isAlive = true;
    hyperlaserEnemyIndex = cgl_wrap(hyperlaserEnemyIndex + 1, 0, HYPERLASER_MAX_ENEMY_COUNT);
    hyperlaserNextEnemyDist = rnd(150, 200) / sqrt(difficulty);
  }
  FOR_EACH(hyperlaserEnemies, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserEnemies, i, HyperlaserEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += scr;
    Vector pp;
    vectorSet(&pp, e->pos.x, e->pos.y);
    bool isMoving = false;
    if (fabs(cgl_wrap(e->angle - e->targetAngle, -CGLP_PI, CGLP_PI)) < 0.1) {
      e->angle = e->targetAngle;
      addWithAngle(&e->pos, e->angle, sqrt(difficulty) * 0.1);
      isMoving = true;
      e->turnTicks--;
    } else {
      e->angle += cgl_wrap(e->targetAngle - e->angle, -CGLP_PI, CGLP_PI) * 0.1;
    }
    color = YELLOW;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision ec;
    bar(e->pos.x, e->pos.y, 2, e->angle, &ec);
    Vector ep;
    vectorSet(&ep, e->pos.x, e->pos.y);
    color = LIGHT_BLACK;
    addWithAngle(&ep, e->angle + CGLP_PI / 2, 2);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(ep.x, ep.y, 4, e->angle, &scratch);
    addWithAngle(&ep, e->angle + CGLP_PI / 2, -4);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(ep.x, ep.y, 4, e->angle, &scratch);
    e->shotTicks--;
    bool hasShield = e->shotTicks > 0 && e->pos.y < 70;
    if (hasShield) {
      e->turretAngle =
          round(angleTo(&e->pos, hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y) /
                (CGLP_PI / 4)) *
          CGLP_PI / 4;
    }
    color = RED;
    thickness = 2;
    barCenterPosRatio = -0.5;
    bar(e->pos.x, e->pos.y, 2, e->turretAngle, &scratch);
    if (e->shotTicks < 0 && e->burstIndex <= 0) {
      e->burstTicks = 0;
      e->burstIndex = e->burstCount;
    }
    e->burstTicks--;
    if (e->burstTicks < 0) {
      ASSIGN_ARRAY_ITEM(hyperlaserShots, hyperlaserShotIndex, HyperlaserShot, ns);
      ns->pos = e->pos;
      vectorSet(&ns->vel, 0, 0);
      addWithAngle(&ns->vel, e->turretAngle, sqrt(difficulty) * 2);
      ns->isAlive = true;
      hyperlaserShotIndex = cgl_wrap(hyperlaserShotIndex + 1, 0, HYPERLASER_MAX_SHOT_COUNT);
      e->burstIndex--;
      if (e->burstIndex > 0) {
        e->burstTicks = 20 / sqrt(difficulty);
      } else {
        e->burstTicks = 9999;
        e->shotTicks = e->shotInterval;
      }
    }
    if (isMoving && (ec.isColliding.character['a'] || e->turnTicks < 0 || e->pos.x < 9 ||
                     e->pos.x > 90)) {
      e->pos = pp;
      e->targetAngle =
          cgl_wrap(e->targetAngle + (rndi(0, 2) * 2 - 1) * CGLP_PI / 2, -CGLP_PI, CGLP_PI);
      e->turnTicks = rnd(200, 300) / sqrt(difficulty);
    }
    color = LIGHT_YELLOW;
    if (hasShield) {
      thickness = 2;
      arc(e->pos.x, e->pos.y, 4, ticks * 0.03, ticks * 0.1 + CGLP_PI, &scratch);
      arc(e->pos.x, e->pos.y, 4, ticks * 0.03 + CGLP_PI, ticks * 0.1 + CGLP_PI * 2, &scratch);
    } else if (ec.isColliding.rect[BLUE] || ec.isColliding.rect[PURPLE] ||
               ec.isColliding.rect[CYAN] || ec.isColliding.rect[GREEN]) {
      play(EXPLOSION);
      addScore(hyperlaserMultiplier * 10, e->pos.x, e->pos.y);
      color = RED;
      particle(e->pos.x, e->pos.y, 19, 2, 0, CGLP_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->pos.y > 99) {
      color = RED;
      text("X", e->pos.x, 96, &scratch);
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  color = TRANSPARENT;
  FOR_EACH(hyperlaserBlocks, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserBlocks, i, HyperlaserBlock, b);
    SKIP_IS_NOT_ALIVE(b);
    Collision bc2;
    character("a", b->pos.x, b->pos.y, &bc2);
    if (bc2.isColliding.rect[YELLOW] && b->pos.y < -3) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(hyperlaserLasers, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserLasers, i, HyperlaserLaser, l);
    SKIP_IS_NOT_ALIVE(l);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc5;
    bar(l->pos.x, l->pos.y, 5, vectorAngle(&l->vel), &lc5);
    if (lc5.isColliding.rect[LIGHT_YELLOW]) {
      play(HIT);
      l->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(hyperlaserShots, i) {
    ASSIGN_ARRAY_ITEM(hyperlaserShots, i, HyperlaserShot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.y += scr;
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    Collision sc;
    character("c", s->pos.x, s->pos.y, &sc);
    if (!hyperlaserPlayer.hasShield && sc.isColliding.rect[BLACK]) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    if (hyperlaserPlayer.hasShield && sc.isColliding.rect[LIGHT_BLUE]) {
      play(POWER_UP);
      addScore(hyperlaserMultiplier, hyperlaserPlayer.pos.x, hyperlaserPlayer.pos.y);
      hyperlaserMultiplier++;
      s->isAlive = false;
      continue;
    }
    bool inRect = s->pos.x >= 5 && s->pos.x < 95 && s->pos.y >= 0 && s->pos.y < 100;
    if (!inRect || sc.isColliding.character['a']) {
      s->isAlive = false;
      continue;
    }
  }
}

void addGameHyperlaser() {
  addGame(hyperlaserTitle, hyperlaserDescription, hyperlaserCharacters,
          hyperlaserCharactersCount, &hyperlaserOptions, true, &hyperlaserUpdate);
}
