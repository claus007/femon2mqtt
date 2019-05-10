# femon2mqtt
monitor your satellite dish and send the result to mosquitto / Openhab
Linux only !

## Building
for building you need qmake (sorry for beeing lazy)
for aptians try ONE of those:

  * sudo apt-get install qt4-qmake
  * sudo apt-get install qt5-qmake

then do:

> qmake femonitor.pro
> make

## Running

### Running the easy way

> sudo nohup ./femonitor &

### Running the correct way

> sudo mv femonitor /usr/bin

> sudo chown root /usr/bin/femonitor

> sudo chmod u+s /usr/bin/femonitor

Then you can run femonitor as normal user

## Options
```
femonitor -h        this help message
          -H        m-host to connect to (default localhost)
          -p        m-port to connect to (default 1883)   
          -t        m-topic (default 'FS20/SateliteDish_<Adapter>_<Frontend>)   
          -a        adapter (default 0)   
          -f        frontend (default 0)   
          -v        verbose mode
```
