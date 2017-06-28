echo "Cross-compiling a non-working ESP32 example"

export PATH=$PATH:$ESP32_TOOLS/xtensa-esp32-elf/bin

# Verify that the ESP32 SDK is at a tested commit
pushd $IDF_PATH
idf_commit=$(git rev-parse HEAD)
tested_idf_commit=53893297299e207029679dc99b7fb33151bdd415
popd
if [ $idf_commit = $tested_idf_commit ]
    then echo "ESP32 SDK commit is okay"
else 
    echo "ESP32 SDK commit is " $idf_commit
    echo "ESP32 SDK commit should be " $tested_idf_commit
    echo "Error: ESP32 SDK commit is wrong"
    exit 1 
fi

cd build_all/esp32
echo "building in" $(pwd)
make
if [ $? = 0 ]
    then echo "built okay"
else 
    echo "!!!FAILED!!! build failed"
    exit result
fi
