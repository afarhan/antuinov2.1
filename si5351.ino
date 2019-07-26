#include <Wire.h>

#define SI_CLK0_CONTROL  16      // Register definitions
#define SI_CLK1_CONTROL 17
#define SI_CLK2_CONTROL 18
#define SI_SYNTH_PLL_A  26
#define SI_SYNTH_PLL_B  34
#define SI_SYNTH_MS_0   42
#define SI_SYNTH_MS_1   50
#define SI_SYNTH_MS_2   58
#define SI_PLL_RESET    177

#define SI_R_DIV_1    0b00000000      // R-division ratio definitions
#define SI_R_DIV_2    0b00010000
#define SI_R_DIV_4    0b00100000
#define SI_R_DIV_8    0b00110000
#define SI_R_DIV_16   0b01000000
#define SI_R_DIV_32   0b01010000
#define SI_R_DIV_64   0b01100000
#define SI_R_DIV_128    0b01110000

#define SI_CLK_SRC_PLL_A  0b00000000
#define SI_CLK_SRC_PLL_B  0b00100000

void i2cSendRegister (byte regist, byte value){   // Writes "byte" into "regist" of Si5351a via I2C
  Wire.beginTransmission(96);                       // Starts transmission as master to slave 96, which is the
                                                    // I2C address of the Si5351a (see Si5351a datasheet)
  Wire.write(regist);                               // Writes a byte containing the number of the register
  Wire.write(value);                                // Writes a byte containing the value to be written in the register
  Wire.endTransmission();                           // Sends the data and ends the transmission
}

void si5351aOutputOff(uint8_t clk)
{
  //i2c_init();
  
  i2cSendRegister(clk, 0x80);   // Refer to SiLabs AN619 to see bit values - 0x80 turns off the output stage

  //i2c_exit();
}


void setupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom)
{
  uint32_t P1;          // PLL config register P1
  uint32_t P2;          // PLL config register P2
  uint32_t P3;          // PLL config register P3

  P1 = (uint32_t)(128 * ((float)num / (float)denom));
  P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
  P2 = (uint32_t)(128 * ((float)num / (float)denom));
  P2 = (uint32_t)(128 * num - denom * P2);
  P3 = denom;

  i2cSendRegister(pll + 0, (P3 & 0x0000FF00) >> 8);
  i2cSendRegister(pll + 1, (P3 & 0x000000FF));
  i2cSendRegister(pll + 2, (P1 & 0x00030000) >> 16);
  i2cSendRegister(pll + 3, (P1 & 0x0000FF00) >> 8);
  i2cSendRegister(pll + 4, (P1 & 0x000000FF));
  i2cSendRegister(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  i2cSendRegister(pll + 6, (P2 & 0x0000FF00) >> 8);
  i2cSendRegister(pll + 7, (P2 & 0x000000FF));
}

//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it is a #define in si5351a.h
//
void setupMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv)
{
  uint32_t P1;          // Synth config register P1
  uint32_t P2;          // Synth config register P2
  uint32_t P3;          // Synth config register P3

  P1 = 128 * divider - 512;
  P2 = 0;             // P2 = 0, P3 = 1 forces an integer value for the divider
  P3 = 1;

  i2cSendRegister(synth + 0,   (P3 & 0x0000FF00) >> 8);
  i2cSendRegister(synth + 1,   (P3 & 0x000000FF));
  i2cSendRegister(synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
  i2cSendRegister(synth + 3,   (P1 & 0x0000FF00) >> 8);
  i2cSendRegister(synth + 4,   (P1 & 0x000000FF));
  i2cSendRegister(synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  i2cSendRegister(synth + 6,   (P2 & 0x0000FF00) >> 8);
  i2cSendRegister(synth + 7,   (P2 & 0x000000FF));
}

// This example sets up PLL A
// and MultiSynth 0
// and produces the output on CLK0
//
void si5351aSetFrequency(uint32_t frequency)
{
  uint32_t pllFreq;
  uint32_t xtalFreq = xtal_freq_calibrated;
  uint32_t l;
  float f;
  uint8_t mult;
  uint32_t num;
  uint32_t denom;
  uint32_t divider;

  divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
                  // PLL frequency: 900MHz
  if (divider % 2) divider--;   // Ensure an even integer division ratio

  pllFreq = divider * frequency;  // Calculate the pllFrequency: the divider * desired output frequency

  mult = pllFreq / xtalFreq;    // Determine the multiplier to get to the required pllFrequency
  l = pllFreq % xtalFreq;     // It has three parts:
  f = l;              // mult is an integer that must be in the range 15..90
  f *= 1048575;         // num and denom are the fractional parts, the numerator and denominator
  f /= xtalFreq;          // each is 20 bits (range 0..1048575)
  num = f;            // the actual multiplier is  mult + num / denom
  denom = 1048575;        // For simplicity we set the denominator to the maximum 1048575

                  // Set up PLL A with the calculated multiplication ratio
  setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
                  // Set up MultiSynth divider 0, with the calculated divider. 
                  // The final R division stage can divide by a power of two, from 1..128. 
                  // reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
                  // If you want to output frequencies below 1MHz, you have to use the 
                  // final R division stage
  setupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
                  // Reset the PLL. This causes a glitch in the output. For small changes to 
                  // the parameters, you don't need to reset the PLL, and there is no glitch
  i2cSendRegister(SI_PLL_RESET, 0xA0);  
                  // Finally switch on the CLK0 output (0x4F)
                  // and set the MultiSynth0 input to be PLL A
  i2cSendRegister(SI_CLK1_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);
}

void si5351aSetFrequency_clk0(uint32_t frequency)
{
  uint32_t pllFreq;
  uint32_t xtalFreq = xtal_freq_calibrated;
  uint32_t l;
  float f;
  uint8_t mult;
  uint32_t num;
  uint32_t denom;
  uint32_t divider;

  divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
                  // PLL frequency: 900MHz
  if (divider % 2) divider--;   // Ensure an even integer division ratio

  pllFreq = divider * frequency;  // Calculate the pllFrequency: the divider * desired output frequency
  //sprintf(buff,"pllFreq: %ld", (long)pllFreq);
  //Serial.println(buff);                

  mult = (pllFreq / xtalFreq);    // Determine the multiplier to get to the required pllFrequency
  l = pllFreq % xtalFreq;     // It has three parts:
  f = l;              // mult is an integer that must be in the range 15..90
  f *= 1048575;         // num and denom are the fractional parts, the numerator and denominator
  f /= xtalFreq;          // each is 20 bits (range 0..1048575)
  num = f;            // the actual multiplier is  mult + num / denom
  denom = 1048575;        // For simplicity we set the denominator to the maximum 1048575

                  // Set up PLL B with the calculated multiplication ratio
  setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
                  // Set up MultiSynth divider 0, with the calculated divider. 
                  // The final R division stage can divide by a power of two, from 1..128. 
                  // reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
                  // If you want to output frequencies below 1MHz, you have to use the 
                  // final R division stage
  setupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
                  // Reset the PLL. This causes a glitch in the output. For small changes to 
                  // the parameters, you don't need to reset the PLL, and there is no glitch
  i2cSendRegister(SI_PLL_RESET, 0xA0);  
                  // Finally switch on the CLK2 output (0x4F)
                  // and set the MultiSynth0 input to be PLL A
  i2cSendRegister(SI_CLK0_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);
}

void si5351aSetFrequency_clk1(uint32_t frequency)
{
  uint32_t pllFreq;
  uint32_t xtalFreq = xtal_freq_calibrated;
  uint32_t l;
  float f;
  uint8_t mult;
  uint32_t num;
  uint32_t denom;
  uint32_t divider;

  divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
                  // PLL frequency: 900MHz
  if (divider % 2) divider--;   // Ensure an even integer division ratio

  pllFreq = divider * frequency;  // Calculate the pllFrequency: the divider * desired output frequency

  mult = (pllFreq / xtalFreq);    // Determine the multiplier to get to the required pllFrequency
  l = pllFreq % xtalFreq;     // It has three parts:
  f = l;              // mult is an integer that must be in the range 15..90
  f *= 1048575;         // num and denom are the fractional parts, the numerator and denominator
  f /= xtalFreq;          // each is 20 bits (range 0..1048575)
  num = f;            // the actual multiplier is  mult + num / denom
  denom = 1048575;        // For simplicity we set the denominator to the maximum 1048575

                  // Set up PLL A with the calculated multiplication ratio
  setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
                  // Set up MultiSynth divider 0, with the calculated divider. 
                  // The final R division stage can divide by a power of two, from 1..128. 
                  // reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
                  // If you want to output frequencies below 1MHz, you have to use the 
                  // final R division stage
  setupMultisynth(SI_SYNTH_MS_1, divider, SI_R_DIV_1);
                  // Reset the PLL. This causes a glitch in the output. For small changes to 
                  // the parameters, you don't need to reset the PLL, and there is no glitch
  i2cSendRegister(SI_PLL_RESET, 0xA0);  
                  // Finally switch on the CLK0 output (0x4F)
                  // and set the MultiSynth0 input to be PLL A
  i2cSendRegister(SI_CLK1_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);
}


void si5351aSetFrequency_clk2(uint32_t frequency)
{
  uint32_t pllFreq;
  uint32_t xtalFreq = xtal_freq_calibrated;
  uint32_t l;
  float f;
  uint8_t mult;
  uint32_t num;
  uint32_t denom;
  uint32_t divider;

  divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
                  // PLL frequency: 900MHz
  if (divider % 2) divider--;   // Ensure an even integer division ratio

  pllFreq = divider * frequency;  // Calculate the pllFrequency: the divider * desired output frequency
  //sprintf(buff,"pllFreq: %ld", (long)pllFreq);
  //Serial.println(buff);                

  mult = (pllFreq / xtalFreq);    // Determine the multiplier to get to the required pllFrequency
  l = pllFreq % xtalFreq;     // It has three parts:
  f = l;              // mult is an integer that must be in the range 15..90
  f *= 1048575;         // num and denom are the fractional parts, the numerator and denominator
  f /= xtalFreq;          // each is 20 bits (range 0..1048575)
  num = f;            // the actual multiplier is  mult + num / denom
  denom = 1048575;        // For simplicity we set the denominator to the maximum 1048575

                  // Set up PLL B with the calculated multiplication ratio
  setupPLL(SI_SYNTH_PLL_B, mult, num, denom);
                  // Set up MultiSynth divider 0, with the calculated divider. 
                  // The final R division stage can divide by a power of two, from 1..128. 
                  // reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
                  // If you want to output frequencies below 1MHz, you have to use the 
                  // final R division stage
  setupMultisynth(SI_SYNTH_MS_2, divider, SI_R_DIV_1);
                  // Reset the PLL. This causes a glitch in the output. For small changes to 
                  // the parameters, you don't need to reset the PLL, and there is no glitch
  i2cSendRegister(SI_PLL_RESET, 0xA0);  
                  // Finally switch on the CLK2 output (0x4F)
                  // and set the MultiSynth0 input to be PLL A
  i2cSendRegister(SI_CLK2_CONTROL, 0x4F | SI_CLK_SRC_PLL_B);
}

