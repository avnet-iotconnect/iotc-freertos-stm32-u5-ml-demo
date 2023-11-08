# Quickstart Instructions

## Firmware Flashing

* Download and install a serial console application (TeraTerm for example) and STM32CubeProgrammer
* Download and unzip [b_u585i_iot02a_ntz_v1_0.zip](https://saleshosted.z13.web.core.windows.net/demo/st/b_u585i_iot02a_ntz_v1_0.zip)
* Connect the board with a Micro USB to a PC cable (**not** the USB-C port)  
* Open the STM32CubeProgrammer and connect to the board by clicking the *Connect* button on the top right.
* Click the *Erasing&Programming* button (second button on the left sidebar) 
  * It is sometimes beneficial to run a *Full Chip Erase* (top right of the screen), but this step is optional.
  * Click *Browse* and navigate to your unzipped .bin file.
  * *Start Address* should be auto-detected to 0x08000000
  * Click the *Start Programming* button.
    
![STM32CubeProgrammer](media/programmer-flash.png "STM32CubeProgrammer")

Once flashing is complete Disconnect the board from the programmer and unplug the device.

## IoTConnect setup
* Add the following items to your template of choice:
  * "version" - data type STRING
  * "class" - data type STRING
* Create a new device
* Note the following values in the device *Connection info* screen 
which we will use for the device runtime configuration in the next step:
  * Your Unique Device ID that you used to create the device will be used as *mqtt_endpoint*.
  * Host, which will be used as *thing_name*.
  * Reporting topic will be in a format: $aws/rules/msg_d2c_rpt/<yourdevice>/<cd>/2.1/0.
Note the 7-capital-letter string, which we will use as *iotc_cd* value in the next steps. 
* Plug in the device and open a serial terminal application.
* Enter the following commands to configure your device:
  * conf set wifi_ssid your-wifi-ssid 
  * conf set wifi_credential your-wifi-password
  * conf set thing_name your-device-id
  * conf set mqtt_endpoint your-endpoint
  * conf set mqtt_port 8883
* Verify values by typing  **conf get** and examining the output.
* Type **conf commit**. Note that must commit the changes so that they take effect.
* Type **reset** to reset the device.

