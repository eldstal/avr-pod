/*
 * avr-pod.c
 * V-USB based joystick emulator
 *
 * Hardware: An ATMEGA328-PU with the following connections:
 *  Pin 1: 4.7k pull-up to +5V
 *  Pin 7: +5v
 *  Pin 8: Ground
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include "descriptor.h"
#include "podcontrols.h"

static uchar idleRate;

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

int main(void) {
	wdt_enable(WDTO_1S);
  initPodControls();

	/* Even if you don't use the watchdog, turn it off here. On newer devices,
	 * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
	 */
	/* RESET status: all port bits are inputs without pull-up.
	 * That's the way we need D+ and D-. Therefore we don't need any
	 * additional hardware initialization.
	 */
	usbInit();
	usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
	uchar i = 0;
	while(--i){             /* fake USB disconnect for > 250 ms */
			wdt_reset();
			_delay_ms(1);
	}

	usbDeviceConnect();
	sei();
	for(;;){                /* main event loop */
			wdt_reset();
			usbPoll();
			if(usbInterruptIsReady()){
				/* called after every poll of the interrupt endpoint */
				usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
			} else {
				updateSensorData();
        updateLEDState();
      }
	}
}
