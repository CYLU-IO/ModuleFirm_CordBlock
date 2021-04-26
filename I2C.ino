/***
   Parameters
*/
int id = 0;
int addr = 51; //initial I2C address
bool unique = false;
bool switchStat = false;
int priority = 0;

bool initialzed = false;

/***
   Basic Functions
*/
void i2cManInit() {
  clearEEPROM();
  //If there is a determined address from the previous connection, ues it.
  if (readFromEEPROM(0).length() > 0) addr = readFromEEPROM(0).toInt();

  Wire.begin(addr);
  Wire.setClock(I2C_FREQUENCY);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  randomSeed(analogRead(0));
}

void i2cManLoop() {
  unique = isUnique(addr);
  
  if (id != 0 && unique && !initialzed) { //if the address is unique, then do following
    initialzed = true;
    turnSwitch(HIGH);
    digitalWrite(conncPin, HIGH);
  }
}

/*
   I2C Utilites
*/
void receiveEvent() { //receive from Master
  String reqData, args = "";

  while (0 < Wire.available()) reqData += char(Wire.read());

  if (reqData.length() > 0) {
    args = reqData.substring(1);

    switch (reqData.substring(0, 1).toInt()) {
      case 1: //randomly update address
        randomSeed(analogRead(0));
        updateI2cAddr(random(args.substring(0, 2).toInt(), args.substring(2).toInt() + 1));
        break;

      case 2: //update certained address
        updateI2cAddr(args.toInt());
        break;

      case 3: //register an id
        updateUid(args.toInt());
        break;

      case 4: //rearrange address by checking id
        if (id != args.toInt()) { //not matched
          updateI2cAddr(51);
          id = 0;
        }
        
        break;

      case 5: //turn OFF the switch
        turnSwitch(LOW);
        break;

      case 6: //turn ON the switch
        turnSwitch(HIGH);
        break;
    }
  }
}

void requestEvent() {
  String resString = int2str(addr, 2) + String(unique) + String(switchStat) + String(priority) + int2str(current, 4) + int2str(id, 4);
  byte res[resString.length()];

  for (byte i = 0; i < resString.length(); i++) res[i] = (byte)resString.charAt(i);

  Wire.write(res, sizeof(res));
}

bool isUnique(int target) {
  String res = "";

  Wire.requestFrom(target, 3);

  while (Wire.available()) {
    char b = Wire.read();
    res += b;
  }

  return (res.substring(0).toInt() == target) ? false : true;
}

/*
   Identity Update
*/
void updateI2cAddr(int newAddr) {
  addr = newAddr;
  saveInEEPROM(0, int2str(addr, 2));
  Wire.begin(addr);
  Serial.println("The I2C address has been changed to " + String(addr));
}

void updateUid(int newId) {
  id = newId;
  saveInEEPROM(2, int2str(addr, 4));
  Serial.println("The UID has been changed to " + String(id));
}