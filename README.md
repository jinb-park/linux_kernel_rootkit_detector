# linux kernel rootkit detector (lkrd)

Linux kernel rootkit detector (lkrd)
Detect rootkit by machine learning technique.

## lkrd_tool

Provide lkrd functions.
You can see usage from lkrd_tool.py.

## lkrd_daemon & lkrd_driver

lkrd_daemon and lkrd_driver provide runtime rootkit detection.
lkrd_driver is under driver/*, It hooks LSM's kernel module inspection function.

lkrd_driver provide a kernel module name to be inspected.
and lkrd_daemon get the name from lkrd_drvier. and inspect it, send result to lkrd_driver.

you can see the result from /var/log/lkrd_daemon.log
result 1 ==> rootkit
result 0 ==> not rootkit

Do not run both lkrd_daemon and lkrd_tool at the same time.

## how to run lkrd_daemon

* run
```
cd drvier
make
cd ..
sudo ./install.sh
sudo insmod driver/lkrd.ko
sudo python lkrd_daemon.py start
```

* test
```
sudo insmod test.ko
cat /var/log/lkrd_daemon.log
```

## how to stop lkrd_daemon

```
sudo python lkrd_daemon.py stop
sudo rmmod lkrd
```

## license

This software is made available under the open source MIT License. © 2016 Jinbum Park

## contact

* e-mail. Jinbum Park <jinb.park7@gmail.com>
* blog. http://blog.daum.net/tlos6733
