aa999 Coursework 5
Adam Ashing
Peterhouse

The modifications made to the Warp firmware include the driver files:

devSSD1331, devSoilsense & devBH1750

And the main loop in warp-kl03-ksdk1.1-boot lines 1353 - 1400.

As well as the CMake file.

The system takes a user input in the SEGGER terminal for how many minutes the system should run, and then performs a loop where 10 readings are taken of each measurand and the mean plotted on the OLED display. The functions are called from the the relevant driver files which are compiled into the warp firmware.

The system aims to plot 3 different colour lines (red, yellow and blue) on the OLED over the given period of time showing the variation of temperature, incident light and soil moisture respectively. s