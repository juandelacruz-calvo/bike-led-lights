#include <Arduino.h>
#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#include <SoftwareSerial.h>
#endif
#include "OneButton.h"

#include <avr/sleep.h>

const int switchPin = PB1;
const int statusLED = PB0;

#ifdef DEBUG_SERIAL
SoftwareSerial mySerial(3, 4); // rx, tx
#endif

OneButton button(switchPin, true);
// save the millis when a press has started.
unsigned long pressStartTime;

void goToSleep()
{
    GIMSK |= _BV(PCIE);                  // Enable Pin Change Interrupts
    PCMSK |= _BV(PCINT1);                // Use PB3 as interrupt pin
    ADCSRA &= ~_BV(ADEN);                // ADC off
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // replaces above statement

    sleep_enable(); // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
    sei();          // Enable interrupts
    // mySerial.println("sleep now!");
    sleep_cpu(); // sleep

    cli();                 // Disable interrupts
    PCMSK &= ~_BV(PCINT1); // Turn off PB3 as interrupt pin
    sleep_disable();       // Clear SE bit
    ADCSRA |= _BV(ADEN);   // ADC on

    sei(); // Enable interrupts
#ifdef DEBUG_SERIAL
    mySerial.println("wake up!");
#endif
} // sleep

void pressStart()
{
#ifdef DEBUG_SERIAL
    mySerial.println("presStart");
#endif
    for (int k = 0; k < 6; k = k + 1)
    {
        if (k % 2 == 0)
        {
            digitalWrite(statusLED, HIGH);
        }
        else
        {
            digitalWrite(statusLED, LOW);
        }
        delay(50);
    } // for
}

void pressStop()
{
    digitalWrite(statusLED, LOW);
    delay(250);
    mySerial.println("pressSTop");

    mySerial.println("go to sleep");

    goToSleep();
}

int pattern = 1;

void cyclePattern()
{
    if (pattern >= 4)
    {
        pattern = 1;
    }
    else
    {
        pattern++;
    }
}

void blink(int times)
{
    for (int k = 0; k < times; k = k + 1)
    {
        if (k % 2 == 0)
        {
            digitalWrite(statusLED, HIGH);
        }
        else
        {
            digitalWrite(statusLED, LOW);
        }
        delay(50);
    } // for
}

long readVcc()
{
    ADMUX = _BV(MUX3) | _BV(MUX2); // selecting bandgap reference of 1.1V (on page 135)
    delay(2);                      // Wait for bandgap reference to stabilize
    uint8_t oldSREG = SREG;        // saving status register
    cli();                         // deactivate Interrupts
    ADCSRA |= _BV(ADSC);           // Start ADC conversion, this is where the 1.1V reference is read against vcc
    while (bit_is_set(ADCSRA, ADSC))
        ;                       // wait until complete
    uint16_t result = ADC;      // Needs to be 16 bit long ! This only reads the two registers in one command
    SREG = oldSREG;             // setting status register to "old" values
    sei();                      // Activate Interrupts
    result = 1126400L / result; // Calculate Vcc (in mV); 1126400L = 1.110241000 // 1024 should be right
    return result;              // Vcc in millivolts
}

void setup()
{
#ifdef DEBUG_SERIAL
    mySerial.begin(9600);
    mySerial.println("Starting");
#endif
    pinMode(statusLED, OUTPUT);

    button.setPressTicks(1000); // that is the time when LongPressStart is called
    button.attachClick(cyclePattern);
    button.attachLongPressStart(pressStart);
    button.attachLongPressStop(pressStop);

    // Flash quick sequence so we know setup has started
    blink(2);
#ifdef DEBUG_SERIAL
    mySerial.println("End setup");
#endif
    if (readVcc() < 3300)
    {
        blink(4);
        goToSleep();
    }
} // setup

void waitAndTick(int ms)
{
    for (int i = 0; i < ms; i = i + 10)
    {
        delay(10);
        button.tick();
    }
}

void pattern1()
{
    digitalWrite(statusLED, LOW);
    waitAndTick(500);
    digitalWrite(statusLED, HIGH);
    waitAndTick(500);
}

void pattern2()
{
    digitalWrite(statusLED, LOW);
    waitAndTick(300);
    digitalWrite(statusLED, HIGH);
    waitAndTick(100);
}

void pattern3()
{
    digitalWrite(statusLED, LOW);
    waitAndTick(100);
    digitalWrite(statusLED, HIGH);
    waitAndTick(100);
}

void pattern4()
{
    digitalWrite(statusLED, HIGH);
    waitAndTick(100);
}

void loop()
{
#ifdef DEBUG_SERIAL
    mySerial.print("voltage: ");
    mySerial.println(readVcc());
#endif
    button.tick();

    switch (pattern)
    {
    case 1:
        pattern1();
        break;
    case 2:
        pattern2();
        break;
    case 3:
        pattern3();
        break;
        break;
    case 4:
        pattern4();
        break;
    default:
        pattern1();
    }
}
