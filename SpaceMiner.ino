enum blinkRoles {ASTEROID, SHIP};
byte blinkRole = ASTEROID;
long millisHold = 0;

Timer animTimer;
byte animFrame = 0;

////ASTEROID VARIABLES
byte oreLayout[6];
byte oreBrightness[6] = {0, 0, 0, 0, 0, 0};
Color oreColors [5] = {OFF, ORANGE, GREEN, CYAN, YELLOW};
Timer resetTimer;
int resetInterval = 3000;
bool isMinable[6];

////SHIP VARIABLES
byte missionCount = 6;
bool missionComplete;
bool gameComplete;
bool isMining[6] = {false, false, false, false, false, false};
byte oreTarget;
byte oreCollected;
int miningTime = 500;
byte displayMissionCompleteColor;
byte displayMissionCompleteIndex = 0;

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
  }
}

void asteroidLoop() {
  if (buttonLongPressed()) {
    blinkRole = SHIP;
    missionCount = 6;
    newMission();
    gameComplete = false;
  }

  //ok, so let's look for ships that need ore
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getBlinkRole(neighborData) == SHIP) { //this neighbor is a ship
        //so here we check to see what our relationship to them is
        if (isMinable[f]) {//this is a face we have offered ore to
          if (getShipMining(neighborData) == 1) {//this ship is mining! we can stop offering!
            isMinable[f] = false;
          }
        } else {//this is a face we COULD offer ore to
          if (oreCheck(getShipTarget(neighborData))) {//this is asking for something we have
            removeOre(getShipTarget(neighborData));
            isMinable[f] = true;
          }
        }
      }
    } else {//no neighbor
      isMinable[f] = false;
    }
  }

  //let's check to see if we should renew ourselves!
  if (resetTimer.isExpired() && isAlone()) {
    updateAsteroid();
    resetTimer.set(rand(1000) + resetInterval);
  }
  //set up communication
  FOREACH_FACE(f) {
    byte sendData = (blinkRole << 4) + (isMinable[f] << 3);
    setValueSentOnFace(sendData, f);
  }
}

void newAsteroid() {
  //clear layout
  FOREACH_FACE(f) {
    oreLayout[f] = 0;
  }
}

bool oreCheck(byte type) {
  bool isAvailable = false;
  FOREACH_FACE(f) {
    if (oreLayout[f] == type) {
      isAvailable = true;
    }
  }
  return (isAvailable);
}

void removeOre(byte type) {
  FOREACH_FACE(f) {
    if (oreLayout[f] == type) {
      oreLayout[f] = 0;
    }
  }
}

void updateAsteroid() {
  byte oreCount = 0;
  FOREACH_FACE(f) {
    if (oreLayout[f] > 0) {
      oreCount++;
    }
  }

  if (oreCount == 0) { //add two
    oreLayout[findEmptySpot()] = findNewColor();
    oreLayout[findEmptySpot()] = findNewColor();
  } else if (oreCount == 4) { //remove one
    oreLayout[findFullSpot()] = 0;
  } else {//add one
    oreLayout[findEmptySpot()] = findNewColor();
  }
}

byte findNewColor() {
  byte newColor;

  bool usedColors[5] = {false, false, false, false, false};
  //run through each face and mark that color as used in the array
  FOREACH_FACE(f) {
    usedColors[oreLayout[f]] = true;
  }

  //now do the shuffle search doodad
  byte searchOrder[5] = {0, 1, 2, 3, 4};
  //shuffle array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(4);
    byte swapB = rand(4);
    byte temp = searchOrder[swapA];
    searchOrder[swapA] = searchOrder[swapB];
    searchOrder[swapB] = temp;
  }

  for (byte i = 0; i < 5; i++) {
    if (usedColors[searchOrder[i]] == false) { //this color is not used
      newColor = searchOrder[i];
    }
  }

  return newColor;
  //return 1;
}

byte findEmptySpot() {
  byte emptyFace;
  byte searchOrder[6] = {0, 1, 2, 3, 4, 5};
  //shuffle array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(5);
    byte swapB = rand(5);
    byte temp = searchOrder[swapA];
    searchOrder[swapA] = searchOrder[swapB];
    searchOrder[swapB] = temp;
  }

  //now do the search in this order. The last 0 you find is the empty spot we are going to return
  FOREACH_FACE(f) {
    if (oreLayout[searchOrder[f]] == 0) {
      emptyFace = searchOrder[f];
    }
  }

  return emptyFace;
}

byte findFullSpot() {
  byte fullFace;
  byte searchOrder[6] = {0, 1, 2, 3, 4, 5};
  //shuffle array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(5);
    byte swapB = rand(5);
    byte temp = searchOrder[swapA];
    searchOrder[swapA] = searchOrder[swapB];
    searchOrder[swapB] = temp;
  }

  //now do the search in this order. The last 0 you find is the empty spot we are going to return
  FOREACH_FACE(f) {
    if (oreLayout[searchOrder[f]] > 0) {
      fullFace = searchOrder[f];
    }
  }

  return fullFace;
}

void shipLoop() {
  if (buttonLongPressed()) {
    blinkRole = ASTEROID;
    newAsteroid();
  }

  //let's look for asteroids that want to trade
  //only do it if we are allowed to
  if (!missionComplete) {
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getBlinkRole(neighborData) == ASTEROID) {//an asteroid!
          //so, what is my relationship to this asteroid?
          if (isMining[f]) { //I'm already mining here
            if (getAsteroidMinable(neighborData) == 0) {//he's done, so I'm done
              isMining[f] = false;
            }
          } else {//I could be mining here if they'd let me
            if (getAsteroidMinable(neighborData) == 1) {//ore is available, take it
              oreCollected++;
              isMining[f] = true;
            }
          }
        }
      } else {//no neighbor
        isMining[f] = false;
      }
    }

    //finally, check to see if the mission is complete
    if (oreCollected == missionCount) {
      missionComplete = true;
      displayMissionCompleteColor = oreTarget;
      displayMissionCompleteIndex = 0;
      resetTimer.set(0);
      oreTarget = 7;//an ore that doesn't exist
      //oh, also, is gameComplete?
      if (missionCount == 1) {
        gameComplete = true;
      }
    }
  }

  //new mission time?
  if (buttonDoubleClicked()) {
    if (!gameComplete) { //only do this if the game isn't over
      if (missionComplete) {
        missionCount--;
        newMission();
      } else {
        newMission();
      }
    }

  }



  //set up communication
  FOREACH_FACE(f) {
    byte sendData = (blinkRole << 4) + (isMining[f] << 3) + (oreTarget);
    setValueSentOnFace(sendData, f);
  }

}

void newMission() {
  missionComplete = false;
  oreTarget = rand(3) + 1;
  oreCollected = 0;
}

void shipDisplay() {
  if (gameComplete) { //big fancy celebration!
    if (resetTimer.isExpired()) {
      //randomly color all faces
      FOREACH_FACE(f) {
        setColorOnFace(oreColors[rand(3) + 1], f);
      }
      resetTimer.set(75);
    }
  }
  else if (missionComplete) { //small celebration

    // Wipe animation
    // TODO: Make this rotate one space at a time, it currently rotates 2 spaces and we have no idea why. Do you know why?
    if (resetTimer.isExpired()) { // add a new face for the next color

      // every 80 ms
      resetTimer.set(80);

      // increment index (0-11)
      displayMissionCompleteIndex = (displayMissionCompleteIndex + 1) % ( 6 * 7 );


      if (((displayMissionCompleteIndex/7) % 2) == 0) {
        setColorOnFace(oreColors[displayMissionCompleteColor], displayMissionCompleteIndex % 6);
      }
      else {
        setColorOnFace(WHITE, displayMissionCompleteIndex % 6);
      }
    }

  }
  else {//just display ore
    FOREACH_FACE(f) {
      if (missionCount > f) {
        if (oreCollected > f) {
          // show the ore brightly acquired
          setColorOnFace(oreColors[oreTarget], f);
        } else {
          // show the ore is needed here
          if (!resetTimer.isExpired()) {
            if (resetTimer.getRemaining() > 900 ) {
              setColorOnFace(oreColors[oreTarget], f);
            }
            else if( resetTimer.getRemaining() <= 900 && resetTimer.getRemaining() > 800 ) {
              setColorOnFace(dim(oreColors[oreTarget], 128), f);
            }
            else if( resetTimer.getRemaining() <= 800 && resetTimer.getRemaining() > 700 ) {
              setColorOnFace(oreColors[oreTarget], f);
            }
            else if( resetTimer.getRemaining() <= 700 && resetTimer.getRemaining() > 0 ) {
              setColorOnFace(dim(oreColors[oreTarget], 128), f);
            }
          }
          else {
            resetTimer.set(1000);
          }
        }
      } else {
        setColorOnFace(OFF, f);
      }
    }
  }
}

void asteroidDisplay() {
  //run through each face and check on the brightness
  FOREACH_FACE(f) {
    if (oreLayout[f] > 0) { //should be ore here
      if (oreBrightness[f] < 255) { //hey, this isn't full brightness yet
        oreBrightness[f] += 5;
      }
    } else {//should not be ore here
      if (oreBrightness[f] > 0) { //hey, this isn't off
        oreBrightness[f] -= 5;
      }
    }
  }

  FOREACH_FACE(f) {
    Color displayColor = oreColors[oreLayout[f]];
    byte displayBrightness = oreBrightness[f];
    if (oreLayout[f] == 0 && displayBrightness > 0) { //a fading ore thing
      displayColor = WHITE;
    }
    setColorOnFace(dim(displayColor, displayBrightness), f);
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

