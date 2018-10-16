enum blinkRoles {ASTEROID, SHIP, DEPOT};
byte blinkRole = ASTEROID;
long millisHold = 0;

Timer animTimer;
byte animFrame = 0;

////ASTEROID VARIABLES
byte oreLayout[6];
Color oreColors [5] = {OFF, ORANGE, GREEN, CYAN, YELLOW};
Timer resetTimer;
int resetInterval = 4000;
bool isMinable[6];

////SHIP VARIABLES
byte miningFace = 0;
byte missionCount = 3;
bool missionComplete;
bool isMining;
byte oreTarget;
byte oreCollected;
Timer canMine;
int miningTime = 500;

void setup() {
  // put your setup code here, to run once:
  newAsteroid();
  newMission();
}

void loop() {
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
      setColor(dim(WHITE, 25));
      break;
  }
}

void asteroidLoop() {
  if (buttonLongPressed()) {
    blinkRole = DEPOT;
    newMission();
  }

  //ok, so I'm hanging out, and a blink touches me. is it a ship asking for a thing I have?
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getBlinkRole(neighborData) == SHIP) { //a ship!
        resetTimer.set(resetInterval);
        byte oreRequest = getShipTarget(neighborData);//what does it want?
        FOREACH_FACE(ff) {//do I have one?
          if (oreLayout[ff] == oreRequest) {//yes
            setColorOnFace(WHITE, ff);
            isMinable[f] = true;//that face says "hey, I have one of those. take it"
          }
        }

        //now, if we are minable, check if that ship is currently mining
        if (isMinable[f]) {
          if (getShipMining(neighborData) == 1) { //oh that ship is mining. Our work is done!
            isMinable[f] = false;
            FOREACH_FACE(fff) {
              if (oreLayout[fff] == oreRequest) {
                oreLayout[fff] = 0;//remove that ore
              }
            }
          }
        }
      } else {
        isMinable[f] = false;
      }//end of found ship check
    } else {
      isMinable[f] = false;
    }//end of found neighbor check
  }//end of face check


  //let's check to see if we should renew ourselves!
  if (resetTimer.isExpired()) {
    newAsteroid();
    resetTimer.set(resetInterval);
  }
  //set up communication
  FOREACH_FACE(f) {
    byte sendData = (blinkRole << 4) + (isMinable[f] << 3);
    setValueSentOnFace(sendData, f);
  }
}

void newAsteroid() {
  //default layout
  oreLayout[0] = 1;
  oreLayout[1] = 2;
  oreLayout[2] = 3;
  oreLayout[3] = 4;
  oreLayout[4] = 0;
  oreLayout[5] = 0;

  //remove one or two of them
  oreLayout[rand(4)] = 0;

  //shuffle array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(5);
    byte swapB = rand(5);
    byte temp = oreLayout[swapA];
    oreLayout[swapA] = oreLayout[swapB];
    oreLayout[swapB] = temp;
  }
}

void shipLoop() {
  if (buttonLongPressed()) {
    blinkRole = ASTEROID;
    newAsteroid();
  }

  //ok, so are we allowed to look around for mining purposes?
  if (canMine.isExpired()) {
    isMining = false;
    //look at neighbors for an asteroid that is minable
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getBlinkRole(neighborData) == ASTEROID) {//an asteroid!
          if (getAsteroidMinable(neighborData) == 1) {
            //we found one we can mine!
            isMining = true;
            oreCollected++;
            canMine.set(miningTime);
          }
        }
      }
    }
  }

  if (oreCollected >= missionCount) {
    missionComplete = true;
  }

  if (buttonDoubleClicked()) {
    newMission();
  }

  //set up communication
  byte sendData = (blinkRole << 4) + (isMining << 3) + (oreTarget);
  setValueSentOnAllFaces(sendData);
}

void newMission() {
  missionComplete = false;
  oreTarget = rand(3) + 1;
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
  //just display ore collected
  FOREACH_FACE(f) {
    if (oreCollected > f) {
      setColorOnFace(oreColors[oreTarget], f);
    } else {
      setColorOnFace(dim(oreColors[oreTarget], 25), f);
    }
  }

  if (!canMine.isExpired()) { //currently mining
    setColorOnFace(WHITE, oreCollected);
  }

  if (missionComplete) {
    setColor(WHITE);
  }
}

void asteroidDisplay() {
  FOREACH_FACE(f) {
    Color displayColor = oreColors[oreLayout[f]];
    setColorOnFace(displayColor, f);
    if (isMinable[f]) {
      setColorOnFace(WHITE, f);
    }
  }
}

byte getBlinkRole(byte data) {
  return (data >> 4);//the first two bits
}

byte getShipTarget(byte data) {
  return (data & 7);//the last three bits
}

byte getShipMining(byte data) {
  return ((data >> 3) & 1);//just the third bit
}

byte getAsteroidMinable(byte data) {
  return ((data >> 3) & 1);//just the third bit
}

