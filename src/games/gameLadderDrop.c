#include "../cglp.h"

char* ladderdropTitle = "LADDER DROP";
char* ladderdropDescription = "[Tap] Drop";

int[9][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] ladderdropCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    },
    {
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        "ll  ll",
    },
    {
        "b    b",
        "bbbbbb",
        "b    b",
        "b    b",
        "bbbbbb",
        "b    b",
    },
    {
        "LLLLLL",
        "r rr r",
        "r rr r",
        "      ",
        "rr rr ",
        "rr rr ",
    },
    {
        "b    b",
        "bbbbbb",
        "b    b",
        "b    b",
        "bbbbbb",
        "b    b",
    },
    {
        "RRRRRR",
        "rrrrrr",
        "rrrrrr",
        "rrrrrr",
        "rrrrrr",
        "rrrrrr",
    },
    {
        "      ",
        "  yyy ",
        " yYYYy",
        " yYyYy",
        " yYYYy",
        "  yyy ",
    }};
int ladderdropCharactersCount = 9;

Options ladderdropOptions = {100, 100, 2, false};

struct LadderdropPanel {
  Vector pos;
  Vector size;
  int[3] lxs;
  int lxsCount;
  int state;
  bool hasCoin;
  bool isAlive;
};
#define LADDER_DROP_MAX_PANEL_COUNT 32
LadderdropPanel[LADDER_DROP_MAX_PANEL_COUNT] ladderdropPanels;
int ladderdropPanelIndex;
float ladderdropPvx;
float ladderdropNextPanelX;
int ladderdropCoinPanelCount;
struct LadderdropPlayer {
  Vector pos;
  int vx;
  int state;
};
LadderdropPlayer ladderdropPlayer;
int ladderdropLockCount;
struct LadderdropCoin {
  Vector pos;
  bool isAlive;
};
#define LADDER_DROP_MAX_COIN_COUNT 32
LadderdropCoin[LADDER_DROP_MAX_COIN_COUNT] ladderdropCoins;
int ladderdropCoinIndex;
float ladderdropScr;
float ladderdropTotalScr;
int ladderdropMultiplier;
int ladderdropCoinPanelInterval = 4;

void ladderdropAddPanel() {
  ASSIGN_ARRAY_ITEM(ladderdropPanels, ladderdropPanelIndex, LadderdropPanel, p);
  ladderdropPanelIndex = cgl_wrap(ladderdropPanelIndex + 1, 0, LADDER_DROP_MAX_PANEL_COUNT);
  Vector size;
  vectorSet(&size, rndi(4, 8), rndi(3, 6));
  int lx = -1;
  int nd = rndi(1, 5);
  p->lxsCount = 0;
  while (lx < size.x) {
    lx += nd;
    p->lxs[p->lxsCount] = lx;
    p->lxsCount++;
    lx += rndi(2, 5);
  }
  bool hasCoin = false;
  ladderdropCoinPanelCount--;
  if (ladderdropCoinPanelCount == 0) {
    ladderdropCoinPanelCount = ladderdropCoinPanelInterval;
    hasCoin = true;
  }
  vectorSet(&p->pos, clamp(ladderdropNextPanelX, 2, 98 - size.x * 6), 0);
  vectorSet(&p->size, size.x, size.y);
  p->state = 0;
  p->hasCoin = hasCoin;
  p->isAlive = true;
}

bool[10] ladderdropClBuf;

// Vircon32 port note: returns a pointer into the shared ladderdropClBuf
// buffer (same pattern as the original's file-scoped "cl" array) - the
// caller must use the result before the next drawPanel() call overwrites
// it, exactly as in the original.
bool* ladderdropDrawPanel(LadderdropPanel* p) {
  int lc;
  if (p->state == 2) {
    lc = 'e';
  } else {
    lc = 'g';
  }
  int fc;
  if (p->state == 1) {
    fc = 'h';
  } else {
    fc = 'f';
  }
  int c;
  int li = 0;
  memset(ladderdropClBuf, false, sizeof(ladderdropClBuf));
  int[2] cc;
  cc[0] = 0;
  cc[1] = 0;
  TIMES(p->size.x, x) {
    TIMES(p->size.y, y) {
      c = 0;
      if (y == 0 && p->hasCoin) {
        c = 'i';
      } else if (y >= 1 && li < p->lxsCount && x == p->lxs[li]) {
        c = lc;
      } else if (y == 1) {
        c = fc;
      }
      if (c > 0) {
        cc[0] = c;
        Collision cl;
        character(cc, p->pos.x + x * 6 + 3, p->pos.y + y * 6 + 3, &cl);
        bool* clc = cl.isColliding.character;
        for (int i = 0; i < 10; i++) {
          ladderdropClBuf[i] |= clc[i + 'a'];
        }
      }
    }
    if (x == p->lxs[li]) {
      li++;
    }
  }
  return ladderdropClBuf;
}

void ladderdropUpdate() {
  Collision scratch;
  // difficulty doesn't change during a single ladderdropUpdate() call,
  // so sd is computed once here and reused everywhere
  // below instead of being recomputed on every use - most importantly,
  // the per-panel loop further down recomputes it for every panel
  // currently in a given state (up to LADDER_DROP_MAX_PANEL_COUNT=32),
  // when the exact same value works for all of them.
  float sd = sqrt(difficulty);
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(ladderdropPanels);
    ASSIGN_ARRAY_ITEM(ladderdropPanels, 0, LadderdropPanel, p);
    vectorSet(&p->pos, 2, 88);
    vectorSet(&p->size, 16, 2);
    p->lxsCount = 0;
    p->state = 2;
    p->hasCoin = false;
    p->isAlive = true;
    ladderdropPanelIndex = 1;
    ladderdropPvx = 1;
    ladderdropNextPanelX = 50;
    ladderdropCoinPanelCount = ladderdropCoinPanelInterval;
    ladderdropAddPanel();
    vectorSet(&ladderdropPlayer.pos, 9, 91);
    ladderdropPlayer.vx = 1;
    ladderdropPlayer.state = 0;
    INIT_UNALIVED_ARRAY_FAST(ladderdropCoins);
    ladderdropCoinIndex = 0;
    ladderdropScr = ladderdropTotalScr = 0;
    ladderdropMultiplier = 1;
    ladderdropLockCount = 0;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 2, 100, &scratch);
  rect(98, 0, 2, 100, &scratch);
  color = BLACK;
  float minY = 99;
  FOR_EACH(ladderdropPanels, i) {
    ASSIGN_ARRAY_ITEM(ladderdropPanels, i, LadderdropPanel, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->state == 0) {
      p->pos.x += ladderdropPvx * sd * 1.5;
      ladderdropNextPanelX = p->pos.x;
      if (p->pos.x < -9) {
        ladderdropPvx *= -1;
        p->pos.x = -9;
      } else if (p->pos.x > 109 - p->size.x * 6) {
        ladderdropPvx *= -1;
        p->pos.x = 109 - p->size.x * 6;
      }
      ladderdropDrawPanel(p);
      if (input.isJustPressed) {
        play(SELECT);
        p->state = 1;
      }
    } else if (p->state == 1) {
      p->pos.y += 6 * sd;
      color = TRANSPARENT;
      bool* cl = ladderdropDrawPanel(p);
      if (cl['e' - 'a'] || cl['f' - 'a']) {
        while (cl['e' - 'a'] || cl['f' - 'a']) {
          p->pos.y--;
          cl = ladderdropDrawPanel(p);
        }
        p->pos.y = floor(p->pos.y) + fmod(ladderdropTotalScr, 1);
        p->state = 2;
        if (p->hasCoin) {
          p->hasCoin = false;
          TIMES(p->size.x, x) {
            ASSIGN_ARRAY_ITEM(ladderdropCoins, ladderdropCoinIndex, LadderdropCoin, c);
            vectorSet(&c->pos, p->pos.x + x * 6 + 2, p->pos.y + 2);
            c->isAlive = true;
            ladderdropCoinIndex = cgl_wrap(ladderdropCoinIndex + 1, 0, LADDER_DROP_MAX_COIN_COUNT);
          }
        }
        ladderdropAddPanel();
      }
      color = BLACK;
      ladderdropDrawPanel(p);
    } else if (p->state == 2) {
      p->pos.y += ladderdropScr;
      color = BLACK;
      ladderdropDrawPanel(p);
      if (p->pos.y < minY) {
        minY = p->pos.y;
      }
    }
    if (p->pos.y > 99) {
      if (p->state == 1) {
        ladderdropAddPanel();
      }
      p->isAlive = false;
    }
  }
  int panelIter = 0;
  for (int i = 0; i < LADDER_DROP_MAX_PANEL_COUNT; i++) {
    if (ladderdropPanels[i].isAlive) {
      if (panelIter != i) {
        ladderdropPanels[panelIter] = ladderdropPanels[i];
        ladderdropPanels[i].isAlive = false;
      }
      panelIter++;
    }
  }
  ladderdropPanelIndex = panelIter;
  color = BLACK;
  ladderdropPlayer.pos.y += ladderdropScr;
  //  player.state: "walk" | "up" | "down" | "downWalk" | "drop"
  if (ladderdropPlayer.state == 0 || ladderdropPlayer.state == 3) {
    ladderdropPlayer.pos.x += ladderdropPlayer.vx * sd * 0.4;
    int[2] pc;
    pc[0] = 'a' + (int)(ticks / 30) % 2;
    pc[1] = 0;
    characterOptions.isMirrorX = ladderdropPlayer.vx < 0;
    Collision pcl;
    character(pc, ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &pcl);
    bool* c = pcl.isColliding.character;
    characterOptions.isMirrorX = false;
    if (c['h']) {
      play(EXPLOSION);
      gameOver();
    }
    if (c['f'] || ladderdropPlayer.pos.x < 5 || ladderdropPlayer.pos.x > 95) {
      play(LASER);
      ladderdropPlayer.vx *= -1;
      ladderdropPlayer.pos.x += ladderdropPlayer.vx * 2;
      ladderdropLockCount++;
      if (ladderdropLockCount > 8) {
        ladderdropPlayer.pos.x = clamp(ladderdropPlayer.pos.x, 10, 90);
        ladderdropPlayer.pos.y -= 6;
        ladderdropLockCount = 0;
        ladderdropPlayer.state = 4;
      }
    } else {
      ladderdropLockCount = 0;
    }
    if (c['e']) {
      if (ladderdropPlayer.state == 0) {
        ladderdropPlayer.state = 1;
      }
    } else {
      ladderdropPlayer.state = 0;
      color = TRANSPARENT;
      Collision pcl2;
      character("a", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y + 6, &pcl2);
      bool* c2 = pcl2.isColliding.character;
      if (!(c2['e'] || c2['f'])) {
        ladderdropPlayer.state = 4;
      }
    }
  } else if (ladderdropPlayer.state == 1) {
    play(HIT);
    ladderdropPlayer.pos.y -= sd * 0.3;
    color = TRANSPARENT;
    Collision cCl;
    character("c", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &cCl);
    bool* c = cCl.isColliding.character;
    if (!c['e'] && c['f']) {
      ladderdropPlayer.state = 2;
    } else if (!c['e']) {
      float py = ladderdropPlayer.pos.y;
      character("c", ladderdropPlayer.pos.x, py, &cCl);
      c = cCl.isColliding.character;
      while (c['e']) {
        py++;
        character("c", ladderdropPlayer.pos.x, py, &cCl);
        c = cCl.isColliding.character;
      }
      ladderdropPlayer.pos.y = floor(py) + fmod(ladderdropTotalScr, 1);
      ladderdropPlayer.pos.x += ladderdropPlayer.vx * sd * 0.5;
      ladderdropPlayer.state = 0;
    }
    color = BLACK;
    int[2] pc;
    pc[0] = 'c' + (int)(ticks / 30) % 2;
    pc[1] = 0;
    character(pc, ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &scratch);
  } else if (ladderdropPlayer.state == 2) {
    play(HIT);
    ladderdropPlayer.pos.y += sd * 0.4;
    color = TRANSPARENT;
    Collision cCl;
    character("c", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y + 6, &cCl);
    bool* c = cCl.isColliding.character;
    if (!c['e'] && c['f']) {
      float py = ladderdropPlayer.pos.y + 6;
      character("c", ladderdropPlayer.pos.x, py, &cCl);
      c = cCl.isColliding.character;
      while (c['f']) {
        py--;
        character("c", ladderdropPlayer.pos.x, py, &cCl);
        c = cCl.isColliding.character;
      }
      ladderdropPlayer.pos.y = floor(py) + fmod(ladderdropTotalScr, 1);
      ladderdropPlayer.state = 3;
    }
    color = BLACK;
    int[2] pc;
    pc[0] = 'c' + (int)(ticks / 30) % 2;
    pc[1] = 0;
    character(pc, ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &scratch);
  } else {
    ladderdropPlayer.pos.y += sd * 0.5;
    color = TRANSPARENT;
    Collision aCl;
    character("a", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &aCl);
    bool* c = aCl.isColliding.character;
    if (c['e'] || c['f']) {
      float py = ladderdropPlayer.pos.y;
      while (!(c['e'] || c['f'])) {
        py--;
        character("a", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &aCl);
        c = aCl.isColliding.character;
      }
      ladderdropPlayer.pos.y = floor(py - 1) + fmod(ladderdropTotalScr, 1);
      ladderdropPlayer.state = 0;
    }
    color = BLACK;
    characterOptions.isMirrorX = ladderdropPlayer.vx < 0;
    character("a", ladderdropPlayer.pos.x, ladderdropPlayer.pos.y, &scratch);
    characterOptions.isMirrorX = false;
  }
  if (ladderdropPlayer.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  FOR_EACH(ladderdropCoins, i) {
    ASSIGN_ARRAY_ITEM(ladderdropCoins, i, LadderdropCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += ladderdropScr;
    Collision coinCl;
    character("i", c->pos.x, c->pos.y, &coinCl);
    bool* cl = coinCl.isColliding.character;
    if (cl['a'] || cl['b']) {
      play(COIN);
      addScore(ladderdropMultiplier, c->pos.x, c->pos.y);
      ladderdropMultiplier++;
      c->isAlive = false;
    }
    if (c->pos.y > 36 && (cl['e'] || cl['f'])) {
      c->isAlive = false;
    }
    if (c->pos.y > 103) {
      if (ladderdropMultiplier > 1) {
        ladderdropMultiplier--;
      }
      c->isAlive = false;
    }
  }
  ladderdropScr = 0.01 * difficulty;
  if (minY < 30.0) {
    ladderdropScr += (30.0 - minY) * 0.1;
  }
  ladderdropTotalScr += ladderdropScr;
}

void addGameLadderDrop() {
  addGame(ladderdropTitle, ladderdropDescription, ladderdropCharacters,
          ladderdropCharactersCount, &ladderdropOptions, false, &ladderdropUpdate);
}
