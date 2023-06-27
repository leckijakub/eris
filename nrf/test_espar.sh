#!/bin/bash

if [ -z $1 ]; then
        echo "Pass the test ID as first argument"
        exit 1
fi

TEST_ID=$1
MEASUREMENTS_DIR="measurements"

mkdir -p "${MEASUREMENTS_DIR}"

pylink-rtt nRF52840_xxAA | tee -a "${MEASUREMENTS_DIR}/${TEST_ID}.log"


