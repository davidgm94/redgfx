#!/bin/sh
rm -r out/
rm compile_commands.json
mkdir out/
pushd out
cmake .. -DCMAKE_BUILD_TYPE=Debug
cp compile_commands.json ..
popd

