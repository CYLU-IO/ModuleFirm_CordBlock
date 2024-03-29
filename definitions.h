#ifndef DEFINITION_H
#define DEFINITION_H

#define MAX_CURRENT             1500
#define MAX_MODULES             20

#define LIVE_DETECT_INTERVAL    1000

#define DEBUG                   0

///// EEPROM Address /////
#define  MODULE_CONFIG_EEPROM_ADDR   0x00

///// Pin Setups /////
#define LED_PIN                 4
#define RELAY_PIN               5
#define RST_PIN                 8
#define BUTTON_PIN              10
#define ON_BOARD_LED_PIN        13
#define CURRENT_SENSOR_PIN      A3

///// UART MESSAGE STRUC /////
#define CMD_FAIL                0x11
#define CMD_EOF                 0x20
#define CMD_REQ_ADR             0x41 //'A'
#define CMD_LOAD_MODULE         0x42 //'B'
#define CMD_CONFIRM_RECEIVE     0x43 //'C'
#define CMD_DO_MODULE           0x44 //'D'
#define CMD_REQ_DATA            0x45 //'E'
#define CMD_HI                  0x48 //'H'
#define CMD_INIT_MODULE         0x49 //'I'
#define CMD_LINK_MODULE         0x4C //'L'
#define CMD_RESET_MODULE        0x52 //'R'
#define CMD_UPDATE_DATA         0x55 //'U'
#define CMD_START               0xFF

///// Module Actions /////
#define DO_TURN_ON              0x6E //'n'
#define DO_TURN_OFF             0x66 //'f'

///// Characteristic Type /////
#define MODULE_SWITCH_STATE     0x61 //'a' 
#define MODULE_CURRENT          0x62 //'b'
#define MODULE_MCUB             0x63 //'c'
#define MODULE_PRIORITY         0x64 //'d'

///// UART RECEIVE STATE /////
typedef enum UART_MSG_RC_STATE {
  RC_NONE,
  RC_HEADER,
  RC_PAYLOAD,
  RC_CHECK
};

struct uart_msg_pack {
  char cmd;
  int length; //only payload's length
  char *payload;

  uart_msg_pack(char c) : cmd(c), length(0), payload(NULL) {};
  uart_msg_pack(char c, int l, char *p) : cmd(c), length(l), payload(p) {};
};

#endif
