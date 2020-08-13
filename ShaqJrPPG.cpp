/*
 FILENAME:      ShaqJrPPG.cpp
 AUTHOR:        Akio K. Fujita
 EMAIL:			akiofujita13@gmail.com
 VERSION:		1.0.0
 
 With assistance from Orlando S. Hoilett
 
 ABBREVIATIONS
 
 PPG - Photoplethysmography - Process of measuring changes in volume in light.
	Used to measure heart rate, respiration, and blood oxygen by measuring
 	changes in volume in the blood vessels.
 
 BPF - Bandpass filters
 
 I2C
 
 
 DESCRIPTION
 
 
 A FEW RESOURCES:
 
 Analog-to-Digital Conversion
 https://www.instructables.com/id/Analog-to-Digital-Conversion-Tutorial/
 
 I2C
 
 Digital Potentiometers
 
 
 UPDATES
 
 
 */
 
#include "ShaqJrPPG.h"


const uint8_t ledPin = 30;
const uint8_t ppgPin = A4;
const uint8_t tiaPin = A5;
const uint8_t ad5171_address = 0x2D;
const uint8_t ad5242_address = 0x2C;



// DEFAULT CONSTRUCTOR
// Must be called before setup in .ino code
ShaqJrPPG::ShaqJrPPG()
{
}


// Initializes functions, member variables, and member objects of the PPG class
// Must be called before setup in .ino code
void ShaqJrPPG::begin()
{
	pinMode(ledPin, OUTPUT);
	turnLEDOff();
	
	analogReadResolution(12); // 12-bits = 0-4095
	
	setLEDCurrent(63); // smallest LED current by default.
	
	// AD5242 digital potentiometer
	// Controls the gain of the photometric front-end
	ad5242 = AD524X(0x2C);
}


// void ShaqJrPPG::turnLEDOn() const
// Turns on LED by writing HIGH to pin. LED is driven using an NPN bipolar junction transistor (BJT).
void ShaqJrPPG::turnLEDOn()
{
	digitalWrite(ledPin, HIGH);
	ledState = true;
}


// void ShaqJrPPG::turnLEDOff() const
// Turns off LED by writing LOW to pin. LED is driven using an NPN bipolar junction transistor (BJT).
void ShaqJrPPG::turnLEDOff()
{
	digitalWrite(ledPin, LOW);
	ledState = false;
}

// bool ShaqJrPPG::getLEDStatus() const
// Returns whether or not the LED is on (true) or off (false).
bool ShaqJrPPG::getLEDStatus() const
{
	return ledState;
}


// Code Reference: https://www.arduino.cc/en/Tutorial/DigitalPotentiometer
//val	value to write to AD5171. Must be between 0 and 63.
//
// This function writes to the digital potentiometer (AD5171 in this version)
// that controls the brightness of the LED.
//
// Current into the LED is determined by the following equations:
// 		I_B = (3.3V-0.6V)/(R_pot + R9)
//		LED_current = I_B * Gain
//		Current gain of an NPN transistor is
//		often written as HFE in datasheets
//
// "val" has inverse relationship w/ LED current
// As val goes up, resistance goes up, LED current goes down... vice versa
void ShaqJrPPG::setLEDCurrent(uint8_t val)
{
	if (val > 63) val = 63;
	
	Wire.beginTransmission(ad5171_address); // address specified in datasheet
	// device address is specified in datasheet
	Wire.write(byte(0x00)); // sends instruction byte
	Wire.write(val); // sends potentiometer value byte (between 0 and 63)
	Wire.endTransmission(); // stop transmitting
	
	R_ledCurrent = val;
}


// uint8_t ShaqJrPPG::getLEDCurrent() const
//
// Returns the setting of the digital potentiometer controlling the LED current.
uint8_t ShaqJrPPG::getLEDCurrent() const
{
	return R_ledCurrent;
}


// void ShaqJrPPG::balanceLEDCurrent()
//
// Balances the LED brightness w/ the TIA gain. Sets the TIA gain to a nominal value given the TIA max range. 
// Setting the digital potentiometer (AD5242) to decimal = 17, gives a resistance of 67kOhms. By doing so, the TIA gain can be moduled in at least one order of magnitdue in either direction, smaller (6.7 kOhms) or larger (667 kOhms).
//
// Then adjust LED brightness around this nominal TIA gain.
void ShaqJrPPG::balanceLEDCurrent()
{
	// Set to some nominal value
	setTIAGain(17);
	
	// Add a bit of delay for settling time
	delay(1);
	
	// Measure TIA output voltage
	uint16_t Vtia = getTIA();
	
	// the bounds are about 10% of 1/2 of full ADC scale
	// adjust the IR current so the TIA fits these bounds
	while (Vtia > 2250 || Vtia < 1850) 
	{
		// If LED is too bright (voltage output of TIA is
		// too large, then decrease LED brightness
		if (Vtia > 2250 && R_ledCurrent > 0) setLEDCurrent(R_ledCurrent - 1);
		
		// If LED is too dim (voltage output of TIA is
		// too small, then increase LED brightness
		else if (Vtia < 1850 && R_ledCurrent < 63) setLEDCurrent(R_ledCurrent + 1);
		
		// If already at min or max of LED current, then adjust TIA gain
		else balanceTIAGain();
		
		// add a bit of delay for settling time
		delay(1); 
		
		// Check the TIA voltage again
		Vtia = getTIA();
	}
}
	


//void ShaqJrPPG::setTIAGain(uint8_t gain)
//gain		value between 0-255 to set resistance of digital potentiometer
//
// Sets gain of digital potentiometer connected to transimpedance amplifier.
// As gain increases, resistance of potentiometer increases, gain of TIA increases, output voltage increases.
void ShaqJrPPG::setTIAGain(uint8_t gain)
{
	ad5242.write(0, gain);
	R_tiaGain = gain;
}


// uint8_t ShaqJrPPG::getPPGGain() const
//
// Returns gain setting of digital potentiometer connected to TIA
uint8_t ShaqJrPPG::getTIAGain() const
{
	return R_tiaGain;
}


// void ShaqJrPPG::balanceTIAGain()
//
// Balances the TIA gain w/ the LED gain. Sets the TIA gain to 1/2 full ADC. 
// Since the PPG circuit has a "virtual ground" at 1/2 full ADC, setting the output of the TIA
// gives the PPG circuit the lowest settling time and highest responsivity.
void ShaqJrPPG::balanceTIAGain()
{
	// Measure TIA output voltage
	uint16_t Vtia = getTIA();
	
	while (Vtia > 2250 || Vtia < 1850)
	{
		// If voltage output of TIA is too large, then decrease TIA gain
		if (Vtia > 2250 && R_tiaGain > 0) setTIAGain(R_tiaGain - 1);
		
		// If voltage output of TIA is too small, then increase TIA gain
		else if (Vtia < 1850 && R_tiaGain < 255) setTIAGain(R_tiaGain + 1);
		
		// If R_tiaGain parameter is at min (0) or max (255), then adjust LED current
		else balanceLEDCurrent();
		
		delay(1); // add a bit of delay for settling time
		
		Vtia = getTIA();
	}
}


// void ShaqJrPPG::setPPGGain(uint8_t gain)
// gain		value between 0-255 to set resistance of digital potentiometer
//
// Sets gain of digital potentiometer connected to transimpedance amplifier.
// As gain parameter increases, resistance of potentiometer increases, gain of bandpass filter increases, output voltage increases.
void ShaqJrPPG::setPPGGain(uint8_t gain)
{
	ad5242.write(1, gain);
	R_ppgGain = gain;
}


// uint8_t ShaqJrPPG::getPPGGain() const
//
// Returns gain setting of digital potentiometer connected to bandpass filter
uint8_t ShaqJrPPG::getPPGGain() const
{
	return R_ppgGain;
}


// uint16_t ShaqJrPPG::getTIA() const
//
// Reads voltage of transimpedance amplifier
uint16_t ShaqJrPPG::getTIA() const
{
	return analogRead(tiaPin);
}


// uint16_t ShaqJrPPG::getPPG() const
//
// Reads voltage of the bandpass filter of the PPG circuit.
uint16_t ShaqJrPPG::getPPG() const
{
	return analogRead(ppgPin);
}