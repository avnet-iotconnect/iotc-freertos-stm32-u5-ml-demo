#!/bin/bash

set -e

this_dir=$(dirname "${0}")

cd ${this_dir}/..

mlpath=./models/ml-source-fsd50k

if [[ -n "${1}" ]]; then
  mlpath="${1}"
  shift
fi

wsdir=./stm32

rm -rf aws-stm32-ml-at-edge-accelerator
git clone https://github.com/aws-samples/aws-stm32-ml-at-edge-accelerator
pushd aws-stm32-ml-at-edge-accelerator >/dev/null
  git reset --hard e17843d19ba0610da596cfc1cea90d6cbbaf94f9
popd >/dev/null

exit -1
cp -rf /stm32ai_files/Inc/* ${wsdir}/Middleware/STM32_AI_Library/Inc
s3sync s3://${bucket}/${mlpath}/stm32ai_files/Lib/ ${wsdir}/Middleware/STM32_AI_Library/Lib/
mv ${wsdir}/Middleware/STM32_AI_Library/Lib/NetworkRuntime730_CM33_GCC.a ${wsdir}/Middleware/STM32_AI_Library/Lib/NetworkRuntime800_CM33_GCC.a
s3sync s3://${bucket}/${mlpath}/C_header ${wsdir}/Projects/Common/dpu/
aws s3 cp --no-progress --recursive --exclude '*' --include 'network*' \
  s3://${bucket}/${mlpath}/stm32ai_files/ ${wsdir}/Projects/Common/X-CUBE-AI/App/
