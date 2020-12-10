#!/bin/sh
rm -r out/
mkdir out/
pushd out
cmake .. -DCMAKE_BUILD_TYPE=Debug
cp compile_commands.json ..
popd

