nagnu
=====

![nagnu](http://frlen.github.io/nagnu.png)

Nagios dashboard

### Tested with
* Nagios 3.2.3
* Nagios 3.5.1
* Nagios 4.0.8

### Tested on
* MacOS X 10.9
* FreeBSD 10.2
* OpenBSD 5.6
* Ubuntu 14.04
* Raspbian (wheezy)

### Build instructions
git clone https://github.com/frlen/nagnu.git  
cd nagnu  
./configure  
make

### Configuration
-c [configfile]

See nagnu.conf.sample

### Excludes
-e [excludesfile]

Place service_description or hostname in a file, one per line, to exclude them from the output.
