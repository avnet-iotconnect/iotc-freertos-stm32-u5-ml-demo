#!/bin/bash

set -e

this_dir=$(dirname "${0}")

cd ${this_dir}/..

mlpath=./models/ml-source-fsd50k-alerts

if [[ -n "${1}" ]]; then
  mlpath="${1}"
  shift
fi

wsdir=./stm32

rm -rf aws-stm32-ml-at-edge-accelerator
git clone https://github.com/aws-samples/aws-stm32-ml-at-edge-accelerator
pushd aws-stm32-ml-at-edge-accelerator >/dev/null
  git reset --hard b8271253a2d811a6db184fef35fcdd94d505b848
popd >/dev/null

cp -rn aws-stm32-ml-at-edge-accelerator/stm32/* ./stm32/
rm -rf aws-stm32-ml-at-edge-accelerator

mkdir -p ${wsdir}/Middleware/STM32_AI_Library/Inc
mkdir -p ${wsdir}/Middleware/STM32_AI_Library/Lib

echo Mpplying the model at ${mlpath}...
cp -rf ${mlpath}/stm32ai_files/Inc/* ${wsdir}/Middleware/STM32_AI_Library/Inc/
cp -f ${mlpath}/stm32ai_files/Lib/NetworkRuntime730_CM33_GCC.a ${wsdir}/Middleware/STM32_AI_Library/Lib/NetworkRuntime800_CM33_GCC.a
cp -rf ${mlpath}/C_header/* ${wsdir}/Projects/Common/dpu/
cp -rf ${mlpath}/stm32ai_files/network*.c ${wsdir}/Projects/Common/X-CUBE-AI/App/
cp -rf ${mlpath}/stm32ai_files/network*.h ${wsdir}/Projects/Common/X-CUBE-AI/App/
echo Done.