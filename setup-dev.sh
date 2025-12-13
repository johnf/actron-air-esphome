#!/bin/bash

if [ ! -d "cdeps/esphome-repo" ]; then
  git clone https://github.com/esphome/esphome.git cdeps/esphome-repo
fi

if [ ! -d "cdeps/ArduinoJson" ]; then
  git clone git@github.com:bblanchon/ArduinoJson.git cdeps/ArduinoJson
fi
