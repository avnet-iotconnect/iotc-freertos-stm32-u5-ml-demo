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

# Development Setup

* Execute [scripts/setup-project.sh](scripts/setup-project.sh). This will populate the files from the AWS original repo
and the [ml-source-fsd50k](models/ml-source-fsd50k) model files from the [models](models) directory.
* Open the STM32CubeIDE. When prompted for the workspace path navigate to the [stm32](stm32) directory.
Note that the workspace has to be located there, or otherwise dependencies will not work correctly. 
The stm32 directory name indirection is preserved from the original project in order to make easy transition
to the AWS AI and build framework.
* Select *File -> Open Projects From File System* from the menu. Navigate to the stm32 directory
in this repo and click *Open*.
* Uncheck all folders that appear in the Folders list and leave the *stm32/Projects/b_u585i_iot02s_ntz* directory checked.
* Click *Finish*.




