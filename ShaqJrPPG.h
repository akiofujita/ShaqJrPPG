/*
 FILENAME:	ShaqJrPPG.h
 AUTHOR:	Akio K. Fujita
 EMAIL:		akiofujita13@gmail.com
 
 With assistance from Orlando S. Hoilett
 
 Please see .cpp file for extended descriptions, instructions, and version updates
 
 
 */
	
 
#ifndef ShaqJrPPG_h
#define ShaqJrPPG_h


// Standard Arduino Libraries
#include <Arduino.h>

// Custom External Libraries
#include "AD524X.h"

class ShaqJrPPG
{

private:

	// potentiometer to control gain of TIA and Bandpass filter
	AD524X ad5242; 
	
	// Gain settings for digital potentiometers
	uint8_t R_ledCurrent;
	uint8_t R_tiaGain;
	uint8_t R_ppgGain;
	
	bool ledState; // true = ON, false = OFF

public:

	ShaqJrPPG(); // must be called in setup in .ino code
	void begin(); // must be called in setup in .ino code

	// Control LED
	void turnLEDOn();
	void turnLEDOff();
	bool getLEDStatus() const;
	void setLEDCurrent(uint8_t val);
	uint8_t getLEDCurrent() const;
	void balanceLEDCurrent();
	
	
	// Amplifier gain control for TIA
	void setTIAGain(uint8_t gain);
	uint8_t getTIAGain() const;
	void balanceTIAGain();
	
	// Amplifier gain control for Bandpass filter
	void setPPGGain(uint8_t gain);
	uint8_t getPPGGain() const;
	
	
	// Read voltage of PPG circuit
	uint16_t getTIA() const;
	uint16_t getPPG() const; // bandpass filters
	
};


#endif /* ShaqJrPPG_h */


/*

	turn LED on and off
	control brightness of LED
	
	control gain of transimpedance amplifier (TIA)
	control gain of bandpass filter
	
*/