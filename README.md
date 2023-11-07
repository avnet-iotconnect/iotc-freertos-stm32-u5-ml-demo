# Introduction

This project is based on AWS's and ST's collaboration GitHub project
[aws-stm32-ml-at-edge-accelerator](https://github.com/aws-samples/aws-stm32-ml-at-edge-accelerator)

The original project support building the Machine Learning Model with SageMaker
, generating the model C and header files
, integrating the generated files by building a signed image with a headless build on AWS
, automatically provisioning the device with credentials 
and automatically re-building the project with CodeBuild and pushing it to the device va AWS's OTA model.

This project is a reduced version of the original project that is focusing on building and configuring the device 
locally with pre-generated AI model files. The pre-generated model files are 
subject to the [ST SLA0044](models/LICENSE.pdf) license.




