# Salvaduino
This is a university project developed by Simone Scaravati(me), NoahRosa and Stefano Radaelli.
Sharon is a robot car, that is able to self-tune its motors for moving exactly straight forward without external correction (which is really common, beacause 5v motors for arduino are really imprecise sometimes!).

* It's controllable in two ways:
    * From the sofware Sharon Path Finder, written with Processing, that allows to create a precise Point-to-Point path which will be executed by the car
    * AndrOSC app (informations below) which allows to drive the car manually and also to let her drive automatically, thanks to a simple algorithm that uses the distance sensor put in the front of the car.


#### Library used:
* https://github.com/CNMAT/OSC  //osc protocol
* https://playground.arduino.cc/Code/NewPing  //library for distance sensor
* Inside Processing: go to library manager and download NetP5 and OscP5

## Using sdsdf
sdfsdfsdf