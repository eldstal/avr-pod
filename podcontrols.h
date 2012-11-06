#pragma once

// 10 bytes of data about the state of the sensors
typedef struct{
		unsigned char  ax[7];			// One byte per analog dial/slider
    unsigned char  switches;		// three lower bits represent a switch each
} report_t;

typedef struct {
    unsigned char indicators;
    unsigned char switches;
} led_t;


extern report_t reportBuffer;
extern led_t LEDBuffer;

// Initialize I/O ports
void initPodControls();

// Fetch some kind of (not necessarily all!) sensor data to the global report buffer
// If this is called repeatedly, the reportbuffer will be filled eventually.
void updateSensorData();

// Do something to the LED outputs.
// As with updateSensorData(), this must be called periodically to ensure all is shown.
void updateLEDState();
