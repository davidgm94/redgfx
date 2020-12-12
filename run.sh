#/bin/sh
pushd out
make
if [ $? -eq 0 ]; then
    echo Compilation succeeded!
    ./redgfx
else
    echo Compilation failed!
fi
popd

