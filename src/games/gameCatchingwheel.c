#include "../cglp.h"

int* catchingwheelTitle = "CATCHING WHEEL";
int* catchingwheelDescription = "[Hold]\n Rotate wheel";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] catchingwheelCharacters = {
    {
        "lllll ",
        " lll  ",
        " lll  ",
    },
    {
        "  ll  ",
        "l l  l",
        " llll ",
        "  l   ",
        " l l  ",
        "l   l ",
    },
};
int catchingwheelCharactersCount = 2;

Options catchingwheelOptions = {100, 100, 8, true};

#define CATCHINGWHEEL_SPOKE_COUNT 6
#define CATCHINGWHEEL_BASKET_CATCH_RADIUS 9

Vector catchingwheelWheelCenter;
float catchingwheelWheelRadius;
float catchingwheelWheelAngle;
float catchingwheelWheelRotationSpeed;
bool[CATCHINGWHEEL_SPOKE_COUNT] catchingwheelWheelIsAlive;

struct CatchingwheelObstacle {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define CATCHINGWHEEL_MAX_OBSTACLE_COUNT 32
CatchingwheelObstacle[CATCHINGWHEEL_MAX_OBSTACLE_COUNT] catchingwheelObstacles;
int catchingwheelObstacleIndex;
float catchingwheelNextObstacleTicks;

struct CatchingwheelHuman {
  Vector pos;
  Vector vel;
  bool isAlive;
};
// Spawn interval ~34/difficulty ticks but fall lifetime is a fixed ~100
// ticks (gravity isn't difficulty-scaled for humans) -> concurrent count
// ~2.94*difficulty grows unbounded; 512 covers difficulty up to ~170.
#define CATCHINGWHEEL_MAX_HUMAN_COUNT 512
CatchingwheelHuman[CATCHINGWHEEL_MAX_HUMAN_COUNT] catchingwheelHumans;
int catchingwheelHumanIndex;
float catchingwheelNextHumanTicks;

int catchingwheelMultiplier;

#define CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL 40
#define CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL 99
#define CATCHINGWHEEL_GRAVITY 0.02

void catchingwheelDestroyBasket(int i) {
  play(EXPLOSION);
  catchingwheelWheelIsAlive[i] = false;
  color = RED;
  float spokeAngle = catchingwheelWheelAngle + i * 2 * CGLP_PI / CATCHINGWHEEL_SPOKE_COUNT;
  Vector spokeEnd;
  spokeEnd = catchingwheelWheelCenter;
  addWithAngle(&spokeEnd, spokeAngle, catchingwheelWheelRadius);
  particle(spokeEnd.x, clamp(spokeEnd.y, 0, 95), 20, 2, 0, CGLP_PI * 2);
  catchingwheelMultiplier = 1;
}

void catchingwheelUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&catchingwheelWheelCenter, 50, 90);
    catchingwheelWheelRadius = 40;
    catchingwheelWheelAngle = 0;
    catchingwheelWheelRotationSpeed = 0.05;
    TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) { catchingwheelWheelIsAlive[i] = true; }
    INIT_UNALIVED_ARRAY_FAST(catchingwheelHumans);
    catchingwheelHumanIndex = 0;
    catchingwheelNextHumanTicks = CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL;
    INIT_UNALIVED_ARRAY_FAST(catchingwheelObstacles);
    catchingwheelObstacleIndex = 0;
    catchingwheelNextObstacleTicks = CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL;
    catchingwheelMultiplier = 1;
  }
  catchingwheelNextObstacleTicks -= difficulty;
  if (catchingwheelNextObstacleTicks <= 0) {
    ASSIGN_ARRAY_ITEM(catchingwheelObstacles, catchingwheelObstacleIndex, CatchingwheelObstacle, no);
    vectorSet(&no->pos, rnd(20, 80), 0);
    vectorSet(&no->vel, 0, 0);
    no->isAlive = true;
    catchingwheelObstacleIndex =
        cgl_wrap(catchingwheelObstacleIndex + 1, 0, CATCHINGWHEEL_MAX_OBSTACLE_COUNT);
    catchingwheelNextObstacleTicks = CATCHINGWHEEL_BASE_OBSTACLE_SPAWN_INTERVAL * rnd(0.8, 1.2);
  }
  color = RED;
  FOR_EACH(catchingwheelObstacles, i) {
    ASSIGN_ARRAY_ITEM(catchingwheelObstacles, i, CatchingwheelObstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->vel.y += CATCHINGWHEEL_GRAVITY;
    o->pos.y += o->vel.y * difficulty;
    text("*", o->pos.x, o->pos.y, &scratch);
    if (o->pos.y > 95) {
      o->isAlive = false;
    }
  }
  color = BLUE;
  thickness = 3;
  arc(catchingwheelWheelCenter.x, catchingwheelWheelCenter.y, catchingwheelWheelRadius, 0,
      CGLP_PI * 2, &scratch);
  if (input.isPressed) {
    catchingwheelWheelAngle += catchingwheelWheelRotationSpeed * difficulty;
  }
  TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) {
    float spokeAngle = catchingwheelWheelAngle + i * 2 * CGLP_PI / CATCHINGWHEEL_SPOKE_COUNT;
    Vector spokeEnd;
    spokeEnd = catchingwheelWheelCenter;
    addWithAngle(&spokeEnd, spokeAngle, catchingwheelWheelRadius);
    color = BLACK;
    thickness = 3;
    line(catchingwheelWheelCenter.x, catchingwheelWheelCenter.y, spokeEnd.x, spokeEnd.y, &scratch);
    if (catchingwheelWheelIsAlive[i]) {
      color = YELLOW;
      character("a", spokeEnd.x, spokeEnd.y, &scratch);
      // Vircon32 port note: the JS version draws this basket at 3x scale
      // (a much bigger catch radius than the plain 6x6 character), which
      // this port's character() has no equivalent for. Catching a falling
      // obstacle is therefore reimplemented here as a direct distanceTo()
      // proximity check (matching the JS scaled hitbox radius) instead of
      // relying on drawn-collision, while the basket-catches-human check
      // below still uses real character collision (unaffected by scale,
      // since neither side is drawn scaled there).
      FOR_EACH(catchingwheelObstacles, k) {
        ASSIGN_ARRAY_ITEM(catchingwheelObstacles, k, CatchingwheelObstacle, o);
        SKIP_IS_NOT_ALIVE(o);
        if (distanceTo(&o->pos, spokeEnd.x, spokeEnd.y) < CATCHINGWHEEL_BASKET_CATCH_RADIUS) {
          catchingwheelDestroyBasket(i);
          break;
        }
      }
    }
  }
  catchingwheelNextHumanTicks -= difficulty;
  if (catchingwheelNextHumanTicks <= 0) {
    ASSIGN_ARRAY_ITEM(catchingwheelHumans, catchingwheelHumanIndex, CatchingwheelHuman, nh);
    vectorSet(&nh->pos, rnd(10, 90), 0);
    vectorSet(&nh->vel, 0, 0);
    nh->isAlive = true;
    catchingwheelHumanIndex = cgl_wrap(catchingwheelHumanIndex + 1, 0, CATCHINGWHEEL_MAX_HUMAN_COUNT);
    catchingwheelNextHumanTicks = CATCHINGWHEEL_BASE_HUMAN_SPAWN_INTERVAL * rnd(0.7, 1);
  }
  color = YELLOW;
  FOR_EACH(catchingwheelHumans, i) {
    ASSIGN_ARRAY_ITEM(catchingwheelHumans, i, CatchingwheelHuman, human);
    SKIP_IS_NOT_ALIVE(human);
    human->vel.y += CATCHINGWHEEL_GRAVITY;
    vectorAdd(&human->pos, human->vel.x, human->vel.y);
    character("b", human->pos.x, human->pos.y, &scratch);
    if (scratch.isColliding.text['*']) {
      human->isAlive = false;
      continue;
    }
    if (scratch.isColliding.character['a']) {
      play(COIN);
      addScore(catchingwheelMultiplier, human->pos.x, human->pos.y);
      catchingwheelMultiplier++;
      int bi = rndi(0, CATCHINGWHEEL_SPOKE_COUNT);
      TIMES(CATCHINGWHEEL_SPOKE_COUNT, k) {
        if (!catchingwheelWheelIsAlive[bi]) {
          catchingwheelWheelIsAlive[bi] = true;
          break;
        }
        bi = (bi + 1) % CATCHINGWHEEL_SPOKE_COUNT;
      }
      human->isAlive = false;
      continue;
    }
    if (human->pos.y > 100) {
      int bi = rndi(0, CATCHINGWHEEL_SPOKE_COUNT);
      TIMES(CATCHINGWHEEL_SPOKE_COUNT, k) {
        if (catchingwheelWheelIsAlive[bi]) {
          catchingwheelDestroyBasket(bi);
          break;
        }
        bi = (bi + 1) % CATCHINGWHEEL_SPOKE_COUNT;
      }
      play(EXPLOSION);
      human->isAlive = false;
      continue;
    }
  }
  bool anyAlive = false;
  TIMES(CATCHINGWHEEL_SPOKE_COUNT, i) {
    if (catchingwheelWheelIsAlive[i]) {
      anyAlive = true;
      break;
    }
  }
  if (!anyAlive) {
    gameOver();
  }
}

void addGameCatchingwheel() {
  addGame(catchingwheelTitle, catchingwheelDescription, catchingwheelCharacters,
          catchingwheelCharactersCount, &catchingwheelOptions, false,
          &catchingwheelUpdate);
}
