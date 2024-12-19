#!/bin/bash

arduino-cli compile -e -b arduino:avr:mega \
  --libraries ../../.. \
  --build-property build.extra_flags="-DDEBUG -I../../src" .
