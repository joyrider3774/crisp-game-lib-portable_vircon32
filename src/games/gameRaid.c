#include "../cglp.h"

int* raidTitle = "RAID";
int* raidDescription = "[Hold]\n Speed up\n[Release]\n Bomb";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] raidCharacters = {
    {
        "ll    ",
        "lllll ",
        "llllll",
    },
    {
        "lll   ",
        "lll   ",
        "lll   ",
        "lll   ",
        " l    ",
    },
};
int raidCharactersCount = 2;

Options raidOptions = {100, 100, 400, true};

int[4] raidBuildingColors = {GREEN, YELLOW, PURPLE, CYAN};

struct RaidShip {
  Vector pos;
  Vector vel;
  float speed;
  float downDist;
  float bombVy;
};
RaidShip raidShip;

bool raidHasBomb;
Vector raidBombPos;
Vector raidBombVel;

struct RaidBuilding {
  float height;
};
RaidBuilding[8] raidBuildings;

struct RaidFalling {
  Vector pos;
  float vy;
  bool isAlive;
};
// Upstream JS uses forEach (not remove()) on this list, so fallen debris is
// never actually removed - it just keeps drawing forever, permanently off
// the bottom of the view. A ring buffer recycles the oldest (always
// long-offscreen) slot instead of growing without bound; visually identical.
#define RAID_MAX_FALLING_COUNT 64
RaidFalling[RAID_MAX_FALLING_COUNT] raidFallings;
int raidFallingIndex;

struct RaidCloud {
  Vector pos;
  float size;
  bool isAlive;
};
#define RAID_MAX_CLOUD_COUNT 64
RaidCloud[RAID_MAX_CLOUD_COUNT] raidClouds;
int raidCloudIndex;
float raidNextCloudDist;

void raidAddClouds(float y) {
  float x = rnd(-10, 90);
  int c = rndi(2, 5);
  TIMES(c, ci) {
    float size = rnd(7, 12);
    ASSIGN_ARRAY_ITEM(raidClouds, raidCloudIndex, RaidCloud, nc);
    vectorSet(&nc->pos, x, y + rnd(0, 3) * RNDPM());
    nc->size = size;
    nc->isAlive = true;
    raidCloudIndex = cgl_wrap(raidCloudIndex + 1, 0, RAID_MAX_CLOUD_COUNT);
    x += size * rnd(0.4, 0.6);
  }
}

void raidUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&raidShip.pos, 50, 15);
    vectorSet(&raidShip.vel, 1, 0);
    raidShip.speed = 1;
    raidShip.downDist = 0;
    raidShip.bombVy = 0;
    raidHasBomb = false;
    TIMES(8, bi) { raidBuildings[bi].height = rnd(10, 60); }
    INIT_UNALIVED_ARRAY_FAST(raidFallings);
    raidFallingIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(raidClouds);
    raidCloudIndex = 0;
    TIMES(3, cci) { raidAddClouds(rnd(10, 90)); }
    raidNextCloudDist = 0;
  }
  float scr = 0;
  if (raidShip.pos.y > 15) {
    scr += (raidShip.pos.y - 15) * 0.05;
  }
  raidNextCloudDist -= scr * 0.3;
  if (raidNextCloudDist < 0) {
    raidAddClouds(110);
    raidNextCloudDist = rnd(20, 40);
  }
  color = LIGHT_BLUE;
  FOR_EACH(raidClouds, ci2) {
    ASSIGN_ARRAY_ITEM(raidClouds, ci2, RaidCloud, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y -= scr * 0.3;
    box(c->pos.x, c->pos.y, c->size, c->size, &scratch);
    if (c->pos.y < -9) {
      c->isAlive = false;
      continue;
    }
  }
  if (input.isPressed) {
    raidShip.speed += 0.01;
  } else {
    raidShip.speed += (1 - raidShip.speed) * 0.2;
    if (!raidHasBomb && ticks > 30 && input.isJustReleased) {
      play(POWER_UP);
      raidHasBomb = true;
      raidBombPos = raidShip.pos;
      raidBombVel = raidShip.vel;
      vectorMul(&raidBombVel, raidShip.speed);
      raidShip.bombVy = 0.33;
    }
  }
  vectorAdd(&raidShip.pos, raidShip.vel.x * raidShip.speed, raidShip.vel.y * raidShip.speed);
  if (raidShip.vel.y == 0) {
    if ((raidShip.pos.x < 10 && raidShip.vel.x < 0) ||
        (raidShip.pos.x > 90 && raidShip.vel.x > 0)) {
      play(SELECT);
      raidShip.vel.y = 0.1;
      raidShip.downDist = difficulty;
    }
  } else {
    float target;
    if (raidShip.pos.x < 50) {
      target = 1;
    } else {
      target = -1;
    }
    raidShip.vel.x += (target - raidShip.vel.x) * 0.05;
    raidShip.downDist -= raidShip.vel.y * raidShip.speed;
    if (raidShip.downDist < 0) {
      float vx;
      if (raidShip.pos.x < 50) {
        vx = 1;
      } else {
        vx = -1;
      }
      vectorSet(&raidShip.vel, vx, 0);
    }
  }
  raidShip.pos.y += raidShip.bombVy - scr;
  raidShip.bombVy *= 0.8;
  color = BLACK;
  characterOptions.isMirrorX = raidShip.vel.x < 0;
  if (raidShip.vel.y == 0) {
    character("a", raidShip.pos.x, raidShip.pos.y, &scratch);
  } else {
    character("b", raidShip.pos.x, raidShip.pos.y, &scratch);
  }
  characterOptions.isMirrorX = false;
  if (raidHasBomb) {
    raidBombVel.y += 0.1;
    vectorMul(&raidBombVel, 0.99);
    vectorAdd(&raidBombPos, raidBombVel.x, raidBombVel.y);
    raidBombPos.y -= scr;
    color = RED;
    thickness = 3;
    bar(raidBombPos.x, raidBombPos.y, 3, vectorAngle(&raidBombVel), &scratch);
    if ((raidBombPos.x < 0 && raidBombVel.x < 0) || (raidBombPos.x > 99 && raidBombVel.x > 0)) {
      raidBombVel.x *= -1;
    }
    if (raidBombPos.y > 99) {
      raidHasBomb = false;
    }
  }
  color = LIGHT_BLACK;
  FOR_EACH(raidFallings, fi) {
    ASSIGN_ARRAY_ITEM(raidFallings, fi, RaidFalling, f);
    SKIP_IS_NOT_ALIVE(f);
    f->vy += 0.1;
    f->pos.y += f->vy - scr;
    rect(f->pos.x, f->pos.y, 9, 10, &scratch);
  }
  TIMES(8, bi2) {
    RaidBuilding* b = &raidBuildings[bi2];
    b->height += scr;
    float x = bi2 * 10 + 10;
    int c = (int)ceil(b->height / 10);
    float y = 100 - b->height + c * 10;
    bool isDestroyed = false;
    int multiplier = 1;
    TIMES(c, ci3) {
      if (isDestroyed) {
        play(HIT);
        ASSIGN_ARRAY_ITEM(raidFallings, raidFallingIndex, RaidFalling, nf);
        vectorSet(&nf->pos, x, y);
        nf->vy = -multiplier * 0.5;
        nf->isAlive = true;
        raidFallingIndex = cgl_wrap(raidFallingIndex + 1, 0, RAID_MAX_FALLING_COUNT);
        addScore(multiplier, x + 5, y);
        multiplier++;
        y -= 10;
        continue;
      }
      color = raidBuildingColors[(bi2 * 17) % 4];
      Collision cc;
      rect(x, y, 9, 10, &cc);
      color = WHITE;
      rect(x + 1, y + 1, 7, 3, &scratch);
      rect(x + 1, y + 6, 7, 3, &scratch);
      if (cc.isColliding.rect[RED]) {
        play(EXPLOSION);
        addScore(multiplier, x + 5, y);
        multiplier++;
        isDestroyed = true;
        b->height = 100 - y;
        color = RED;
        particle(x + 5, y + 5, 19, 2, 0, CGLP_PI * 2);
        raidHasBomb = false;
      }
      if (cc.isColliding.rect[LIGHT_BLACK]) {
        play(HIT);
        b->height = 100 - y;
        color = RED;
        particle(x + 5, y + 5, 16, 1, 0, CGLP_PI * 2);
      }
      if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
      y -= 10;
    }
  }
}

void addGameRaid() {
  addGame(raidTitle, raidDescription, raidCharacters, raidCharactersCount, &raidOptions, false,
          &raidUpdate);
}
