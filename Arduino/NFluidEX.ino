/*
  February 14, 2017 (Started as NC1194_Potentiostat.ino)

  Updated March 23, 2017- largely to address hardware design changes on new design iteration NC1194 Potentiostat V3. These include use of i2c octal switch ADG715 for 
  potentiostat signal switching (A0 and A1 on ADG715 pulled to ground, so i2c address is 0x48). Network analyzer AD5933 address is 0x0D. Other changes are use of 
  low dropout 3.3V regulator AP7365 for wireless devices (high side switching transistor of TPS 62172was not able to deliver required current from single cell Lithium battery),
  corrected i2c pinout for ESP8266 (GPIO5 for SCL, GPIO4 for SDA), and reconfigured potentiostat network (error made in earlier designs).

  Updated dates up to about April 24, 2017- include code to read and communicate temperature, and impedance register values from AD5933 for given frequency, as well as
  current and voltage applied through transimpedance amplifier...

  Updated (to version 04_Exp) on April 30, 2017, for testing various functionality necessary for development of more fully developed Android GUI.

  Changed name to ABE-Stat_1_0_03 (previous versions in summer 2017) on August 20, 2017, mostly to run different basic measurement features of NC1194 Potentiostat. Version 1_0_03 is the
  first to include specific analytical methods (i.e. starting with cyclic voltammetry and electrochemical impedance spectroscopy); ABE-Stat_1_0_04 first version to implement EIS; also polishing
  up CV...

  ABE-Stat 1_0_5 started September 8, 2017- building on fully functional ABE-Stat1_0_04 which has calibration routines including for battery, EIS functions using AD5933 (from 5 kHz to 100 kHz),
  CV functions up to 0.2 scan rate. Adding EIS functionality from 0.1Hz to about 100 Hz using signals generated entirely from DAC5061, and DFT of TIA samples from ADS1220 (in this case running 
  in turbo mode at highest data rate 2000 sps...

  ABE-Stat 1_0_06 started September 14, 2017; admittance vs frequency calibrations change with different AD5933 settings (i.e. PGAx5GainSetting, excitationCode, and TIAGainCode;
  so update calibration routines to calibrate admittance magnitude (scaled by load resistor from calibration
  Version 06_worked on through September 26 2017 to include open circuit configuration and open circuit voltage measurement (i.e. pH electrode); upgraded this version through
  September 26 to include calibration for every setting of AD5933, and all basic interfaces with Android ABE-Stat app through Cyclic Voltammetry and Electrochemical
  Impedance spectroscopy, including for low frequencies using ADC (ADS1220) for customized single frequency DFT...

  ABE-Stat_1_0_07 started September 27, 2017; simple check for TIA saturation not always effective (at saturation amplifier inputs violate ideality(?) and the observed TIA output voltage 
  is not always close to positive analog rail value (i.e. in some instances is lower than 1 V)- so updated resistance measurement, current measurements, and EIS measurements
  to confirm linearity of a result at a different setting before accepting a value... Also, added methods to conduct Differential Pulse Voltammetry.
  Also included a firmware number to report back to Android... Also tried to check all calculated values for divide by 0 to prevent "nan" or "inf" being written to Android when it expects
  a parsable number string...

  ABE-Stat_1_0_08 October 26; add code to shut off power after 1 minute idle with no bluetooth connection.
  ABE-Stat_1_0_09 November 3, 2017; blink LED a few times and turn off if battery is discharged; corrected EEPROM_Int_Write so that 16 bit value gets written to two 8 bit blocks (previously LSB overwrites MSB)

  ABE-Stat_1_0_10 December 15, 2017, turn off WiFi at startup, and keep off to suppress noise; also increase update rates for CV to try to smooth applied potential

  ABE-Stat_1_01_00 started January 19, 2018, update functions to use external clock 250kHz clock with AD5933 for lower frequency EIS; update requires updated board design ABE-Stat_1.0.01
  (previous versions worked with NC-1194_Potentiostat). Need to add additional EEPROM and calibration assignments to calibrate device operating over new frequencies and clock,
  and ABE-Stat_DigitalConfigurations (i.e. !O12 controls power enable to AD5933 and external clock- leave off for other voltammetric applications to limit noise).
  Also update DigitalConfigurations to use ADG715 S02 to make biasing amplifier a voltage follower for open circuit configuration (don't allow unpredictable leakage from drifting
  output). (used for preliminary testing of EIS with MCLK external clock, and also some troubleshooting to improve LED modulation for indicating charge state)

  ABE-Stat_1_01_02 started February 21, 2018; start developing calibration of EIS with external MCLK, and defining best frequency ranges for each setting (?).

  ABE-Stat_1_01_03 started May 9, 2018; to work with hardware version ABE-Stat 1_0_02, (separate power controller on O13 for AD5933 external clock); device also attempts to correct
  750 Hz harmonic distortion observed in version 01

  ABE-Stat_1_01_07, starting July 6, 2018 building on last stable version ABE-Stat_1_01_03 (versions 1_01_04 to 1_01_05 are transitional codes that either won't compile, or resulted in 
  issues of such as analog switches not behaving predictably / reproducibly(?)), so here we salvage some of the improvements in approach for reliable EIS from version 1_01_06
  Generally, when using AD5933 for the first measurement an optimization routine is called to test every gain and PGA setting at the given excitation signal (bias and amplitude)
  to identify the one that results in the best quality data (saturation will result in an overprediction of impedance, so setting is chosen with the highest digitized admittance value

  ABE-Stat_1_01_08, starting July 6, 2018 building on last stable version ABE-Stat_1_01_03 (versions 1_01_04 to 1_01_05 are transitional codes that either won't compile, or resulted in 
  issues of such as analog switches not behaving predictably / reproducibly(?)), version 1.01.07 successfully updated testImpedance function to with a separate startAD5933Synthesizer
  and readAD5933Data functions, and averages 10 successive readings... Here we try to also incorporate the function to automitically identify the optimal test settings...

  ABE-Stat_1_01_09, starting July 12, 2018 quickly hard code observed bias on AD5933 working vs system analog reference (so we can correct for to apply correct bias for EIS)
   Also hard code correction for frequency, and send back EIS results at every available setting to reconstruct spectra...

  ABE-Stat_1_01_10, starting August 10, 2018 add new functionality for generic logging function (i.e. log potential in current for configurable electrode network / applied potential),
    i.e. for chronoamperometry

  ABE-Stat_1_01_11, starting August 25, 2018 force all analytical / multimeter functions to send code ('j') inside iterative loops to prevent Android device from timing out and
    disconnecting, and several other debugs to prevent system timeout which especially might cause ABE-Stat to lock up without ability to reset / self-shutoff

  ABE-Stat_1_01_12, starting August 26, 2018 upgrades timing functions for CV (replace "delay(1)" with yield(); delay(1) was especially problematic given that DAC increment period
    is 1 ms...; also used micros() for timing instead of millis() (a bad choice for 1 ms interval time for updating DAC value); also corrected issue with electrode configuration commands
    not being reliable- evidently i2c commands are not read by ADG715 octal analog switch if AD5933 (on shared i2c bus) is not powered- so always ensure that AD5933 is powered
    during instructions to change electrode configuration.

  ABE-Stat_1_01_13, starting August 27, 2018 Polish up a few last items, i.e. allow uniform user defined (log scale) intervals for EIS, and calibrate frequency AD5933 internal clock
     and correspondingly ensure that frequency values reported for EIS are accurate (each device will operate with slightly different frequencies).

  ABE-Stat_1_01_14, starting September 19, 2018 Recover battery charge reporting functionality not working correctly in recent versions; return electrode configuration back to open
    open circuit potential after completion of every analytical method (prevents undesired application of corrosive potentials on networks / electrodes when not in use)...
    (Note, actually poor battery monitoring performance is related to low (out of spec) range of ESP8266 ADC on many devices (full scale should go to 1.0 V, but evidently the
    scale / reference is significantly smaller on many ESP8266 chips...

  ABE-Stat_1_01_15, starting October 1, 2018; chooses the correct feedback resistance to prevent TIA saturation while "equilibrating" at desired potential on CV and DPV
    (saturation can diminish "equilibrating" potential and redox currents- biasing measurements as system corrects saturation problem (this is an issue for systems where
    currents would exceed 10 uA under the desired potential, using highest gain setting. Symptom of problem is big tail on starting redox current...

  ABE-Stat_1_01_16, starting October 28, 2018; checks current and increases gain if observed current is sub-optimal (i.e. predicted TIA voltage from increased gain does not 
     saturate amplifier) (ultimately algorithm uses previous impedance estimate to predict settings for new measurement to prevent saturation- approach works best starting at
     low frequencies with high impedance, and using small increments in EIS frequency measurement so large changes in impedance don't occur between successive measurements...

  ABE-Stat_1_01_17, starting November 19, 2018; change thresholds for predicting saturation (i.e. some fixed resistor EIS increases in "observed" impedance when measuring with network
    analyzer (i.e. above 60 Hz impedance reported for ~3300 Ohm resistor is ~5100 Ohm). Also try to fix problem where second EIS scan gets hung up in open circuit configuration.
    Predicted TIA voltage threshold changed from 1.2 V to 1.0 V...

  ABE-Stat_1_01_18, March 7, 2019- remove all function calls to AD5933_PowerOn(false), and replace with 
    new function AD5933_PowerDown() that maintains voltage to supply pin of AD5933, but puts
    device into power down mode to save power and shut of unused systems that might cause noise in 
    analytical methods implemented with other hardware. Shutting off power to AD5933 altogether
    drags the data pin on i2c bus so that many instructions to change ADG715 settings (for
    electrode configuration and excitation signal composition) are missed resulting in incorrect
    potentiostat behavior / measurements.
  
  Shift register outputs (2x serialized 74HC595, O00 - O15):
      O00 74LVG1G53 switch1 signal (!CS to !CS1 (ADS1220/ "0") or !CS2 (AD5061/ "1")
      O01 74LVG1G53 switch1 !enable (!CS1 && !CS2 pulled high if O01 high)
      O02 74LVG1G53 switch2 signal (RN42 BT_Connected "0" or ADS1220 !DRDY "1" to GPIO00)
      O03 74LVG1G53 switch2 !enable (GPIO00 floats when O03 high)
      O04 ADG779 switch1 signal (TIA feedback to LMP7721 Vout "0" or AD59933 RFB "1")
      O05 ADG779 switch2 signal (TIA input to LMP7721 V- "0" or AD5933 Vin "1")
      O06 Input to illuminate red segment of 3-color LED
      O07 PMOS gate driver for multicolor LED (active low)
      O08 / O09 A0 and A1 of ADG704 switch (set TIA gain, 00 = 100M; 01 = 10k; 10 = 1M; 11 = 100
      O10 TIA gain setting enable (otherwise feedback is open circuit)
      O11 Software shutoff ("0" removes power from circuit, but only when no USB supply is provided)
      O12 !Enable for AD5933 DVDD (leave off / high for voltammetric applications to remove potential source of noise)
      O13 !Enable for AD5933 250kHz external clock (leave off / high for voltammetric applications to remove source of noise).

      O14 &O15 general purpose outputs broken out to board periphery (careful using O12 and O13 as on ABE-Stat hardware implementation v02; can 
        conflict with enable / power circuit for AD5933 and its external clock

  ADG715 Switch Connections (S0 - S7; states set by state of corresponding bit written to i2c address 0x48):
      S0 Connect working electrode to TIA
      S1 Not used (contacts held at ground)
      S2 voltage follower on biasing amplifier (use to stabilize output in open circuit configuration)
      S3 Feedback reference electrode to amplifier setting counter electrode potential
      S4 Connect Counter electrode amplifier output to reference electrode (connect counter and reference electrodes)
      S5 Connect counter electrode to it's amplifier output
      S6 Sum AD5933 output/ signal source into potentiostat network
      S7 Sum AD5061 DAC voltage into potentiostat network
   
by Daniel M. Jenkins
*/

#include <SPI.h>
#include <Wire.h> // reserved for when we start communicating with AD5933
#include <EEPROM.h>

#include "ESP8266WiFi.h"  // necessary for disabling WiFi- try to suppress extra noise

// IO for SPI communication  (CS shared with shift register clock )
#define MISO             12
#define MOSI             13
#define SCK              14
#define CS_RCK           15 // shared with shift register clock

//inputs to shift register (configure as outputs from ESP8266)
#define SER              16 // data into shift registers
#define CLOCK            2  // clock data in from SER to data registers (note that this is separate from the register clock)

#define RELAY            10 // controls relay module

// i2c and SPI default pins based on definitions using the pinout shown at https://github.com/esp8266/Arduino/blob/master/doc/reference.md 

// GPIO00 assigned as input (listen for either !DRDY from ADS1220, or BluetoothConnected Signal)
#define GPIO00            0

// map to connect ESP8266 !CS to corresponding !CS pins on ADS1220 or AD5061 (only allow one to connect at a time, other is always pulled high- make sure that both
// devices are not writing to MISO simultaneously (even though AD5061 doesn't have a data out pin)
#define ADS1220_ADC          0  // SPI chip select designation for ADS1220 (value on shift register output 00 to connect !CS to ADS1220)
#define AD5061_DAC           1  // SPI chip select designation for AD5061 (value on shift register output 00 to connect !CS to AD5061)

#define TWO_ELECTRODE_CONFIG  0x02
#define THREE_ELECTRODE_CONFIG  0x03
#define OPEN_CIRCUIT_CONFIG 0x00

#define BAUD             115200 // baud rate for serial communication

// i2c addresses
#define ADG715OctalSwitch      0x48
#define AD5933NetworkAnalyzer  0x0D

// Control / data register assignments for network analyzer
#define AD5933_Control            0x80  // 2 byte control register
#define AD5933_StartFrequency     0x82  // 3 byte control register
#define AD5933_FrequencyIncr      0x85  // 3 byte control register
#define AD5933_IncrNumber         0x88  // 2 byte control register
#define AD5933_SettleCycleNumber  0x8A // 2 byte control register
#define AD5933_Status             0x8F   // 1 byte status register
#define AD5933_TemperatureData    0x92  // 2 byte data register
#define AD5933_RealData           0x94  // 2 byte data register
#define AD5933_ImaginaryData      0x96  // 2 byte data register

#define DAC_SLOPE_EEPROM_ADDRESS      0
#define DAC_OFFSET_EEPROM_ADDRESS     EEPROMBlockSize
#define DAC_LOW_LIMIT_EEPROM_ADDRESS   2 * EEPROMBlockSize
#define DAC_HIGH_LIMIT_EEPROM_ADDRESS  3 * EEPROMBlockSize

#define GAIN_0_EEPROM_ADDRESS         4 * EEPROMBlockSize
#define GAIN_1_EEPROM_ADDRESS         5 * EEPROMBlockSize
#define GAIN_2_EEPROM_ADDRESS         6 * EEPROMBlockSize
#define GAIN_3_EEPROM_ADDRESS         7 * EEPROMBlockSize

#define PHASE_SLOPE_EEPROM_ADDRESS    8 * EEPROMBlockSize // these are now the base addresses for these values- now that we have different calibration data for each of 24 settings on AD5933
#define PHASE_OFFSET_EEPROM_ADDRESS   9 * EEPROMBlockSize
#define Z_SLOPE_EEPROM_ADDRESS        10 * EEPROMBlockSize
#define Z_OFFSET_EEPROM_ADDRESS       11 * EEPROMBlockSize

  // after inclusion of calibration with external clock, memory start address for G_PER_V is 200 = (48 * 4) blocks of double values for 48 different settings, + 8 double values for DAC and TIA Gain calibration values
#define AD5933_G_PER_V_EEPROM_ADDRESS 200 * EEPROMBlockSize // prior to calibrations for external clock, starting block address 104 = (24 * 4) blocks of double values for impedance / phase calibration coefficients for 24 different settings, + 8 double values for DAC and TIA Gain calibration values
#define AD5933_BIAS_V_EEPROM_ADDRESS 201 * EEPROMBlockSize

#define V_BATT_FULL_EEPROM_ADDRESS    202 * EEPROMBlockSize  // note that this is simply a 16 bit number- 10 bit value summed 64 times (saved as 16 bit unsigned integer in a 2 byte block)
#define FCLK_CORRECTION_EEPROM_ADDRESS  203 * EEPROMBlockSize // this is the normalization factor for the AD5933 clock frequency relative to the nominal / specified value of 16.776MHz 

// Other system constants
#define fCLK                          16776000  // 16.776 MHz internal clock on AD5933
#define fMCLK                         250000    // 250 kHz external clock to MCLK of AD5933

const String firmWareVersion = "vnfluidex1.01\t";

// Addresses in EEPROM for calibration constants/ coefficients
const int EEPROMBlockSize = sizeof(double); // so maximum double precision values to store in EEPROM is 512 (4096 / 8)

boolean MCLKext = false;  // boolean to keep track of whether AD5933 is using external clock...

// calibration constants
double vslope = 0.996713;
double voffset = 0.00181;
double vlowlimit = -1.6147;
double vhighlimit = 1.6111;

double Gain0 = 994.0;   
double Gain1 = 9909.0;
double Gain2 = 99106.4;
double Gain3 = 985978.6; 

double AD5933_GperVoltOut = 10000.0;
double AD5933_BiasV = -0.026;

double phaseSlope[48];
double phaseOffset[48];
double ZSlope[48];
double ZOffset[48];

int technique;
int relayCounter = 0;

unsigned int ref100Charge = 64000;  // uncalibrated 100% battery value corresponding to 10 bit digital ADC read 3.3V range, through voltage divider 33k and 10k resistors- measured 64 times
double fCLK_Factor = 1.0; // ratio of actual AD5933 clock frequency to nominal / specified value of 16.776MHz...

const int LED_modulationPeriod = 1000;
int LED_perMilDutyCycle;
long LEDcycleTime;  // start of (on) period for modulating LED
long LEDoffTime;    // start of "off" portion of LED modulation...

const long timeoutInterval = 60000; // period in ms (1 minute) before device shuts itself off if idle (no communication received from Android
long shutoffTime;

unsigned int ShiftRegisterValue = 0x3000; // starting ABE-Stat_1_0_11, leave bits 12 and 13 high (default no power to AD5933 or its external clock)
//unsigned int ShiftRegisterValue = 0x0000; // first prototype i2c stopped working- SDA getting clipped- short circuit on AD5933 when latter is not powered?
byte ADG715SwitchStates = 0x00;

byte electrodeConfig = OPEN_CIRCUIT_CONFIG;

SPISettings ADS1220Settings(4000000, MSBFIRST, SPI_MODE1);  // SPI configuration for ADS1220 ADC
SPISettings AD5061Settings(30000000, MSBFIRST, SPI_MODE1);  // SPI configuration for AD5061; since no data is returned, can also use MODE2

union { // Union makes conversion from 4 bytes to an unsigned 32-bit int easy; mostly used for ADS1220 24 bit data reads
    uint8_t bytes[4];
    uint32_t word32;
  } data;

union { // here make a union of double with individual constituent bytes
  uint8_t doubleBytes[sizeof(double)];
  double floatingValue;
} doubleValue;

int16_t realZ, imaginaryZ; // real and imaginary parts of impedance- these are 16 bit signed (two's complement) integers...
double lastKnownZ;  // last known impedance value...
double lastKnownPhase;
double lowFZ; // effective DC impedance (assume phase = 0)
double battCharge;

byte TIAGainCode, excitationCode;  // TIA Gain code 0 - 3 => 100 10k 1M 100M; AD5933 Excitation code 0 - 3 => 200mV 400mV 1V 2V
boolean PGAx5Setting; // PGA setting on AD5933 (x5 when true; x1 when false)
boolean repeatedAD5933Measurement = false;  // flag used for AD5933 to know whether to issue a frequency increment command, or measure repeat command...
double biasVoltage = 0.0;
double admittanceMargin = 10000.0;
double TIAGain = 10000;

uint32_t completedCalibrationsCode = 0x0000;  // used to keep track of which settings of AD5933 have been calibrated in most recent invocation of calibration routine
boolean calibratedAD5933biasAndGperVolt = false;  // used to keep track of whether AD5933 bias and admittance values per output voltage have been calibrated on this cycle...

byte command;
double frequency;

void setup()
{
  battCharge = 100.0;
  LED_perMilDutyCycle = 1000; // initialize battery to full charge (don't let device shut down if initialized to uncharged state)
  
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(1);                                // give RF section time to shutdown
  
  // output pins controlling shift register
  pinMode(SER, OUTPUT); // outputs for driving shift registers/ expanded outputs
  pinMode(CS_RCK, OUTPUT); // also used as CS for SPI
  pinMode(CLOCK, OUTPUT);

  pinMode(0, INPUT);  // GPIO 00 is input (used to measure BT connected or ADS1220 !DRDY signals

  // SPI pins
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);

  pinMode(GPIO00, INPUT);

  pinMode(RELAY, OUTPUT);

  devicePower(true);  // activate digital pin to supply power from battery

  SPI.begin();
  int allocatedEEPROM  = (205 * EEPROMBlockSize); // we're storing 203 (205) double values of EEPROMBlockSize, + 1 16 bit integer for battery charge value...
              // Note for ABE-Stat_1_01_01 and beyond- assuming double requires 8 bytes of memory, we now need 1618 bytes of EEPROM
  EEPROM.begin(allocatedEEPROM); // initialize EEPROM with 1632 bytes (can assign up to 4096)

  // retrieve calibration constants...
  /*vslope = EEPROM_Double_Read(DAC_SLOPE_EEPROM_ADDRESS);
  voffset = EEPROM_Double_Read(DAC_OFFSET_EEPROM_ADDRESS);
  vlowlimit = EEPROM_Double_Read(DAC_LOW_LIMIT_EEPROM_ADDRESS);
  vhighlimit = EEPROM_Double_Read(DAC_HIGH_LIMIT_EEPROM_ADDRESS);
  
  Gain0 = EEPROM_Double_Read(GAIN_0_EEPROM_ADDRESS);
  Gain1 = EEPROM_Double_Read(GAIN_1_EEPROM_ADDRESS);
  Gain2 = EEPROM_Double_Read(GAIN_2_EEPROM_ADDRESS);
  Gain3 = EEPROM_Double_Read(GAIN_3_EEPROM_ADDRESS);*/

  AD5933_GperVoltOut = EEPROM_Double_Read(AD5933_G_PER_V_EEPROM_ADDRESS);
  //AD5933_BiasV = EEPROM_Double_Read(AD5933_BIAS_V_EEPROM_ADDRESS);
  //AD5933_BiasV = -0.026;  // hard code observed value on RNBT-D785
  AD5933_BiasV = 0.0;

  lastKnownZ = 10000.0; // just dummy value- best to use ADC (AD1220) for low frequency impedance measurement first to a starting estimate for Z at high frequencies
  lowFZ = 10000.0; // low frequency / DC impedance, assumed zero phase...
  lastKnownPhase = 0.0;
      // used to try to pick settings a priori that will not result in saturation of transimpedance amplifier

  //fCLK_Factor = EEPROM_Double_Read(FCLK_CORRECTION_EEPROM_ADDRESS); // recalls the calibrated value of AD5933 clock to nominal value from data sheet...

  /*for (int zCoefficient = 0; zCoefficient < 48; zCoefficient++) { // 48 AD5933 settings (4 excitation settings x 3 gain settings x 2 PGA Settings x 2 clock settings)
      int blockOffset = (4 * EEPROMBlockSize) * zCoefficient;  // offset from given base address for given index
      phaseSlope[zCoefficient] = EEPROM_Double_Read(PHASE_SLOPE_EEPROM_ADDRESS + blockOffset);
      phaseOffset[zCoefficient] = EEPROM_Double_Read(PHASE_OFFSET_EEPROM_ADDRESS + blockOffset);
      ZSlope[zCoefficient] = EEPROM_Double_Read(Z_SLOPE_EEPROM_ADDRESS + blockOffset);
      ZOffset[zCoefficient] = EEPROM_Double_Read(Z_OFFSET_EEPROM_ADDRESS + blockOffset);
  }*/

  // phaseSlope coefficients:

  phaseSlope[0] = 0.00074388;
  phaseSlope[1] = 0.00074;
  phaseSlope[2] = 0.00072837;
  phaseSlope[3] = 0.00071547;
  phaseSlope[4] = 0.00075676;
  phaseSlope[5] = 0.00075804;
  phaseSlope[6] = 0.00075069;
  phaseSlope[7] = 0.0007389;
  phaseSlope[8] = 0.00100005;
  phaseSlope[9] = 0.00100675;
  phaseSlope[10] = 0.0010005;
  phaseSlope[11] = 0.00098326;
  phaseSlope[12] = 0.00084526;
  phaseSlope[13] = 0.00083974;
  phaseSlope[14] = 0.00084615;
  phaseSlope[15] = 0.00084156;
  phaseSlope[16] = 0.0008662;
  phaseSlope[17] = 0.00085725;
  phaseSlope[18] = 0.00086842;
  phaseSlope[19] = 0.00085775;
  phaseSlope[20] = 0.00109426;
  phaseSlope[21] = 0.00106486;
  phaseSlope[22] = 0.00110368;
  phaseSlope[23] = 0.00110591;
  phaseSlope[24] = 0.02887541;
  phaseSlope[25] = 0.02891247;
  phaseSlope[26] = 0.02890272;
  phaseSlope[27] = 0.02885737;
  phaseSlope[28] = 0.02894855;
  phaseSlope[29] = 0.02875448;
  phaseSlope[30] = 0.02879983;
  phaseSlope[31] = 0.02881105;
  phaseSlope[32] = 0.02878666;
  phaseSlope[33] = 0.02899585;
  phaseSlope[34] = 0.02899098;
  phaseSlope[35] = 0.02885298;
  phaseSlope[36] = 0.02910215;
  phaseSlope[37] = 0.02894612;
  phaseSlope[38] = 0.02894465;
  phaseSlope[39] = 0.02883981;
  phaseSlope[40] = 0.02884762;
  phaseSlope[41] = 0.02869743;
  phaseSlope[42] = 0.0288876;
  phaseSlope[43] = 0.02890418;
  phaseSlope[44] = 0.02886956;
  phaseSlope[45] = 0.02878569;
  phaseSlope[46] = 0.02891198;
  phaseSlope[47] = 0.0288837;

  // phaseOffset coefficients:

  phaseOffset[0] = -91.59;
  phaseOffset[1] = -90.42;
  phaseOffset[2] = -90.11;
  phaseOffset[3] = -90.23;
  phaseOffset[4] = -91.78;
  phaseOffset[5] = -91;
  phaseOffset[6] = -90.21;
  phaseOffset[7] = -90.23;
  phaseOffset[8] = -90.54;
  phaseOffset[9] = -89.94;
  phaseOffset[10] = -89.25;
  phaseOffset[11] = -89.33;
  phaseOffset[12] = -91.55;
  phaseOffset[13] = -90.85;
  phaseOffset[14] = -90.09;
  phaseOffset[15] = -90.26;
  phaseOffset[16] = -91.63;
  phaseOffset[17] = -90.57;
  phaseOffset[18] = -90.04;
  phaseOffset[19] = -90.13;
  phaseOffset[20] = -90.09;
  phaseOffset[21] = -88.58;
  phaseOffset[22] = -88.8;
  phaseOffset[23] = -89.07;
  phaseOffset[24] = -91.32;
  phaseOffset[25] = -90.71;
  phaseOffset[26] = -90.4;
  phaseOffset[27] = -90.23;
  phaseOffset[28] = -91.19;
  phaseOffset[29] = -90.48;
  phaseOffset[30] = -90.3;
  phaseOffset[31] = -90.18;
  phaseOffset[32] = -91.08;
  phaseOffset[33] = -90.74;
  phaseOffset[34] = -90.45;
  phaseOffset[35] = -90.18;
  phaseOffset[36] = -91.62;
  phaseOffset[37] = -90.81;
  phaseOffset[38] = -90.39;
  phaseOffset[39] = -90.15;
  phaseOffset[40] = -91.32;
  phaseOffset[41] = -90.47;
  phaseOffset[42] = -90.31;
  phaseOffset[43] = -90.24;
  phaseOffset[44] = -91.22;
  phaseOffset[45] = -90.46;
  phaseOffset[46] = -90.39;
  phaseOffset[47] = -90.16;

  // New ZSlope coefficients:

  ZSlope[0] = 0.0364;
  ZSlope[1] = 0.0318;
  ZSlope[2] = 0.05;
  ZSlope[3] = 0.1552;
  ZSlope[4] = 0.5836;
  ZSlope[5] = 0.7775;
  ZSlope[6] = 0.7318;
  ZSlope[7] = 1.6107;
  ZSlope[8] = -7.2967;
  ZSlope[9] = -18.4232;
  ZSlope[10] = -53.9474;
  ZSlope[11] = -100.1027;
  ZSlope[12] = 0.0812;
  ZSlope[13] = 0.0272;
  ZSlope[14] = -0.5908;
  ZSlope[15] = -1.0462;
  ZSlope[16] = 1.564;
  ZSlope[17] = 0.5218;
  ZSlope[18] = -4.5991;
  ZSlope[19] = -8.2576;
  ZSlope[20] = -50.8909;
  ZSlope[21] = -121.2458;
  ZSlope[22] = -331.9083;
  ZSlope[23] = -661.0435;
  ZSlope[24] = 1.7714;
  ZSlope[25] = 0.8937;
  ZSlope[26] = -1.1944;
  ZSlope[27] = -5.3523;
  ZSlope[28] = 23.4932;
  ZSlope[29] = 13.9145;
  ZSlope[30] = -7.5888;
  ZSlope[31] = -48.0232;
  ZSlope[32] = 151.5172;
  ZSlope[33] = 77.7356;
  ZSlope[34] = 17.0924;
  ZSlope[35] = -496.5146;
  ZSlope[36] = 9.3649;
  ZSlope[37] = 7.0627;
  ZSlope[38] = -4.4325;
  ZSlope[39] = -8.0481;
  ZSlope[40] = 100.5074;
  ZSlope[41] = 36.4239;
  ZSlope[42] = -49.333;
  ZSlope[43] = -197.4861;
  ZSlope[44] = 596.9;
  ZSlope[45] = 784.796;
  ZSlope[46] = -157.5002;
  ZSlope[47] = -1648.9614;
  
  // Old ZSlope coefficients:

  /*ZSlope[0] = 0.0218;
  ZSlope[1] = 0.0068;
  ZSlope[2] = 0.0324;
  ZSlope[3] = 0.1297;
  ZSlope[4] = 0.3977;
  ZSlope[5] = 0.5313;
  ZSlope[6] = 0.6092;
  ZSlope[7] = 1.4366;
  ZSlope[8] = -9.0206;
  ZSlope[9] = -20.3332;
  ZSlope[10] = -52.1838;
  ZSlope[11] = -96.6556;
  ZSlope[12] = -0.018;
  ZSlope[13] = -0.0882;
  ZSlope[14] = -0.6711;
  ZSlope[15] = -1.0626;
  ZSlope[16] = 0.3888;
  ZSlope[17] = -0.4314;
  ZSlope[18] = -5.3683;
  ZSlope[19] = -9.1697;
  ZSlope[20] = -57.9119;
  ZSlope[21] = -112.1181;
  ZSlope[22] = -318.2485;
  ZSlope[23] = -637.1788;
  ZSlope[24] = 3.1237;
  ZSlope[25] = 0.6266;
  ZSlope[26] = -0.5284;
  ZSlope[27] = -4.4367;
  ZSlope[28] = 15.9879;
  ZSlope[29] = 13.0634;
  ZSlope[30] = 4.5759;
  ZSlope[31] = -47.5086;
  ZSlope[32] = 173.585;
  ZSlope[33] = 128.9891;
  ZSlope[34] = 16.6745;
  ZSlope[35] = -370.2566;
  ZSlope[36] = 11.2198;
  ZSlope[37] = 11.6002;
  ZSlope[38] = 1.508;
  ZSlope[39] = -19.8065;
  ZSlope[40] = 78.2058;
  ZSlope[41] = 49.6757;
  ZSlope[42] = 24.0114;
  ZSlope[43] = -168.933;
  ZSlope[44] = 1462.9072;
  ZSlope[45] = 930.2804;
  ZSlope[46] = -275.3431;
  ZSlope[47] = -2027.8133;*/

  // New ZOffset coefficients:

  ZOffset[0] = 108822;
  ZOffset[1] = 212560;
  ZOffset[2] = 548797;
  ZOffset[3] = 1103380;
  ZOffset[4] = 958442;
  ZOffset[5] = 1873769;
  ZOffset[6] = 4865436;
  ZOffset[7] = 9788776;
  ZOffset[8] = 9572811;
  ZOffset[9] = 18856891;
  ZOffset[10] = 48715931;
  ZOffset[11] = 97740320;
  ZOffset[12] = 545796;
  ZOffset[13] = 1067811;
  ZOffset[14] = 2759186;
  ZOffset[15] = 5541480;
  ZOffset[16] = 4787668;
  ZOffset[17] = 9439668;
  ZOffset[18] = 24457586;
  ZOffset[19] = 49175889;
  ZOffset[20] = 47419070;
  ZOffset[21] = 92559498;
  ZOffset[22] = 242559642;
  ZOffset[23] = 491066147;
  ZOffset[24] = 105644;
  ZOffset[25] = 211786;
  ZOffset[26] = 551133;
  ZOffset[27] = 1112148;
  ZOffset[28] = 925268;
  ZOffset[29] = 1867210;
  ZOffset[30] = 4880768;
  ZOffset[31] = 9866528;
  ZOffset[32] = 9258675;
  ZOffset[33] = 18524917;
  ZOffset[34] = 48163629;
  ZOffset[35] = 97522176;
  ZOffset[36] = 525949;
  ZOffset[37] = 1053535;
  ZOffset[38] = 2753185;
  ZOffset[39] = 5535652;
  ZOffset[40] = 4640752;
  ZOffset[41] = 9384117;
  ZOffset[42] = 24438991;
  ZOffset[43] = 49320873;
  ZOffset[44] = 46532838;
  ZOffset[45] = 92372063;
  ZOffset[46] = 241269783;
  ZOffset[47] = 486751223;
  
  // Old ZOffset coefficients:

  /*ZOffset[0] = 109652;
  ZOffset[1] = 214022;
  ZOffset[2] = 549828;
  ZOffset[3] = 1103499;
  ZOffset[4] = 970895;
  ZOffset[5] = 1889148;
  ZOffset[6] = 4868537;
  ZOffset[7] = 9792911;
  ZOffset[8] = 9730342;
  ZOffset[9] = 19044115;
  ZOffset[10] = 48727776;
  ZOffset[11] = 97740732;
  ZOffset[12] = 553093;
  ZOffset[13] = 1074869;
  ZOffset[14] = 2760910;
  ZOffset[15] = 5532679;
  ZOffset[16] = 4871120;
  ZOffset[17] = 9506107;
  ZOffset[18] = 24497426;
  ZOffset[19] = 49245509;
  ZOffset[20] = 48136854;
  ZOffset[21] = 92174175;
  ZOffset[22] = 242242316;
  ZOffset[23] = 490600145;
  ZOffset[24] = 104129;
  ZOffset[25] = 211606;
  ZOffset[26] = 549648;
  ZOffset[27] = 1110945;
  ZOffset[28] = 928521;
  ZOffset[29] = 1867906;
  ZOffset[30] = 4866136;
  ZOffset[31] = 9857671;
  ZOffset[32] = 9217687;
  ZOffset[33] = 18463468;
  ZOffset[34] = 48200029;
  ZOffset[35] = 97346760;
  ZOffset[36] = 526950;
  ZOffset[37] = 1049217;
  ZOffset[38] = 2742487;
  ZOffset[39] = 5532935;
  ZOffset[40] = 4660727;
  ZOffset[41] = 9371741;
  ZOffset[42] = 24349401;
  ZOffset[43] = 49324134;
  ZOffset[44] = 45816725;
  ZOffset[45] = 92165659;
  ZOffset[46] = 241229489;
  ZOffset[47] = 487202528;*/

  ref100Charge = EEPROM_Int_Read(V_BATT_FULL_EEPROM_ADDRESS);

  Wire.begin(); //Note on ESP8266 Wire.pins(int sda, int scl) from http://www.devacron.com/arduino-ide-for-esp8266/
  Wire.setClock(400000L); // upper limit on speed- AD5933 and ADG715 max clock rate for SCL is 400000Hz// to be safe comment this out...
  //Wire.setClock(100000L); // apparently rising edge time constant on SDA is ~1us on some devices, so slow clock down a little bit for clear communication...
  //Wire.setClock(50000L);
  // operate Wire / i2c port at slower speed- when AD5933 is off capacitance of inputs slows down data line transitions :-(

  Serial.begin(BAUD);
  
  Configure_ADS1220();  // configure default settings on ADS1220 Analog to Digital Converter

  // initialize variables to obviously contrived values, to facilitate troubleshooting (if functions don't update values with real data

  TIAGainCode = 0x01; // initialize TIA gain code to 0x01 (for AD5933 test, only use 0x01 and 0x02- 10k and 1M)
  excitationCode = 0x00; // initialize excitation code to 0x00 (200 mVp-p, so weakest excitation signal)
  PGAx5Setting = false; // initially set AD5933 PGA gain to 1

  // configure potentiostat network
  setTIAGain(TIAGainCode);  // enable feedback on transimpedance amplifier- gain doesn't matter for test (otherwise working potential will float) argument of 1 uses 10k feedback resistor

  electrodeConfig = OPEN_CIRCUIT_CONFIG;
  resetElectrodeConfig();
                          
  Release_GPIO00(); // don't connect GPIO00 to !DRDY or BT_Connected (allow user a time to reset ESP8266 into programming mode)

  illuminateLED();  // make sure power is supplied to tri-color LED
  redLED(true); // then turn on red LED of tri-color initially just to make sure it works

  delay(500);  // use a software delay function here to enable program to be reset to reprogram mode
      //- now we're changing the code to generate a sin wave as fast as possible, but still only measure the thermocouple voltage periodically (so removing delays from main loop- won't get another opportunity to keep GPIO00 released)

  for (int i = 0; i <= 4; i ++) {
    // Let's turn on the relay...
    digitalWrite(RELAY, LOW);
    delay(50);
  
    // Let's turn off the relay...
    digitalWrite(RELAY, HIGH);
    delay(50);
  }

  for (int i = 0; i <= 4; i ++) {
    // Let's turn on the relay...
    digitalWrite(RELAY, LOW);
    delay(100);
  
    // Let's turn off the relay...
    digitalWrite(RELAY, HIGH);
    delay(100);
  }

  for (int i = 0; i <= 4; i ++) {
    // Let's turn on the relay...
    digitalWrite(RELAY, LOW);
    delay(200);
  
    // Let's turn off the relay...
    digitalWrite(RELAY, HIGH);
    delay(200);
  }
  
  redLED(false);  // then turn LED back off (should go back to green, unless bluetooth is connected which is highly unlikely at startup)
  
  DAC_AD5061_Connect(true); // connect AD5061 Digital to Analog Converter from network
  AD5933_Connect(false);  // disconnect network analyzer input by default
  AD5933_PowerOn(true); // make sure supply reaches power pin of AD5933 (otherwise it will slow down i2c data line)
  AD5933_PowerDown();  // and just put in power down mode digitally (to save current and diminish noise contributions of DDS and other AD5933 systems)
  setTIAGain(0x01);  // make sure selected feedback resistor connected for TIA
  TIA_LMP7721();
  shutoffTime = timeoutInterval + millis();
  batteryChargePercent();
  long startyMcStartyPants = millis();
  LEDcycleTime = startyMcStartyPants + LED_modulationPeriod;
  LEDoffTime = startyMcStartyPants + LED_perMilDutyCycle;  // set times for LED modulation...
}

void loop() {
    Configure_ADS1220();  // configure default settings (normal mode, 50/60Hz recursive filters, data ready signal only on !DRDYpin) here; may have been changed during analyses...
    while(Serial.available()) {
      command = Serial.read();
      //ADS1220_ReportTemperature();  // always send the temperature data no matter what electrical measurements are requested
      //batteryReport();  // also report battery charge every time data is requested
      double computedValue0, computedValue1;
      switch(command) { // in this example, data is only communicated from Android device connected through bluetooth; ESP8266 WiFi only receives data passively to share to internet
        case 'a': // code that an analytical method is selected- first decode what analysis...
          startAnalyticalMethod();
          break;
        case 'q':
          Calibrate();
          break;
        case 'f': // code from Android to set the cell configuration ('2' is 2 cell, '3' is 3 cell, '0' or any other is open circuit)
          setElectrodeConfig();
          break;
        case 'i': // code from Android requesting current
          computedValue0 = cellCurrent();
          computedValue1 = cellPotential();
          Serial.print('i');
          Serial.print(computedValue0, 3); // cellCurrent is in nA, so returned resolution is pA
          Serial.print('\t');
          Serial.print(computedValue1, 1);
          Serial.print('\t');
          break;
        case 'n':
          /*Serial.print(Gain0);
          Serial.print('\t');
          Serial.print(Gain1);
          Serial.print('\t');
          Serial.print(Gain2);
          Serial.print('\t');
          Serial.print(Gain3);*/
          Serial.print(firmWareVersion); // response to request for firmware version
          break;
        case 'o': // Android requests open circuit potential
          openCircuitConfig();
          DAC_AD5061_Connect(false);  // important to disconnect source of voltage from amplifier
          AD5933_Connect(false);
          TIA_LMP7721();
          setTIAGain(0x00); // use lowest gain for transimpedance amp (pulls working closer to reference)
          computedValue0 = -ADS1220_Diff_Voltage(0x04, 0x00) / 1000.0;// value returned is in mV
          Serial.print('o');
          Serial.print(computedValue0, 4);  // in mV, resolution uV
          Serial.print('\t');
          break;
        case 'r':// code from Android requesting cell resistance
          computedValue0 = cellResistance();
          Serial.print('r');
          Serial.print(computedValue0, 1);
          Serial.print('\t');
          break;
        case 'v': // code to set bias voltage on cell (for constant bias current measurement)
          DAC_AD5061_SetCalibratedVoltage(Serial.parseFloat());
          break; 
        case 'z': // code from Android requesting cell capacitance (also send measured phase)
          computedValue0 = cellCapacitance();
          computedValue1 = correctPhase();
          Serial.print('z');
          Serial.print(computedValue0, 0);
          Serial.print('\t');
          Serial.print(computedValue1, 1);
          Serial.print('\t');
          break;
        default:
          break;
      }
      devicePower(true);  // any time we receive communication restore battery power (will prevent shutoff if remove USB cable), and reset timeout clock
      shutoffTime = millis() + timeoutInterval;
    }
    if (millis() < (shutoffTime - timeoutInterval)) shutoffTime = millis() + timeoutInterval; // timer overrun- reset shutoffTime...
    if (millis() > shutoffTime) {
        devicePower(false); // remove battery power from device if timed out...
    }
    if (battCharge < 1.0) blinkOff();
    if (millis() > LEDcycleTime) {
        illuminateLED();  // turn on LED...
        batteryChargePercent(); // measure battery state
        LEDoffTime = LED_perMilDutyCycle + LEDcycleTime;  // value is updated in batteryChargePercent() function- is battery charge in % * 10...
        LEDcycleTime += LED_modulationPeriod;
    }
    if (millis() > LEDoffTime) {
        if (LED_perMilDutyCycle < 1000) noLED();  // turn off LED for rest of cycle (if not 100% duty cycle), and tentatively set next value to turn off...
        LEDoffTime += LEDcycleTime;
    }
}

void setElectrodeConfig() {
    while(!Serial.available()); // wait for character to arrive
    boolean changedConfig = false;
    char bcmd = Serial.read();
    if (bcmd == '2') {
      if (electrodeConfig != TWO_ELECTRODE_CONFIG) changedConfig = true;
      electrodeConfig = TWO_ELECTRODE_CONFIG;
    }
    else if (bcmd == '3') {
      if (electrodeConfig != THREE_ELECTRODE_CONFIG) changedConfig = true;
      electrodeConfig = THREE_ELECTRODE_CONFIG;
    }
    else {
      if (electrodeConfig != OPEN_CIRCUIT_CONFIG) changedConfig = true;
      electrodeConfig = OPEN_CIRCUIT_CONFIG;
    }
    if (changedConfig) {
      /*
       * Following block taken from start of CellCapacitance function; electrode configurations not getting set properly except after 
       * executing CellCapacitance() (!?!?!?); it looks like i2c bus doesn't work properly (for communicating to ADG715 switches)
       * if AD5933 i2c device is not powered (?)
       */
      //AD5933_PowerOn(true); // have to turn on AD5933 to use it!
      DAC_AD5061_Connect(true); // connect DAC from AD5061 to network...  // calibration appears to be different with and without AD5061, so go ahead and connect,
          // then for calibrations set bias to 0- so make all measurements connected to this network (and apply "0" bias / output voltage)
      biasVoltage = 0.0;
      //DAC_AD5061_SetCalibratedVoltage(biasVoltage - (AD5933_BiasV)); // here we'll apply unbiased voltage
      DAC_AD5061_SetCalibratedVoltage(biasVoltage);
      AD5933_Connect(true); // connect cell input to network analyzer
      TIA_AD5933(); // connect cell output to network analyzer...
      setTIAGain(0x02); // use 2nd most sensitive gain- 1000000 ohm resistor results in poor amplifier performance on network analyzer
      /*
       * end of block required for consistent setting of electrode configuration (!?!?!?)
       */

      resetElectrodeConfig();
    }
}

void blinkOff() {
    for (int i = 0; i < 3; i++) {
        noLED();
        delay(500);
        illuminateLED();
        delay(500);
    }
    devicePower(false);
}
