StaticJsonDocument<96> data;

void serialInit() {
  Serial1.swap(0);
  Serial1.begin(9600); //RX: 1, TX: 0

  Serial2.swap(0);
  Serial2.begin(9600); //RX: 7, TX: 2

  Serial3.swap(0);
  Serial3.begin(9600); //RX: 3, TX:6
}

void establishContact() {
  static unsigned long t;

  if (!module_status.initialized && millis() - t > 300) {
    sendReq(Serial1);

    t = millis();
  }
}

void ModuleLiveCheckRoutine() {
  if (module_status.completeInit &&
      module_status.moduleLivePrevious != module_status.moduleLiveSignal &&
      millis() - module_status.moduleLiveSentTime > LIVE_DETECT_INTERVAL) {
    module_status.moduleLivePrevious = module_status.moduleLiveSignal;
    sendReq(Serial1);
  }
}

void receiveSerial1() {
  static UART_MSG_RC_STATE state = RC_NONE;

  static int length;
  static char buffer[128];
  static int buffer_pos;

  switch (uartReceive(Serial1, state, length, buffer, buffer_pos)) {
    case CMD_LOAD_MODULE:
      if (length < 1) return;

      led.setOffSingle();

      uartTransmit(Serial2, CMD_HI);
      module_status.addr = (int)buffer[1] + 1; //update self-addr as the increasement of the previous addr

      if (module_config.initialized != 0x01) {
        strcpy(module_config.name, "Switch_");
        strcat(module_config.name, String(module_status.addr).c_str());

        module_config.priority = module_status.addr;
        module_config.type = 1; //American plug
        module_config.initialized = 0x01;
      }

      delay(100);

      if (Serial2.available() == 0) { //last module
        data["total"] = module_status.addr;
        data["addr"] = module_status.addr;
        data["pri"] = module_config.priority;
        data["switch_state"] = (int)module_config.switchState;
        data["name"] = module_config.name;

        int l = measureJson(data);
        char *p = (char*)malloc(l * sizeof(char));
        serializeJson(data, p, l);

        uart_msg_pack* pack = new uart_msg_pack(CMD_LINK_MODULE, l, p);
        uartTransmit(Serial1, pack);
        free(p);
      } else {
        clearSerial(Serial2);
        sendAddress(Serial2);
      }

      module_status.initialized = true;

#if DEBUG
      Serial.println("[UART] Module status: initialized");
      Serial.print("[UART] Module addr: ");
      Serial.println(module_status.addr);
#endif
      break;

    case CMD_HI:
      uartTransmit(Serial1, CMD_HI);
      break;
  }

}

void receiveSerial2() {
  if (!module_status.initialized) return;

  static UART_MSG_RC_STATE state = RC_NONE;

  static int length;
  static int buffer_pos;
  static char buffer[128];

  switch (uartReceive(Serial2, state, length, buffer, buffer_pos)) {
    case CMD_REQ_ADR:
      sendAddress(Serial2);
      break;

    case CMD_UPDATE_DATA:
      //sendCmd(Serial1, CMD_UPDATE_DATA, buffer, length);
      break;

    case CMD_HI:
      module_status.moduleLiveSignal = true;
      break;

    case CMD_LINK_MODULE:
      //sendCmd(Serial1, CMD_LINK_MODULE, buffer, length);
      DeserializationError err = deserializeJson(data, buffer);

      if (err == DeserializationError::Ok) {
        if (data["addr"].as<int>() - 1 != module_status.addr) return; //not my turn yet

        data["addr"] = module_status.addr;
        data["pri"] = module_config.priority;
        data["switch_state"] = (int)module_config.switchState;
        data["name"] = module_config.name;

        int l = measureJson(data);
        char *p = (char*)malloc(l * sizeof(char));
        serializeJson(data, p, l);
        //sendCmd(Serial1, CMD_LINK_MODULE, p, l);
        free(p);

        module_status.moduleLiveSignal = true;
        module_status.moduleLivePrevious = true;
      }
      break;
  }
}

void receiveSerial3() {
  static UART_MSG_RC_STATE state = RC_NONE;

  static int length;
  static char buffer[128];
  static int buffer_pos;

  switch (uartReceive(Serial3, state, length, buffer, buffer_pos)) {
    case CMD_REQ_DATA:
      if (length < 1) return;

      switch (buffer[0]) {
        case MODULE_CURRENT:
          sendUpdateData(Serial1, MODULE_CURRENT, (int)module_status.current);
          break;
      }
      break;

    case CMD_HI:
      module_status.moduleLiveSignal = false;

      uartTransmit(Serial1, CMD_HI);

      module_status.moduleLiveSentTime = millis();
      break;

    case CMD_DO_MODULE:
      if (length < 2) return;

      for (int i = 0; i < length / 2; i++) {
        if (buffer[i * 2] != module_status.addr) continue;

        module_status.controlTask = (uint8_t)buffer[i * 2 + 1];
        break;
      }

      break;

    case CMD_INIT_MODULE:
      if (length < 2) return;

      for (int i = 1; i < length; i++) {
        if (buffer[i] != module_status.addr) continue;

        EEPROM.put(MODULE_CONFIG_EEPROM_ADDR, module_config);

        module_status.controlTask = module_config.switchState ? DO_TURN_ON : DO_TURN_OFF;
        module_status.completeInit = true;

        led.setOnSingle();
        break;
      }
      break;

    case CMD_UPDATE_DATA:
      if (length < 4) return;

      for (int i = 0; i < (length - 1) / 3; i++) {
        int a = buffer[i * 3 + 1];
        if (a != module_status.addr && a != 0) continue;

        int value = bytesCombine(buffer[i * 3 + 2], buffer[i * 3 + 3]);

        switch (buffer[0]) {
          case MODULE_MCUB:
            module_status.mcub = value;

#if DEBUG
            Serial.print("[UART] Update MCUB: ");
            Serial.println(module_status.mcub);
#endif
            break;

          case MODULE_PRIORITY:
            module_config.priority = value;
            eepromUpdate(MODULE_CONFIG_EEPROM_ADDR, module_config);

#if DEBUG
            Serial.print("[UART] Update PRIORITY: ");
            Serial.println(module_config.priority);
#endif
            break;
        }
        break;
      }
      break;

    case CMD_RESET_MODULE:
      if (length < 2) return;

      for (int i = 1; i < length; i++) {
        if (buffer[i] != module_status.addr) continue;

        led.setOnSingle();
        module_config.initialized = 0x00;
        eepromUpdate(MODULE_CONFIG_EEPROM_ADDR, module_config);

        module_status.initialized = false;
        module_status.completeInit = false;
        break;
      }
      break;
  }

  taskLoop();
}

void sendReq(Stream &_serial) {
  led.blinkSingle(50);
  uartTransmit(_serial, CMD_REQ_ADR);
}

void sendAddress(Stream &_serial) {
  uart_msg_pack* pack = new uart_msg_pack(CMD_LOAD_MODULE, 1, module_status.addr);
  uartTransmit(_serial, pack);
}

void sendUpdateData(Stream &_serial, char type, int value) {
  char *p = (char *)malloc(4 * sizeof(char));
  
  p[0] = module_status.addr;
  p[1] = type;
  p[2] = value & 0xff;
  p[3] = value >> 8;
  uart_msg_pack* pack = new uart_msg_pack(CMD_UPDATE_DATA, 4, p);
  uartTransmit(_serial, pack);
}

// /*** Foundation ***/
char uartReceive(Stream &_serial, UART_MSG_RC_STATE &state, int &length, char *buffer, int &buffer_pos) {
  Stream* serial = &_serial;

  switch (state) {
    case RC_NONE: {
        if (serial->available() < 1) break;

        uint8_t start_byte = serial->read();

        if (start_byte != CMD_START) {
#if DEBUG
          Serial.print("[UART] Incorrect start byte: ");
          Serial.println(start_byte, HEX);
#endif
          break;
        }

        state = RC_HEADER;
      }

    case RC_HEADER: {
        if (serial->available() < 2) break;

        length = serial->read() & 0xff;
        length |= (uint16_t) serial->read() << 8;

        buffer_pos = 0;

        state = RC_PAYLOAD;
      }

    case RC_PAYLOAD: {
        if (buffer_pos < length && serial->available()) {
          buffer[buffer_pos++] = serial->read();
        }

        if (buffer_pos < length) break;

        state = RC_CHECK;
      }

    case RC_CHECK: {
        if (serial->available() < 2) break;

        uint8_t checksum = serial->read();
        uint8_t eof = serial->read();

        state = RC_NONE;

        if (checksum != calcCRC(buffer, length)) {
#if DEBUG
          Serial.print("[UART] ERROR: Incorrect CRC8: ");
          Serial.println(checksum, HEX);
          break;
#endif
        }

        if (eof != CMD_EOF) {
#if DEBUG
          Serial.print("[UART] ERROR: Unexpected EOF: ");
          Serial.println(eof, HEX);
#endif

          break;
        }

        return buffer[0];
      }
  }

  return CMD_FAIL;
}

void uartTransmit(Stream &_serial, uart_msg_pack* pack) {
  Stream* serial = &_serial;
  char crc = calcCRC(pack);

  serial->write(CMD_START);
  serial->write((pack->length + 1) & 0xff);
  serial->write((pack->length + 1) >> 8 & 0xff);
  serial->write(pack->cmd);

  for (int i = 0; i < pack->length; i++) serial->write(pack->payload[i]);

  serial->write(crc);
  serial->write(CMD_EOF);

  free(pack->payload);
  delete pack;
}
void uartTransmit(Stream &_serial, char cmd) {
  uart_msg_pack *pack = new uart_msg_pack(cmd);
  uartTransmit(_serial, pack);
}

void clearSerial(Stream &_serial) {
  Stream* serial = &_serial;

  while (serial->available() > 0) serial->read();
}
