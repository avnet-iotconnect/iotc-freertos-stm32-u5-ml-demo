# ML Audio Classifier Demo:  Setup
<img src="media/sound-classifier-dashboard.png" alt="drawing" width="300"/>

## Overview
This guide is designed to assist users in setting up and demonstrating an ML-based audio classification system using Avnet IoTConnect powered by AWS and the STMicroelectronics STM32U5 Discovery Kit built on the ultra-low-power STM32U585AII6Q microcontroller based on the Arm® Cortex®-M33 core with Arm® TrustZone®. This system demonstrates real-time machine learning inference on edge devices, highlighting the integration of cloud and IoT technologies.

While the foundation of the technology can support the classification of events based on most any sensor input (temperature, sound, motion, current, ect.), this solution utilizes the Discovery Kit's 2 microphones for audio classification.  The trained audio models are stored in the Models folder of the project, and with automation of the included _../scripts/setup-project.sh_ script, these models are incorporated into the project and the MCU binary is built.  The out-of-the-box demo incorporates 5 audio classifications to identify the following scenarios:  _Honking, Car Racing, Alarms, Dog(s) Barking, and Rushing Water_.    When an event is triggered by the corresponding sound, the event name, confidence level, and GPS location of the audio classifier node is sent to the cloud to alert the user.

This guide outlines the steps to recreate the demo. 

## ML Audio Classifier Sensor Nodes
### Program and Provision
Follow the [Quickstart Guide](https://github.com/avnet-iotconnect/iotc-freertos-stm32-u5-ml-demo/blob/main/QUICKSTART.md) to flash the STM32U5 Discovery Kit and provision audio classifier devices into IoTConnect.  

### Device Commands
**set-confidence-threshold**
-   **Purpose:** This command sets a global confidence threshold that an audio event must exceed to be considered valid. It's essential for reducing false positives and ensuring that the system only reacts to events with a high probability of accuracy.
-   **Usage:** `set-confidence-threshold [threshold_value]`
-   **Example:** `set-confidence-threshold 75`
-   **Explanation:** Here, the threshold is set at 75%. Any audio event classified below this confidence level will not trigger an alert or action.

**set-confidence-offsets**
-   **Purpose:** Adjusts the confidence offsets for different types of audio events, allowing for customized sensitivity settings for each sound classification. This command is crucial for tailoring the device's response based on specific sound characteristics.
-   **Usage:** `set-confidence-offsets [Alarm], [Bark], [Liquid], [Race_car_and_auto_racing], [Vehicle_horn_and_car_horn_and_honking], [other]`
-   **Default Values:**
    -   Alarm: 0
    -   Bark: -49 
    -   Liquid: -19
    -   Race car and auto racing: 39
    -   Vehicle horn, car horn, and honking: 27
    -   Other: -25
-   **Example:** `set-confidence-offsets 0, -45, -19, 39, 27, -25`
-    **Explanation:** This example sets the confidence offsets where notably, the offset for "Bark" is adjusted from -49 to -45. This adjustment slightly increases the likelihood that barking sounds are classified as positive detections, reflecting changes made to enhance detection accuracy for this specific sound type.

**set-device-location**
-   **Purpose:** Updates or sets the geographical location of the device. This command is particularly useful for mobile or relocated devices where location-specific data can significantly impact the context and relevance of audio event classifications.
-   **Usage:** `set-device-location [latitude] [longitude]`
-   **Example:** `set-device-location 34.0522 -118.2437`
-   **Explanation:** This example sets the device location to specific coordinates (latitude 34.0522, longitude -118.2437), which could be crucial for applications that rely on geographical data for functionality or reporting.

**set-inactivity-timeout**
-   **Purpose:** This command configures a timeout period that begins after an audio event is detected. During this time, the device suppresses further alerts of the same type, displaying a "no activity" status instead. This helps prevent overlapping alerts when sounds occur in quick succession.
- **Usage:** `set-inactivity-timeout [units]`
-   **Example:** `set-inactivity-timeout 800`
-   **Explanation:** In this example, the timeout is set for 800 units, where each unit represents 1/100th of a second, totaling 8 seconds. If the device detects an audio event, it will not issue another alert for the same event type for the next 8 seconds, thereby managing the frequency of alerts effectively.

## Audio Samples
<details>
<summary>Expand</summary>
Audio clips to use for this demo can be downloaded [here](https://saleshosted.z13.web.core.windows.net/demo/st/iotc-freertos-stm32-u5-ml-demo/audio-samples.zip)
These clips have been extracted from the FDS50K libraries at [Freesound.org](https://annotator.freesound.org/fsd/release/FSD50K)  and edited as follows:

 - Stereo to Mono:  The conversion from stereo to mono ensures that the same audio clip is played by each of the speakers, regardless if the left or right channel is selected.
- Converted to .wav files:  Not required as VLC player can play various encoding types, but for the purpose of using identical codecs, converted to ensure audio settings are constant when the sample is played.
- Clip sample Edits:  When required, dead-space has been removed from the front-end of the sample.  And in some cases, multiple samples have been extracted from the same clip when the sounds are vastly different.

This curated sample group represents the types of activities that would be of interest in an urban environment at a street corner and includes barking, water rushing, honking, car racing, and alarms. Sounds that depict violence were not chosen because of various venue's content policies and includes gunshots, explosions, car break-ins (glass shattering), fights/arguments, and screaming.

![Event Notifications](https://saleshosted.z13.web.core.windows.net/media/stmicro/reinvent23/readme/MLstatuslabels.gif)

</details>

## Demonstration Setup
### Overview
The following section outlines the setup for demonstrating the Urban Sound Event Classifier.  In this configuration, 2 audio classification devices are acoustically isolated from one another, each representing 2 physical locations on a dashboard map. This demo setup uses an edge server to play the audio samples.  This edge server is connected to IoTConnect and uses the Python SDK to receive C2D commands, as well as send basic telemetry from the edge server.  The edge server will play the selected audio sample when the appropriate command is received to a connected audio device.  In addition, the edge server will play the audio sound on the chosen audio channel.
### Materials

- [STM32U5 Discovery Kit Board](https://www.avnet.com/shop/us/products/stmicroelectronics/b-u585i-iot02a-3074457345647217745)![enter image description here](https://www.st.com/bin/ecommerce/api/image.PF271412.en.feature-description-include-personalized-no-cpn-large.jpg)
- [Clear Pelican Case](https://www.amazon.com/gp/product/B0C73G2WXJ)
	- The purpose of the case is the following:
		- Group the demo components into and enclosed system that best represents a final product.
		- Isolate the sound to a specific Sensor Node, allowing a demonstration of 2 node locations with different audio classifications.
		- Keep the audio volume down, as sirens, alarms, and other distracting noises are being played.
			- _Note, eliminating outside noise to prevent false classifications is not the reason for the case.  While this was the primary concern, the audio classification models work well in a noisy environment.  Confidence levels can be adjusted to prevent false notifications. However, the audio sample must be heard by the Sensor Node, so the case does help this cause._
- [Battery Pack](https://www.amazon.com/gp/product/B07CZDXDG8)
	- Battery pack provides power to the STM32U5 board, allowing the node to be completely wireless.  Caution, the minimal current draw can cause the battery pack output to cycle, resetting the board.  Plugging in the Bluetooth speakers increases the current draw and can prevent power cycling.
	
[Bluetooth Stereo Speakers](https://www.amazon.com/gp/product/B078SLF7YF)


## Audio Generator
### Verify Python Installation
Verify if Python3 is installed.  If installed, you will want to link it to “python”, and if not, you will want to install python.
```
python3 –version
```
if yes, link python to python3:
```
sudo ln -s /usr/bin/python3 /usr/bin/python
```
If not, install python:
```
sudo apt update
sudo apt install python3
sudo ln -s /usr/bin/python3 /usr/bin/python
python --version
```
### Install External Python Library – psutil

**psutil (Python system and process utilities)** is a cross-platform library for retrieving information on running processes and system utilization (CPU, memory, disks, network, sensors) in Python. It is optional, but installed to provide information about the linux machine to IoTConnect and allows remote monitoring of the system.
```
pip install psutil
```
**Verify installation** by fetching the CPU usage and the number of logical CPUs in the system:
```
Python
> psutil.cpu_percent()
> psutil.cpu_count(logical=True)
> Ctrl-D (to exit Python shell)
```
### Install PulseAudio Control and VLC Player

**Install PulseAudio Control**:  Used to manage audio syncs (left and right channels).  This allows us to play the audio sample on the left speaker, right speaker, or both and allow the demo to isolate the audio to 1 sensor when 2 sensors are being represented.
```
sudo apt-get install pulseaudio
```
**Install VLC Player:** VLC allows use to play audio through the command line and can also allow us to trigger an audio event through an IoTConnect command using systemd calls.
```
sudo apt-get install vlc
```
### Download the IoTConnect Python SDK

The IoTConnect Python SDK will be running on the Sound Generator.  We will be using the IoTConnect Python SDK V1.1 which is referenced in the documentation site [here](https://docs.iotconnect.io/iotconnect/sdk/sdk-flavors/python/python-sdk-1-1/).  This SDK is based on IoTConnect [Device Message version 2.1](https://docs.iotconnect.io/iotconnect/sdk/message-protocol/device-message-2-1/), which supports both Azure and AWS connectivity.

 1. Create a Work directory: If such a directory does not exist, create one.  for the purpose of this guide, directory “work” has been created at the user home directory.
```
mkdir work
cd work
```
 2. Download and Extract the SDK package:
```
wget -O iotc-python-sdk-std21-patch.zip https://github.com/avnet-iotconnect/iotc-python-sdk/archive/std21-patch.zip && unzip iotc-python-sdk-std21-patch.zip
```
 3. Navigate to the SDK directory
```
cd iotc-python-sdk-std21-patch
```
 4. Install the SDK libraries
```
pip3 install iotconnect-sdk.tar.gz
```
 5. Download and unzip the sound generator custom script to the ‘sample’ directory
```
wget -O iotc_sound_generator.zip https://saleshosted.z13.web.core.windows.net/demo/st/iotc-freertos-stm32-u5-ml-demo/sound_generator/iotc_sound_generator.zip && unzip iotc_sound_generator.zip -d ./sample
```

### Create a Device Template in IoTConnect

 1. Download and extract the sample device template:** A device template has been provided for you to import.  Once imported, it can be modified to support any additional functionality you require.
```
wget -O soundgen.zip https://saleshosted.z13.web.core.windows.net/media/stmicro/reinvent23/templates/soundgen.zip && unzip soundgen.zip -d ./sample
```
 2. Open IoTConnect in a browser (FireFox on your Ubuntu machine)
 3. Select Device from the left-hand toolbar
 4. Select templates
5. Browse to see if the template, “soundgen” already exists.  If so, you can skip to the next section.
6. If “soundgen” does not already exist, select Import
7. Browse for the downloaded template and select the soundgen.json file stored in the sample directory.  Note, the template is saved once it is imported.

### Create a Device and Export AWS Certificates from IoTConnect

1. Select Device from the left-hand tool bar
2. Select, “create device”
3. Enter Device Information:  Enter a unique ID and Display Name, select the Entity where the device should reside, and the template “soundgen”.  Leave “auto-generated” selected for the Device Certificate. You can now select “Save & View”.  Take note: do not enter spaces in the UniqeID or Display Name.  While the display name does not have formatting requirements, it is used to generate certificate file names.  By leaving out spaces, you will save yourself time. Also, Unique ID and Display Name can have the same value. Make note of the Unique ID you entered as you will need it later.

4. Download the AWS-generated device certificates.
	- Select “Connection Info” from the device you just created.
	- Select the certificate icon to download the certificate files
5. Unzip the certs and move them into the “aws_cert” folder in the cloned repository. In my case, the device display name is “MCLi7”, and the certificates are stored in the generated zip file “MCLi7-certificates.zip”
mple/aws_cert

### Gather your Device Credentials

Make note of the following Device credentials

- UniqueID
	- This is the device id you provided when you created the device. It is also noted in the Device List and as the ClientID in the connection info.
	- Copy and store the UniqueID for future use.
- SID (Python SDK ID)
	- You can get get the SID from the IoTConnect UI portal "Settings -> Key Vault -> SDK Identities -> select language Python and Version 2.1"
	- On the toolbar, navigate to settings -> key vault
	- Select SDK Identities -> select language Python and Version 2.1".  Copy and store the Identity key for future use.

## Modify the sample application

 1. Open the iotc_sound_generator.py script in an editor and modify the following:
	- Enter your UniqueID and SID values
	- Enter the correct paths to your SSL Key and SSL Cert
	- Save the Python script

## Run and Test the Script

 1. To run the Script
```
python iotc_sound_generator.py
```
 2. In the terminal, the device should load its certificates and connect to IoTConnect.  You will see telemetry from the terminal every 5 seconds when it is connected.
 3. Go to the device in IoTConnect, and enter the command window view.  Here you will be able to enter the command, such as z_channel, by selecting the command and entering a parameter value.  Execute the command and see if the the values are accepted in the terminal window.
 4. Using this same window, select an Audio sample, such as Bark, and enter a value 1-14 and see if it plays through your speakers.  If it does not, make sure the speaker is selected correctly in your Ubuntu Settings, and test with the VLC player GUI interface.
