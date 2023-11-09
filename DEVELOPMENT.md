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

