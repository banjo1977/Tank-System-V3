Contour Tank System upgrade Contour tank system is a SENSESP instantiaton which takes readings from 6 analogue inputs: 3 x fresh water tanks 2 x fuel tanks 1 x black water (sewage) tank.

The tanks are read by barometic pressure sensing devices which output 0-5vDC.

The SENSESP framework outputs these values to the SENSESP server which is connected on a WIFI lan. This system works extremely well.

HOWEVER - the server can fail and we want to ensure independent visibility of the tank level, regardless of whether the server is operating. We also want there to be an independent buzzer.

As such:

Data from the tanks is averaged and output to bar graphs on a 4.2" e-paper display

The display updates every 60 seconds

If the 'display update' touch pin is activated (connected to a screw on the case) the display is updated.

If the black wate tank exceeds a certain level, then the buzzer will be activated IF the buzzer function is enabled (this is controlled by a second touch pin).

The display also includes an icon for buzzer on / off, an icon showing wifi signal strenght, and a status line which:

on boot displays software version number and IP address
Thereafter displays the last update time of the tank data. Time is drawn from the signalk server.
TO DO:

Shift accross to a good display front-lit e-paper display
activate the front light for a defined period of time when the button is touched
