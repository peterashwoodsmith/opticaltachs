#include <Arduino.h>
#include <TM1637Display.h>

//
// Trivial Little Arduino Nano program to act as a optical tachometer and output the frequency in HZ of
// the optical sensor.  We use an IR diode and an IR transistor of the same wavelength. A TM1637 4 digit
// Display is used to display the results.
//

//
// This is the constructor for the display object 'display' we use NANO pins 3 and 4 to drive it in addition
// of course to power and ground.
//
TM1637Display display(3,4);

//
// We can do interrupt driven counting, or we can do analog port reads looking for spikes and counting them.
//
int interrupt_mode_g = 0;

// 
// We use an interrupt routine to track the number of flashes of reflected light the IR transistor detects.
// The interrupt routing of course just increments the interrupt_count variable by one.
//
unsigned long interrupt_count;

void interrupt()
{    interrupt_count += 1;
}

// 
// To get started all we need to do is attach out interrupt handler to the rising edge on PIN 2
// adjust the display brighness and make sure interrupts are disabled before we start sampling.
//
void setup() 
{    if (interrupt_mode_g) {
         attachInterrupt(digitalPinToInterrupt(2), interrupt, RISING);
         noInterrupts();
     }
     display.setBrightness(0xf);
}
 
//
// Sample the sensors for ms ms. We return the actual time we used to sample.
// During the sample interval we either count interrupts, or using the analog 
// port we attempt to count transitions.
//
long int sample(unsigned long ms)
{    unsigned long t_end, t_now, t_start, t_dur, us_per_sample;
     if (interrupt_mode_g) { 
         t_start = micros();  
         interrupt_count = 0;  
         interrupts();
         delay(ms); 
         noInterrupts();
         t_end = micros();
     } else {
         const long MAX_SAMPLE = 500;
         static unsigned short sample[MAX_SAMPLE];
         for (int i=0; i < MAX_SAMPLE; sample[i++] = 0);
         t_dur = ms * 1000;
         t_start = micros();
         t_end = t_start + t_dur; 
         t_now = t_start;
         us_per_sample = t_dur / MAX_SAMPLE;
         while(1) {
             unsigned int sx = ((t_now - t_start)/us_per_sample) % MAX_SAMPLE;
             sample[sx] = analogRead(A0);
             t_now = micros();
             if ((t_start < t_end)  &&(t_now >= t_end))   break;
             if ((t_end   < t_start)&&(t_now >= t_start)) break;
         }
         unsigned short max_sample = 0, min_sample = 1024;
         for(int i = 0; i < MAX_SAMPLE; i++) {
             unsigned short si = sample[i];
             if (si > max_sample) max_sample = si;
             if (si < min_sample) min_sample = si;
         }
         interrupt_count = 0;
         unsigned short delta = max_sample-min_sample;
         if ((delta > 50)&&(delta < 1000)) {
             unsigned short thresh = min_sample + delta/2;
             unsigned short state = 0;
             for(int i = 0; i < MAX_SAMPLE; i++) {
                 unsigned short si = sample[i];
                 if ((si > thresh) && (state == 0)) { 
                      state = 1; 
                      interrupt_count += 1; 
                 } 
                 if (si < thresh) 
                      state = 0;
             }
         }
     }
     if (t_start > t_end)
         return(t_start - t_end);
     return(t_end - t_start);
}

//
// The loop just performs 1/2 second of sampling after which is prints the frequency of the flashes that were
// seen. This of course is just 2 x the number of interrupts that were counted. The sampling can happen during
// a delay via interrupts, or the sampling can be done via looping and reading the analog port.
//
void loop() 
{    unsigned long t_dur = sample(500);
     unsigned long t_fact = 1000000 /(t_dur+1);
     display.showNumberDec(interrupt_count * t_fact);
}
