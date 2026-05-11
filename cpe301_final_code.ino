// Maya Muha and Katie Mewes

#include <Servo.h>
#include <RTClib.h>

RTC_DS3231 rtc;

Servo motor;

int state = 0;

#define RDA 0x80
#define TBE 0x20  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

// define port register pointrs
unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;
volatile unsigned char* pin_h = (unsigned char*) 0x100;

unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* port_b = (unsigned char*) 0x25;

unsigned char* ddr_e = (unsigned char*) 0x2D;
unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* pin_e = (unsigned char*) 0x2C;

// define stuff for adc
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Timer Pointers 1
volatile unsigned char *myTCCR1A  = (volatile unsigned char *) 0x80;
volatile unsigned char *myTCCR1B  = (volatile unsigned char *) 0x81;
volatile unsigned char *myTCCR1C  = (volatile unsigned char *) 0x82;
volatile unsigned char *myTIMSK1  = (volatile unsigned char *) 0x6F;
volatile unsigned char *myTIFR1   = (volatile unsigned char *) 0x36;
volatile unsigned int  *myTCNT1   = (volatile unsigned int *)  0x84;
// global ticks counter
unsigned int currentTicks = 65535;
unsigned char timer_running = 0;

//millis setup
unsigned long previousMillis = 0; 


void setup() {
  U0init(9600);
  adc_init();
  setup_timer_regs();

  //actuator: motor
  motor.attach(11);
  motor.write(0);
  
  // rtc setup
  rtc.begin();
  
  // red led (error state)
  *ddr_h |= 0x01 << 4;
  // green led (active state)
  *ddr_h |= 0x01 << 5;
  // yellow led (idle state)
  *ddr_h |= 0x01 << 6;
  //blue led (off state)
  *ddr_b |= 0x01 << 4;
  
  // main led
  *ddr_b |= 0x01 << 6;

  // on button
   *ddr_e &= ~(0x01 << 5); 
   *port_e |= 0x01 << 5;

  //off button
   *ddr_e &= ~(0x01 << 3);
   *port_e |= 0x01 << 3;

  //reset button
   *ddr_h &= ~(0x01 << 3);
   *port_h |= 0x01 << 3;

}

void loop() {
  //rtc values to char values for printing
  DateTime now = rtc.now();
  uint8_t hour = now.hour();
  uint8_t min = now.minute();
  uint8_t second = now.second();
  char hourChar[3];
  char minChar[3];
  char secChar[3];
  itoa(hour, hourChar, 10);
  itoa(min, minChar, 10);
  itoa(second, secChar, 10);

  //interrupt implementation
  attachInterrupt(digitalPinToInterrupt(3), sendStateOne, FALLING);

  //char arrays for printing
  unsigned long currentMillis = millis();
  char idle[] = "IDLE state entered";
  char off_state[] = "OFF state entered";
  char active[] = "ACTIVE state entered";
  char error[] = "ERROR state entered";
  char motor_on[] = "Motor 90 position";
  char motor_off[] = "Motor 0 position";

// off state
 if (state == 0){
  *port_h &= ~(0x01 << 4);
  *port_h &= ~(0x01 << 5);
  *port_h &= ~(0x01 << 6);
  *port_b |= 0x01 << 4;
  
  *port_b &= ~(0x01 << 6);
  motor.write(0);
 }

// prints idle state entered + RTC time
 if (!(*pin_e & 0x20)){
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    U0putchar(hourChar[0]);
    U0putchar(hourChar[1]);
    U0putchar(':');
    U0putchar(minChar[0]);
    U0putchar(minChar[1]);
    U0putchar(':');
    U0putchar(secChar[0]);
    U0putchar(secChar[1]);
    U0putchar('~');
    for (int i = 0; i < 18; i++){
     U0putchar(idle[i]);
    }
    U0putchar('\n');
   }
 }

 // if reset button is pressed
 if (!(*pin_h & 0x8)){
  state = 1;

  //prints idle state entered + RTC time
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    U0putchar(hourChar[0]);
    U0putchar(hourChar[1]);
    U0putchar(':');
    U0putchar(minChar[0]);
    U0putchar(minChar[1]);
    U0putchar(':');
    U0putchar(secChar[0]);
    U0putchar(secChar[1]);
    U0putchar('~');
    for (int i = 0; i < 18; i++){
     U0putchar(idle[i]);
    }
    U0putchar('\n');
   }
  
 }
 
 //if off button pressed
 if (!(*pin_e & 0x8)){
  state = 0;

  //prints off state entered + RTC time
  if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    U0putchar(hourChar[0]);
    U0putchar(hourChar[1]);
    U0putchar(':');
    U0putchar(minChar[0]);
    U0putchar(minChar[1]);
    U0putchar(':');
    U0putchar(secChar[0]);
    U0putchar(secChar[1]);
    U0putchar('~');
    for (int i = 0; i < 17; i++){
     U0putchar(off_state[i]);
    }
    U0putchar('\n');
   }
 }

// idle state conditions
 if (state == 1){
  unsigned int light_val = adc_read(0);
  
  *port_h &= ~(0x01 << 4);
  *port_h &= ~(0x01 << 5);
  *port_h |= 0x01 << 6;
  *port_b &= ~(0x01 << 4);

  *port_b &= ~(0x01 << 6);
  motor.write(0);

  // sends to active state if light value is under threshold
  if (light_val < 500){
    state = 2;

    //prints active state entered + actuator on + RTC time
    if (currentMillis - previousMillis >= 500) {
    previousMillis = currentMillis;
    U0putchar(hourChar[0]);
    U0putchar(hourChar[1]);
    U0putchar(':');
    U0putchar(minChar[0]);
    U0putchar(minChar[1]);
    U0putchar(':');
    U0putchar(secChar[0]);
    U0putchar(secChar[1]);
    U0putchar('~');
    for (int i = 0; i < 20; i++){
     U0putchar(active[i]);
    }
    U0putchar('\n');
    U0putchar(hourChar[0]);
    U0putchar(hourChar[1]);
    U0putchar(':');
    U0putchar(minChar[0]);
    U0putchar(minChar[1]);
    U0putchar(':');
    U0putchar(secChar[0]);
    U0putchar(secChar[1]);
    U0putchar('~');
    for (int i = 0; i<17; i++){
      U0putchar(motor_on[i]);
      }
    U0putchar('\n');
   }
  }

 }

// active state conditions
  if (state == 2){
    unsigned int light_val = adc_read(0);
    
    
    *port_h &= ~(0x01 << 4);
    *port_h |= 0x01 << 5;
    *port_h &= ~(0x01 << 6);
    *port_b &= ~(0x01 << 4);

    motor.write(90);
    *port_b |= 0x01 << 6;

    //sends back to idle state if light value is no longer under threshold
    if (light_val > 500){
    state = 1;
    // prints idle state re entered + actuator off + RTC time
    if (currentMillis - previousMillis >= 500) {
       previousMillis = currentMillis;
       U0putchar(hourChar[0]);
       U0putchar(hourChar[1]);
       U0putchar(':');
       U0putchar(minChar[0]);
       U0putchar(minChar[1]);
       U0putchar(':');
       U0putchar(secChar[0]);
       U0putchar(secChar[1]);
       U0putchar('~');
       for (int i = 0; i < 18; i++){
        U0putchar(idle[i]);
       }
       U0putchar('\n');
       U0putchar(hourChar[0]);
       U0putchar(hourChar[1]);
       U0putchar(':');
       U0putchar(minChar[0]);
       U0putchar(minChar[1]);
       U0putchar(':');
       U0putchar(secChar[0]);
       U0putchar(secChar[1]);
       U0putchar('~');
       for (int i = 0; i<16; i++){
        U0putchar(motor_off[i]);
        }
       U0putchar('\n');
      }
    }
    // sends to error state if light value is zero
    if (light_val == 0){
    state = 3;
    // print error state entered + RTC time
    if (currentMillis - previousMillis >= 500) {
     previousMillis = currentMillis;
     U0putchar(hourChar[0]);
     U0putchar(hourChar[1]);
     U0putchar(':');
     U0putchar(minChar[0]);
     U0putchar(minChar[1]);
     U0putchar(':');
     U0putchar(secChar[0]);
     U0putchar(secChar[1]);
     U0putchar('~');
     for (int i = 0; i < 19; i++){
       U0putchar(error[i]);
     }
     U0putchar('\n');
    }
   }
  }

// error state conditions
  if (state == 3){
    *port_h |= 0x01 << 4;
    *port_h &= ~(0x01 << 5);
    *port_h &= ~(0x01 << 6);
    *port_b &= ~(0x01 << 4);

    motor.write(0);
    *port_b &= ~(0x01 << 6);
  }
   
}

// functions to replace serial monitor:
void U0init(unsigned long U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  if (*myUCSR0A & RDA)
    return 1;
  else
  return 0;
}
unsigned char U0getchar()
{
  unsigned char ch;
  while(!(*myUCSR0A & 0x80));
  ch = *myUDR0;
  return ch;
}
void U0putchar(unsigned char U0pdata)
{
  while(!(*myUCSR0A & 0b00100000));
  *myUDR0 = U0pdata;
}

//functions to replace analog read
void adc_init() 
{

  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11110111;
  *my_ADCSRA &= 0b11111000;

  *my_ADCSRB &= 0b11110111;
  *my_ADCSRB &= 0b11111000;
 
  *my_ADMUX &= 0b11011111;
  *my_ADMUX &= 0b11100000;
  *my_ADMUX &= 0b01111111;
  *my_ADMUX |= 0b01000000;
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  
  *my_ADMUX &= ~(0x1F);
  *my_ADCSRB &= ~(0x08);
  
  *my_ADMUX |= adc_channel_num;

  
  *my_ADCSRA |= 0x40;
  
  while((*my_ADCSRA & 0x40) != 0);
  
  unsigned int val = (*my_ADC_DATA & 0x03FF);
  return val;
}

// function for timer
void setup_timer_regs()
{
  
  *myTCCR1A = 0x00;
  *myTCCR1B = 0x00;
  *myTCCR1C = 0x00;
  
  *myTIFR1 |= 0x01;
  
  *myTIMSK1 |= 0x01;
}

// TIMER OVERFLOW ISR
ISR(TIMER1_OVF_vect)
{
  
  *myTCCR1B &= 0xF8;
  
  *myTCNT1 = (unsigned int)(65535 - (unsigned long)(currentTicks));
  
  *myTCCR1B |= 0x01;
  
  if(currentTicks != 65535)
  {
    
    *port_b ^= 0x40;
  }
}

// Interrupt function
void sendStateOne (){
  state = 1;
}
