//----------------------------------------------
// DEBUG
// #define DEBUG_MODE

#ifdef DEBUG_MODE
  // Debug pin
  #define DEBUG_pin 7
#endif
//----------------------------------------------


//----------------------------------------------
// Which Timer will be used
//#define TIMER_1
//#define TIMER_2
//----------------------------------------------


//----------------------------------------------
// Shift register pins
// Pins connected to 74HC595

// SH_CP = clock / 74HC595 pin no 11
#define SH_CP_clock_pin 8

// ST_CP = latch / 74HC595 pin no 12
#define ST_CP_latch_pin 9

// DS = data / 74HC595 pin no 14
#define DS_data_pin 10
//----------------------------------------------


//----------------------------------------------
// brightness & LEDs
#define BRIGHTNESS_LEVELS 25
#define MAX_BRIGHTNESS BRIGHTNESS_LEVELS-1

/*
// LED brightness adjusted to human eye
//                            0   1   2   3   4    5    6    7    8    9   10   11
//                            |   |   |   |   |    |    |    |    |    |    |    |
const uint8_t LED_b[]   = { 0, 21, 42, 63, 85, 106, 127, 148, 170, 191, 212, 233 };
const uint8_t LED_eye[] = { 0,  1,  2,  3,  6,  10,  20,  36,  66, 120, 218, 254 };
*/

// 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
// 0 1   3     6          11                         20          24
byte hEye[] = { 0, 1, 3, 6, 11, 20, 24 };

#define LED_NUMBER 8

//                         3        1        0        1        3        6       11       20
// 8 LEDs, brightness      |        |        |        |        |        |        |        |
// used to show LEDs       |        |        |        |        |        |        |        |
int b[LED_NUMBER] = { hEye[0], hEye[1], hEye[2], hEye[3], hEye[4], hEye[5], hEye[6], hEye[7] };
// 8 LEDs, brightness gradient
// used only in loop() to change b[] values
int d[LED_NUMBER] = {     -1,      -1,       1,       1,       1,       1,       1,       1 };

// @todo: remove LED_PATTERN, should use only brightness
//                    <- x --><- R --><- G --><- B -->
#define LED_PATTERN 0b11111111111111111111111111111111
//----------------------------------------------


//----------------------------------------------
// PWM cycle (counter)
// 0...MAX_BRIGHTNESS
int cyclePWM = 0;
//----------------------------------------------


//----------------------------------------------
// setup
void setup() {

  Serial.begin(9600);

  Serial.println(F( "" ));
  Serial.println(F( "startup begin" ));

  // set pins to output, they are addressed in the main loop
  pinMode( ST_CP_latch_pin, OUTPUT );
  pinMode( SH_CP_clock_pin, OUTPUT );
  pinMode( DS_data_pin, OUTPUT );
  digitalWrite( ST_CP_latch_pin, HIGH );
  digitalWrite( SH_CP_clock_pin, LOW );
  digitalWrite( DS_data_pin, LOW );

  #ifdef DEBUG_MODE
    // debug pin
    pinMode( DEBUG_pin, OUTPUT );
    digitalWrite( DEBUG_pin, LOW );
  #endif
  
  // set up interrupts
  setUpInterrupts();
  
  Serial.println(F( "startup end" ));
}


#if defined(TIMER_1) || defined(TIMER_2)
  //----------------------------------------------
  // interrupt service body
  // @todo: do not use LED_PATTERN, use brightness level only
  void interruptBody() {
    #ifdef DEBUG_MODE
      digitalWrite( DEBUG_pin, HIGH );
    #endif
    
    showAtBrightness( LED_PATTERN );
  
    #ifdef DEBUG_MODE
      digitalWrite( DEBUG_pin, LOW );
    #endif  
  }
#endif


#ifdef TIMER_1
  //----------------------------------------------
  // interrupt service routine Timer1
  ISR( TIMER1_COMPA_vect ) {
    interruptBody();
  }
#endif


#ifdef TIMER_2
  //----------------------------------------------
  // interrupt service routine Timer2
  ISR( TIMER2_COMPA_vect ) {
    interruptBody();
  }
#endif


//----------------------------------------------
// shiftOutFast by Marcin Gosiewski
// changed to use long (4 bytes) - to steer 4 x shift register
// @todo: use digitalWriteFast
void shiftOutFast4( long val ) {
  uint8_t i;
  long b = val;
  
  for ( i = 0; i < 32; i++ ) {
    Serial.print( (b&1) );
    digitalWrite( DS_data_pin, (b&1) );
    b >>= 1;
    delayMicroseconds(2);
    digitalWrite( SH_CP_clock_pin, HIGH );
    delayMicroseconds( 2 );
    digitalWrite( SH_CP_clock_pin, LOW );       
  }
  Serial.println();
}


//----------------------------------------------
// send data to 4 x 74HC595
void sendDataToRegister( long d ) {
  digitalWrite( ST_CP_latch_pin, LOW );
  shiftOutFast4( d );
  digitalWrite( ST_CP_latch_pin, HIGH );
}

#define PRESCALER 64
#define FREQUENCY 2000

//----------------------------------------------
// set up interrupts
void setUpInterrupts(  ) {

  // disable all interrupts
  noInterrupts();
  
  #ifdef TIMER_1
    Serial.println(F( "T1 - initialize" ));
    //---------------------------
    // timer1
    {
      TCCR1A = 0;
      TCCR1B = 0;
      TCNT1  = 0; // Timer1 counter
    
      //                        prescaler
      //                               |   frequency
      //                               |    |
      //                               v    v
      // compare match register 16MHz/64/2000Hz
      OCR1A   = 16000000 / PRESCALER / FREQUENCY - 1;
      TCCR1B |= (1 << WGM12);  // CTC mode
      
      switch (PRESCALER) {
        case    1:  TCCR1B &= ~(1 << CS12); TCCR1B &= ~(1 << CS11); TCCR1B |=  (1 << CS10); break; // 001
        case    8:  TCCR1B &= ~(1 << CS12); TCCR1B |=  (1 << CS11); TCCR1B &= ~(1 << CS10); break; // 010
        case   64:  TCCR1B &= ~(1 << CS12); TCCR1B |=  (1 << CS11); TCCR1B |=  (1 << CS10); break; // 011
        case  256:  TCCR1B |=  (1 << CS12); TCCR1B &= ~(1 << CS11); TCCR1B &= ~(1 << CS10); break; // 100
        case 1024:  TCCR1B |=  (1 << CS12); TCCR1B &= ~(1 << CS11); TCCR1B |=  (1 << CS10); break; // 101
        default:    TCCR1B &= ~(1 << CS12); TCCR1B &= ~(1 << CS11); TCCR1B &= ~(1 << CS10); break; // 000
      }
      
      TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
    }
    // timer 1 initialized
    //---------------------------
    Serial.println(F( "T1 - done" ));
  #endif

  
  #ifdef TIMER_2
    Serial.println(F( "T2 - initialize" ));
    //---------------------------
    // timer2
    {
      TCCR2A = 0;
      TCCR2B = 0;
      TCNT2  = 0; // Timer2 counter
    
      //                        prescaler
      //                               |   frequency
      //                               |    |
      //                               v    v
      // compare match register 16MHz/64/2000Hz
      OCR2A   = 16000000 / PRESCALER / FREQUENCY - 1;
      TCCR2B |= (1 << WGM22);  // CTC mode
      
      switch (PRESCALER) {
        case    1:  TCCR2B &= ~(1 << CS22); TCCR2B &= ~(1 << CS21); TCCR2B |=  (1 << CS20); break; // 001
        case    8:  TCCR2B &= ~(1 << CS22); TCCR2B |=  (1 << CS21); TCCR2B &= ~(1 << CS20); break; // 010
        case   64:  TCCR2B &= ~(1 << CS22); TCCR2B |=  (1 << CS21); TCCR2B |=  (1 << CS20); break; // 011
        case  256:  TCCR2B |=  (1 << CS22); TCCR2B &= ~(1 << CS21); TCCR2B &= ~(1 << CS20); break; // 100
        case 1024:  TCCR2B |=  (1 << CS22); TCCR2B &= ~(1 << CS21); TCCR2B |=  (1 << CS20); break; // 101
        default:    TCCR2B &= ~(1 << CS22); TCCR2B &= ~(1 << CS21); TCCR2B &= ~(1 << CS20); break; // 000
      }
      
      TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt
    }
    // timer 2 initialized
    //---------------------------
    Serial.println(F( "T2 - done" ));
  #endif

  
  // enable all interrupts
  interrupts();
}


//----------------------------------------------
// Shows pattern (d) using brightness per LED
// @todo: this method using LED_PATTERN should not be used, method using only brightness should be used instead
// 4 x 8 bits
void showAtBrightness( long d ) {

  for ( int i = 0; i < LED_NUMBER; i++ ) {
    
    /*
    // common cathode (-)
    if ( cyclePWM < b[i] ) {
      // show (only if 1)
      d |= ( ( 1 << i ) & d );
    } else {
      // do not show regardless if 0 or 1
      d &= ~( 1 << i );
    }
    */

    // common anode (+)
    if ( cyclePWM < b[i] ) {
      // do not show regardless if 0 or 1
      d &= ~( 1 << i );
    } else {
      // show (only if 1)
      d |= ( ( 1 << i ) & d );
    }
    
  }
  
  sendDataToRegister( d );
  
  // next PWM cycle
  if ( ++cyclePWM > MAX_BRIGHTNESS ) {
    cyclePWM = 0;
  }
  
}


//  Matrix
//    01234567  <-- x, (+), 1 show, 0 do not show
//  7 xxxxxxxx
//  6 xxxxxxxx
//  5 xxxxxxxx
//  4 xxxxxxxx
//  3 xxxxxxxx
//  2 xxxxxxxx
//  1 xxxxxxxx
//  0 xxxxxxxx
//  ^
//  |
//  y (colors), (-), 0 show, 1 do not show
// 
// rgb --> r
//  gb --> g
//   b --> b
//                          common anode
//  <- - --><- - --><- - --><- + -->
//  <- x --><- x --><- x --><- y -->
//  <- R --><- B --><- G -->
long LED[] = {
  /*
  0b00000000000000000000000000000000, // off
  0b00000000111111111111111111111111, // red
  0b11111111111111110000000011111111, // green
  0b11111111000000001111111111111111, // blue
  */
//  <- R --><- B --><- G -->
  0b00000001111111111111111111111110, // red
  0b11111111111111110000001011111101, // green
  0b11111111000001001111111111111011, // blue
  0b00001000111111111111111111110111, // red
  0b11111111111111110001000011101111, // green
  0b11111111001000001111111111011111, // blue
  0b01000000111111111111111110111111, // red
  0b11111111111111111000000001111111,  // green
  0b01000000111111111111111110111111, // red
  0b11111111001000001111111111011111, // blue
  0b11111111111111110001000011101111, // green
  0b00001000111111111111111111110111, // red
  0b11111111000001001111111111111011, // blue
  0b11111111111111110000001011111101  // green
};
//  <- R --><- B --><- G -->

#define DELAY 200

//----------------------------------------------
// loop
void loop() {

  for ( int i = 0; i < sizeof(LED) / sizeof(long); i++ ) {
    sendDataToRegister( LED[i] );
    delay( DELAY );
  }
  
  
  /*
  // for each LED
  for ( int i = 0; i < LED_NUMBER; ++i ) {
    Serial.print( b[i] );
    Serial.print( "." );

    // set brightness usong human eye levels
    // @todo: use hEye[]
    // 0 1 3 6 11 20 24
    if ( d[i] == 1 ) {
      switch( b[i] ) {
        case  0: b[i] =  1; break;
        case  1: b[i] =  3; break;
        case  3: b[i] =  6; break;
        case  6: b[i] = 11; break;
        case 11: b[i] = 20; break;
        case 20: b[i] = 24; break;
        case 24: b[i] = 20; d[i] = -1; break;
        default: break;
      }
    } else if ( d[i] == -1 ) {
      switch( b[i] ) {
        case 24: b[i] = 20; break;
        case 20: b[i] = 11; break;
        case 11: b[i] =  6; break;
        case  6: b[i] =  3; break;
        case  3: b[i] =  1; break;
        case  1: b[i] =  0; break;
        case  0: b[i] =  1; d[i] = 1; break;
        default: break;
      }
    }
       
  }
  Serial.println();
    
  delay( 500 );
  */
}
