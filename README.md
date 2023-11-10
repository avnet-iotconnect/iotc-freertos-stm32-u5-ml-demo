# Introduction

This project is based on AWS's and ST's collaboration GitHub project
[aws-stm32-ml-at-edge-accelerator](https://github.com/aws-samples/aws-stm32-ml-at-edge-accelerator) for the 
[B-U585I-IOT02A Discovery kit for IoT node with STM32U5 series](https://www.st.com/en/evaluation-tools/B-U585I-IOT02A.html)
board.

The original project in the AWS's repo supports building the Machine Learning Model with SageMaker
, generating the model C and header files
, integrating the generated files by building a signed image with a headless build on AWS
, automatically provisioning the device with credentials 
and automatically re-building the project with CodeBuild and pushing it to the device va AWS's OTA model.

This project is a reduced version of the original project that is focusing on building and configuring the device 
locally with pre-generated AI model files. The pre-generated model files are 
subject to the [ST SLA0044](models/LICENSE.pdf) license.

The sounds recognized by this version are:
Speech, Crying_and_sobbing, Glass, Gunshot_and_gunfire, Knock.

Telemetry also has a **confidence** report number value along with **class**.  
The confidence number is a percentage ranging from 0 to 100.
In reality, numbers lower than 30 or so will not be reported,
as it is more likely that the model will be more confident 
in the "other" detection.
Telemetry will be reported only when there are sounds.
If class="other" is reported it means that it detected a sound, 
but nothing is recognized.

# Instructions

* If you wish to try the project out on your board, see the [Quickstart Guide](QUICKSTART.md).
* If you wish to compile and run this project, see the [Development Instructions](DEVELOPMENT.md).