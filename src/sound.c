#include "sound.h"
#include "math.h"
#include "string.h"
#include "cglp.h"
#include "machineDependent.h"
#include "random.h"
#include "misc.h"

Random soundRandom;
float _rnd(float low, float high) {
  return getRandom(&soundRandom, low, high);
}
int _rndi(int low, int high) {
  return getIntRandom(&soundRandom, low, high);
}

#define BASE_NOTE_DURATION (60.0 / tempo / 32)
#define QUANTIZED_DURATION (60.0 / tempo / 2)

bool isSoundEnabled = true;
bool isPlayingBgm;

struct Note {
  float freq;
  float duration;
  float when;
};

#define MAX_SOUND_EFFECT_NOTE_LENGTH 32
Note[SOUND_EFFECT_TYPE_COUNT][MAX_SOUND_EFFECT_NOTE_LENGTH + 1] soundEffects;
float[SOUND_EFFECT_TYPE_COUNT] soundEffectPlayedTimes;

float midiNoteToFreq(int midiNote) {
  return 440 * pow(2, (float)(midiNote - 69) / 12);
}

void addNotes(Note* ns, int count, int when, int from, int to, float amplitudeFrom, float amplitudeTo) {
  float mn = from;
  float mo = (float)(to - from) / (count - 1);
  float an = amplitudeFrom;
  float ao = (float)(amplitudeTo - amplitudeFrom) / (count - 1);
  for (int i = 0; i < count; i++) {
    Note* n = &ns[i];
    n->freq = midiNoteToFreq((int)(mn + _rnd(-an, an)));
    n->duration = BASE_NOTE_DURATION;
    n->when = when * BASE_NOTE_DURATION;
    mn += mo;
    an += ao;
    when++;
  }
}

void coinSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(4, 8);
  ns[i].freq = midiNoteToFreq(_rndi(70, 80));
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = d * BASE_NOTE_DURATION;
  i++;
  w += d;
  d = _rndi(7, 15);
  ns[i].freq = midiNoteToFreq(_rndi(85, 95));
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = d * BASE_NOTE_DURATION;
  i++;
  ns[i].freq = -1;
}

void laserSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(9, 19);
  addNotes(&ns[i], d, w, _rndi(75, 95), _rndi(45, 65), _rndi(5, 9), 0);
  i += d;
  ns[i].freq = -1;
}

void explosionSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(5, 12);
  addNotes(&ns[i], d, w, _rndi(70, 90), _rndi(50, 60), _rndi(5, 15), _rndi(15, 25));
  i++;
  w += d;
  d = _rndi(12, 20);
  addNotes(&ns[i], d, w, _rndi(50, 70), _rndi(30, 50), _rndi(15, 25), _rndi(5, 15));
  i += d;
  ns[i].freq = -1;
}

void powerUpSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(2, 5);
  addNotes(&ns[i], d, w, _rndi(75, 85), _rndi(65, 75), 0, 0);
  i += d;
  w += d;
  d = _rndi(6, 9);
  addNotes(&ns[i], d, w, _rndi(65, 75), _rndi(85, 95), 0, 0);
  i += d;
  w += d;
  d = _rndi(3, 6);
  addNotes(&ns[i], d, w, _rndi(85, 95), _rndi(75, 85), 0, 0);
  i += d;
  w += d;
  d = _rndi(6, 9);
  addNotes(&ns[i], d, w, _rndi(75, 85), _rndi(95, 105), 0, 0);
  i += d;
  ns[i].freq = -1;
}

void hitSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(5, 9);
  addNotes(&ns[i], d, w, _rndi(70, 80), _rndi(60, 70), _rndi(0, 10), _rndi(0, 10));
  i += d;
  ns[i].freq = -1;
}

void jumpSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(2, 5);
  ns[i].freq = midiNoteToFreq(_rndi(70, 80));
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = d * BASE_NOTE_DURATION;
  i++;
  w += d;
  d = _rndi(9, 19);
  addNotes(&ns[i], d, w, _rndi(55, 70), _rndi(80, 95), 0, 0);
  i += d;
  ns[i].freq = -1;
}

void selectSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(2, 4);
  int f = _rndi(60, 90);
  ns[i].freq = midiNoteToFreq(f);
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = ceil(d * 0.7) * BASE_NOTE_DURATION;
  i++;
  w += d;
  d = _rndi(4, 9);
  f += _rndi(-2, 5);
  ns[i].freq = midiNoteToFreq(f);
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = d * BASE_NOTE_DURATION;
  i++;
  ns[i].freq = -1;
}

void randomSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(3, 15);
  addNotes(&ns[i], d, w, _rndi(30, 99), _rndi(30, 99), _rndi(0, 20), _rndi(0, 20));
  i += d;
  w += d;
  d = _rndi(3, 15);
  addNotes(&ns[i], d, w, _rndi(30, 99), _rndi(30, 99), _rndi(0, 20), _rndi(0, 20));
  i += d;
  ns[i].freq = -1;
}

void clickSe(Note* ns) {
  int i = 0;
  int w = 0;
  int d = _rndi(2, 6);
  ns[i].freq = midiNoteToFreq(_rndi(65, 80));
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = ceil(d / 2) * BASE_NOTE_DURATION;
  i++;
  w += d;
  d = _rndi(2, 6);
  ns[i].freq = midiNoteToFreq(_rndi(70, 85));
  ns[i].when = w * BASE_NOTE_DURATION;
  ns[i].duration = ceil(d / 2) * BASE_NOTE_DURATION;
  i++;
  ns[i].freq = -1;
}

void generateSoundEffect() {
  coinSe(soundEffects[COIN]);
  laserSe(soundEffects[LASER]);
  explosionSe(soundEffects[EXPLOSION]);
  powerUpSe(soundEffects[POWER_UP]);
  hitSe(soundEffects[HIT]);
  jumpSe(soundEffects[JUMP]);
  selectSe(soundEffects[SELECT]);
  randomSe(soundEffects[RANDOM]);
  clickSe(soundEffects[CLICK]);
}

void playSoundEffect(int type) {
  if (!isSoundEnabled) {
    return;
  }
  float ct = md_getAudioTime();
  float qt = ceil(ct / QUANTIZED_DURATION) * QUANTIZED_DURATION;
  if (qt <= soundEffectPlayedTimes[type]) {
    return;
  }
  for (int i = 0; i < MAX_SOUND_EFFECT_NOTE_LENGTH; i++) {
    Note* ns = &soundEffects[type][i];
    if (ns->freq == -1) {
      break;
    }
    md_playTone(ns->freq, ns->duration, qt + ns->when);
  }
  soundEffectPlayedTimes[type] = qt;
}

#define MAX_BGM_NOTE_LENGTH 64
Note[MAX_BGM_NOTE_LENGTH + 1] bgm;
int bgmNoteLength = 32;
float bgmDuration;
int bgmIndex;
float bgmTime;

struct Chord {
  int midiNote;
  bool isMinor;
};

// Vircon32 port note: designated initializers ("{.field = value}") are not
// accepted by this compiler - positional initializers are used instead,
// in declaration order (midiNote, isMinor).
Chord[3][4] chords = {
    {
        {0, false},
        {0, false},
        {4, true},
        {9, true},
    },
    {
        {5, false},
        {5, false},
        {2, true},
        {2, true},
    },
    {
        {7, false},
        {7, false},
        {11, true},
        {11, true},
    }};

int[3][3] nextChordsIndex = {
    {0, 1, 2},
    {1, 2, 0},
    {2, 0, 0},
};

int[7] keys = {0, 2, 3, 5, 7, 9, 10};
int[2][4] progression = {{0, 4, 7, 11}, {0, 3, 7, 10}};

void generateChordProgression(int[4]* midiNotes, int len) {
  int key = keys[_rndi(0, 7)];
  int octave = 3;
  int chordChangeInterval = 4;
  Chord chord;
  int chordIndex;
  for (int i = 0; i < len; i++) {
    if (i % chordChangeInterval == 0) {
      if (i == 0) {
        chordIndex = _rndi(0, 2);
        chord = chords[chordIndex][_rndi(0, 4)];
      } else if (_rnd(0, 1) < 0.8 - ((i / chordChangeInterval) % 2 * 0.5)) {
        chordIndex = nextChordsIndex[chordIndex][_rndi(0, 3)];
        chord = chords[chordIndex][_rndi(0, 4)];
      }
    }
    int progIndex;
    if (chord.isMinor) {
      progIndex = 1;
    } else {
      progIndex = 0;
    }
    for (int j = 0; j < 4; j++) {
      midiNotes[i][j] = octave * 12 + 12 + key + chord.midiNote + progression[progIndex][j];
    }
  }
}

void reversePattern(bool* pattern, int interval, int len, int freq) {
  bool[MAX_BGM_NOTE_LENGTH] pt;
  for (int i = 0; i < interval; i++) {
    pt[i] = false;
  }
  for (int i = 0; i < freq; i++) {
    pt[_rndi(0, interval)] = true;
  }
  for (int i = 0; i < len; i++) {
    if (pt[i % interval]) {
      pattern[i] = !pattern[i];
    }
  }
}

void createRandomPattern(bool* pattern, int len, int freq) {
  int interval = 4;
  for (int i = 0; i < len; i++) {
    pattern[i] = false;
  }
  while (interval < len) {
    reversePattern(pattern, interval, len, freq);
    interval *= 2;
  }
}

void generateBgm() {
  int noteLength = bgmNoteLength;
  int[MAX_BGM_NOTE_LENGTH][4] chordMidiNotes;
  generateChordProgression(chordMidiNotes, noteLength);
  bool[MAX_BGM_NOTE_LENGTH] pattern;
  createRandomPattern(pattern, noteLength, 1);
  bool[MAX_BGM_NOTE_LENGTH] continuingPattern;
  for (int i = 0; i < noteLength; i++) {
    continuingPattern[i] = _rnd(0, 1) < 0.8;
  }
  float duration = bgmDuration;
  float when = -duration;
  bool hasPrevNote = false;
  int bgmNoteIndex = 0;
  for (int i = 0; i < noteLength; i++) {
    when += duration;
    if (!pattern[i]) {
      hasPrevNote = false;
      continue;
    }
    if (continuingPattern[i] && hasPrevNote) {
      bgm[bgmNoteIndex - 1].duration += duration;
      continue;
    }
    hasPrevNote = true;
    int mn = chordMidiNotes[i][_rndi(0, 4)];
    bgm[bgmNoteIndex].freq = midiNoteToFreq(mn);
    bgm[bgmNoteIndex].when = when;
    bgm[bgmNoteIndex].duration = duration;
    bgmNoteIndex++;
  }
  bgm[bgmNoteIndex].freq = -1;
}

int getHashFromString(int* str) {
  int hash = 0;
  int len = strlen(str);
  for (int i = 0; i < len; i++) {
    int chr = str[i];
    hash = (hash << 5) - hash + chr;
    hash |= 0;
  }
  return hash;
}

void initSound(int* title, int* description, int soundSeed) {
  isPlayingBgm = false;
  int[99] randomSeedStr;
  strncpy(randomSeedStr, title, 98);
  strncat(randomSeedStr, description, 98);
  int seed = getHashFromString(randomSeedStr) + soundSeed;
  setRandomSeed(&soundRandom, seed);
  bgmIndex = 0;
  bgmDuration = BASE_NOTE_DURATION * 8;
  generateSoundEffect();
  memset(soundEffectPlayedTimes, 0, sizeof(soundEffectPlayedTimes));
  generateBgm();
  float ct = md_getAudioTime();
  bgmTime = ceil(ct / QUANTIZED_DURATION) * QUANTIZED_DURATION;
}

void updateSound() {
  if (!isSoundEnabled || !isPlayingBgm) {
    return;
  }
  float ct = md_getAudioTime();
  Note* bn = &bgm[bgmIndex];
  float bt = bgmTime + bn->when;
  if (ct > bt) {
    bgmIndex = 0;
    bgmTime = ceil(ct / QUANTIZED_DURATION) * QUANTIZED_DURATION;
  } else if (bt < ct + 1.0 / FPS * 2) {
    md_playTone(bn->freq, bn->duration, bt);
    bgmIndex++;
    if (bgm[bgmIndex].freq == -1) {
      bgmIndex = 0;
      bgmTime += bgmNoteLength * bgmDuration;
    }
  }
}
