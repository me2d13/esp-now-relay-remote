# esp-now-relay-remote

This is platform.io project for esp32-c6 device.

## Project background

This is one part of home automation project which can control relay(s) over
* esp-now message from paired device (remote control)
* esp-now message from mqtt<->esp-now bridge

## Included components
* esp-now-relay - controls relay to open gate or do another action
* __esp-now-relay-remote__ - battery powered ESP32-C6 device to control relay e.g. from car (this repo)
* esp-now-gw-mqtt - mqtt<->esp-now bridge - wifi and mqtt part
* esp-now-gw-transmitter - mqtt<->esp-now bridge - esp-now part

## Features
This is code for battery powered remote control (built on ESP32-C6) which controls relay module via esp-now messages. Messages are encrypted with random seed and rolling code inside (so captured message can't be replayed). Deep sleep is supported for efficient battery usage. Battery level is monitored and sent via message to relay module so can be transferred further (e.g. to home automation system).

## Button press
* single press - send the command to relay module
* long press (between 3 and 10 seconds) - pairing attempt, target (relay) module must be set to pairing mode to accept pairing
* very long press (> 10 seconds) - clear pairing for this button

## LED responses to button press
* 5 quick flashes - button pressed but not paired
* 1 flash 0.5 second - command sent
* 3 flashes (1 sec each) - pairing attempt
* 1 long flash (5 seconds) - pairing ereased