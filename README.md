#[push.tf](http://push.tf)

Command line client for push.tf cloud storage service. 


### Development
Developement is in beta stage and a many features are still in the pipe.


### Compilation
```shell
gcc *.c -Wall -lcurl -o pushtf
```
This tool depends of [libcurl](http://http://curl.haxx.se/libcurl). It compiles (at least once) under Linux x86 (32/64 bit), Raspberry Pi, Mac OSX (Tiger, Snow Leopard), FreeBSD.

Pre-compiled standalone binaries are available on [push.tf](http://push.tf) :
```shell
# Linux
wget http://push.tf/linux/pushtf

# Raspberry Pi
wget http://push.tf/rpi/pushtf

# Mac OSX
curl -O http://push.tf/mac/pushtf
```


### Usage
```
push.tf command line client
Usage:
   pushtf [options] file_or_ID

Options:
  -d | --debug    	debug mode
  -g | --get		get a file
  -h | --help		this help
  -o <filename>		output filename w/ --get
  -q | --quiet		quiet mode
  -v | --verbose	verbose mode
  -V | --version	display components versions

Push files:
./pushtf FILE [FILE ..]

Get files:
./pushtf -g ID [ID ..]
```

### Examples
#### Send file
```shell
$ pushtf datas.zip 
100 % [****************************************************] 82392529 bytes
datas.zip : http://push.tf/8091
```

#### Retrieve file
```shell
$ pushtf -g 8091
100 % [****************************************************] 82392529 bytes
filename : datas.zip
```

#### Send piped data
```shell
$ tar zcf - directory | pushtf -
4669104 bytes - 112 KB/s
(null) : http://push.tf/8059
```

#### Send data in a pipe
```shell
$ pushtf -g 8059 -o - | tar zxf -
```
or
```shell
$ curl http://push.tf/8059 | tar zxf -
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 4593k  100 4593k    0     0  1034k      0  0:00:04  0:00:04 --:--:-- 1065k
```
