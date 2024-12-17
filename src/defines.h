/**************************************************************************************************
  Filename:       defines.h
  Revised:        $Date: 2024-12-12$
  Revision:       $Revision: 01 $
  Description:    board definitions
**************************************************************************************************/

#define SR_OUT_PIN_OE       15          // 595 shift register output enable
#define SR_OUT_PIN_STCP     2           // output latch storage clock
#define SR_OUT_PIN_MR       12          // shift register master reset
#define SR_OUT_PIN_SHCP     14          // shift register serial clock
#define SR_OUT_PIN_SDOUT    27          // serial data out

#define SR_OUT_PWM0         32          // PWM channels
#define SR_OUT_PWM1         33
#define SR_OUT_PWM2         25
#define SR_OUT_PWM3         26

#define SR_IN_PIN_CE        5           // 165 shift register chip enable
#define SR_IN_PIN_CP        18          // clock
#define SR_IN_PIN_PL        19          // parallel load
#define SR_IN_PIN_SDIN      4           // serial data in

#define SR_IN_PIN_AP_SET    34          // net config pin
#define SR_OUT_PIN_AP_LED   13          // net config LED

// bit mask for output shif register 595
#define BIT_CLEAR           0           // 0b0000 0000 0000 0000

#define BIT_OUT_0           0x0080      // 0bx000 0000 1000 0000
#define BIT_OUT_1           0x0040      // 0bx000 0000 0100 0000
#define BIT_OUT_2           0x0020      // 0bx000 0000 0010 0000
#define BIT_OUT_3           0x0010      // 0bx000 0000 0001 0000
#define BIT_OUT_4           0x0008      // 0bx000 0000 0000 1000
#define BIT_OUT_5           0x0004      // 0bx000 0000 0000 0100
#define BIT_OUT_6           0x0002      // 0bx000 0000 0000 0010
#define BIT_OUT_7           0x0001      // 0bx000 0000 0000 0001

#define BIT_ROOF_CLOSE      0x0100      // 0bx000 0001 0000 0000
#define BIT_ROOF_OPEN       0x0200      // 0bx000 0010 0000 0000
#define BIT_CPU_OK          0x0400      // 0b0000 0100 0000 0000
#define BIT_WS_OK           0x0800      // 0b0000 1000 0000 0000
#define BIT_DOME            0x0100      // 0b0001 0000 0000 0000
#define BIT_SWITCH          0x0200      // 0b0010 0000 0000 0000
#define BIT_SAFEMON         0x0400      // 0b0100 0000 0000 0000


// bit mask for input shif register 165
#define BIT_IN_0            0x0001      // 0bxx00 0000 0000 0001
#define BIT_IN_1            0x0002      // 0bxx00 0000 0000 0010
#define BIT_IN_2            0x0004      // 0bxx00 0000 0000 0100
#define BIT_IN_3            0x0008      // 0bxx00 0000 0000 1000
#define BIT_IN_4            0x0010      // 0bxx00 0000 0001 0000
#define BIT_IN_5            0x0020      // 0bxx00 0000 0010 0000
#define BIT_IN_6            0x0040      // 0bxx00 0000 0100 0000
#define BIT_IN_7            0x0080      // 0bxx00 0000 1000 0000

#define BIT_FC_CLOSE        0x0100      // 0bx000 0001 0000 0000
#define BIT_FC_OPEN         0x0200      // 0bx000 0010 0000 0000
#define BIT_BUTTON_OPEN     0x0400      // 0bx000 0100 0000 0000
#define BIT_BUTTON_CLOSE    0x0800      // 0bx000 1000 0000 0000

#define BIT_SAFE_RAIN       0x0100      // 0bxx01 0000 0000 0000
#define BIT_SAFE_POWER      0x0200      // 0bxx10 0000 0000 0000
