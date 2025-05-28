Contour Tank System upgrade
Contour tank system is a SENSESP instantiaton which takes readings from 6 analogue inputs:
3 x fresh water tanks
2 x fuel tanks
1 x black water (sewage) tank.

The tanks are read by barometic pressure sensing devices which output 0-5vDC.  A resistor network converts this to 0-3v. 

The SENSESP framework outputs these values to the SENSESP server which is connected on a WIFI lan. This system works extremely well. 

HOWEVER - I want the tank level display to be visible regardless of whether the server is working; and for there to be an independent buzzer.  The status of this buzzer can be addressable via Signal K.  (I have an example of this that I can send you).

The following functions are (in theory) available in hardware as built but not instantiated in software:
Paperwhite display (1 every 120 second update of tank levels (0-100%) using bar graph)
Buzzer (to be triggered by black water tank, but also cancelled by signalK input if required)  To be reflected as a signal K remote relay.
2 x touch sensors.  For use as required (maybe 1 is used to silence the buzzer, the other is used to toggle or refresh the display).

You have the details of the paper white display. 

This image shows the current pin usage; you may advise that this needs to change. 

The touch sensors are wired up as per guidance; I have proven that they work in hardware (ie I’ve been able to detect ‘touched’ or not, but I’ve not been able to integrate this into functionality and I was not able to make the display work.

I will send the current version of the software (which works)

If you need hardware to prototype - especially the display, I’m happy to fund that. 
link: https://docs.google.com/document/d/1ol7EuxyLQOXvyz_BNhHbE6N20tq0fCEktmN29jaxFCo/edit?pli=1&tab=t.0
