enum blinkRoles {ASTEROID, SHIP, DEPOT};
byte blinkRole = ASTEROID;
long millisHold = 0;
long frameMillis;
int oreInterval = 1000;

Timer animTimer;
byte animFrame = 0;

////ASTEROID VARIABLES
bool isMinable = false;
enum oreTypes {COPPER, PLUTONIUM, TUNGSTEN, COBALT};
byte oreType;
Color oreColors [4] = {ORANGE, GREEN, CYAN, MAGENTA};
int oreCountdown;
bool oreLocations[6];
Timer resetTimer;
int resetInterval = 2000;


////SHIP VARIABLES
bool isMining;
byte miningFace = 0;
bool missionComplete;
byte oreTarget;
int oreCollected;

void setup() {
  // put your setup code here, to run once:
  newAsteroid();
  newMission();
}

void loop() {
  //how many millis happened since last frame?
  frameMillis = millis() - millisHold;

  switch (blinkRole) {
    case ASTEROID:
      asteroidLoop();
      asteroidDisplay();
      break;
    case SHIP:
      shipLoop();
      shipDisplay();
      break;
    case DEPOT:
      depotLoop();
      setColor(WHITE);
      break;
  }

  //reset the millisHold
  millisHold = millis();
}

void asteroidLoop() {
  if (buttonLongPressed()) {
    blinkRole = DEPOT;
    newMission();
  }

  //check my neighbors, look for mining ships
  byte miningNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //hey, a neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborRole(neighborData) == SHIP && getShipMining(neighborData)) { //it's a ship, and it's mining
        miningNeighbors++;
      }
    }
  }
  //now we know how many neighbors are mining. Decrement oreCountdown by that amount
  oreCountdown -= frameMillis * miningNeighbors;

  if (oreCountdown < oreInterval) {
    resetTimer.set(resetInterval);
  }

  //also, if there is no ore left, make it
  if (oreCountdown <= 0) {
    isMinable = false;
  }

  //  if (resetTimer.isExpired()) {
  //    newAsteroid();
  //  }

  //set up communication
  byte sendData = (blinkRole << 4) + (oreType << 2) + (isMinable << 1);
  setValueSentOnAllFaces(sendData);
}

void newAsteroid() {
  isMinable = true;
  oreType = rand(3);
  byte oreCount = rand(2) + 1;
  oreCountdown = oreInterval * oreCount;
  //array the ore around the asteroid
  FOREACH_FACE(f) {
    if (f < oreCount) {
      oreLocations[f] = true;
    } else {
      oreLocations[f] = false;
    }
  }
  //shuffle the array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(5);
    byte swapB = rand(5);
    bool temp = oreLocations[swapA];
    oreLocations[swapA] = oreLocations[swapB];
    oreLocations[swapB] = temp;
  }
  resetTimer.set(NEVER);
}

void shipLoop() {
  //first, should we change mode?
  if (buttonLongPressed()) {
    blinkRole = ASTEROID;
  }

  //are we touching an asteroid with a compatible ore type?
  isMining = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //a neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborRole(neighborData) == ASTEROID) {//asteroid!
        if (getAsteroidOreType(neighborData) == oreTarget && getAsteroidMinable(neighborData) == 1) { //the correct ore, and isMinable!
          miningFace = f;
          isMining = true;
        }
      }
    }
  }//end of mining check

  //if we are mining, we need to increment our ore and check for victory
  if (isMining) {
    oreCollected += frameMillis;
  }

  if (oreCollected > oreInterval * 3) {
    isMining = false;
    missionComplete = true;
  }

  //set up communication
  byte sendData = (blinkRole << 4) + (isMining << 3) + (missionComplete << 2);
  setValueSentOnAllFaces(sendData);
}

void newMission() {
  missionComplete = false;
  oreTarget = rand(3);
  oreCollected = 0;
}

void depotLoop() {
  if (buttonLongPressed()) {
    blinkRole = SHIP;
    newAsteroid();
  }
  //set up communication
  byte sendData = (blinkRole << 4);
  setValueSentOnAllFaces(sendData);
}

void shipDisplay() {
  //display the drill
  if (isMining) {
    if (animTimer.isExpired()) {
      setColor(dim(WHITE, 25));
      //setColorOnFace(dim(WHITE, 25), faceAdjust(animFrame));
      animFrame++;
      if (animFrame == 3) {
        animFrame = 0;
      }
      setColorOnFace(WHITE, faceAdjust(animFrame));
      animTimer.set(75);
    }
  } else {
    setColorOnFace(dim(WHITE, 25), faceAdjust(0));//drill left
    setColorOnFace(dim(WHITE, 25), faceAdjust(1));//drill center
    setColorOnFace(dim(WHITE, 25), faceAdjust(2));//drill right
  }

  //display the ore
  setColorOnFace(dim(oreColors[oreTarget], dimnessAdjust(1, oreCollected)), faceAdjust(3));
  setColorOnFace(dim(oreColors[oreTarget], dimnessAdjust(2, oreCollected)), faceAdjust(4));
  setColorOnFace(dim(oreColors[oreTarget], dimnessAdjust(3, oreCollected)), faceAdjust(5));

  if (missionComplete) {
    setColor(YELLOW);
  }
}

byte faceAdjust(byte face) {
  return (((face + 6) + miningFace - 1) % 6);
}

void asteroidDisplay() {
  //run through each face, if it has ore, figure out how much is left there
  byte oreNum = 0;
  FOREACH_FACE(f) {
    if (oreLocations[f] == true) {
      oreNum++;
      setColorOnFace(dim(oreColors[oreType], dimnessAdjust(oreNum, oreCountdown) - 25), f);
    } else {
      setColorOnFace(OFF, f);
    }
  }
  if (oreCollected <= 0) {
    setColor(OFF);
  }
}

byte dimnessAdjust(byte level, int total) {
  byte dimness;
  float multiplier = oreInterval / 230;
  byte fullLights = total / oreInterval;
  byte overflow = total % oreInterval;
  if (fullLights >= level) {//this level is already full
    dimness = 255;
  } else if (fullLights = level - 1) {
    dimness = (overflow * multiplier) + 25;
  } else {
    dimness = 25;
  }
  return dimness;
}

byte getNeighborRole(byte data) {
  return (data >> 4);//the first two bits
}

byte getAsteroidOreType (byte data) {
  return ((data >> 2) & 3);
}

byte getAsteroidMinable (byte data) {
  return ((data >> 1) & 1);
}

byte getShipMining(byte data) {
  return ((data >> 3) & 1);
}

byte getShipMissionComplete(byte data) {
  return ((data >> 2) & 1);
}

