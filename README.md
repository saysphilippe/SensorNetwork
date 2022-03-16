# SensorNetwork
Firmware and documentation for the SensorNetwork
The SensorNode runs ESPNOW as default when shipped out. You can choose to overwrite this code if you want to. The circuit is build with multiple purposes in mind.
1) The circuit will be set up up with a micro switch that when opened it will allow power to go thru the MOSFET and power the rest of the circuit. When the switch is closed it will not use any power. Based on measurements it uses 1/4 second to wake up when powered, send battery voltage and temperature from the Dallas DS18B20 which is mounted on the board.
2) If you would like to have the SensorNode on power only to be steared by firmware thru the deep sleep for x amount of time, you can solder the VIN brigde AND solder a wire between PIN 16 and RESET as shown on the illustration X. 
Be aware of the different power usage on the two different setup's.
