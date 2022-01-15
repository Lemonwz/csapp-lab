####执行make btest报错

```
$ make btest
$ ./btest
```

![""](multilib.png "multilib")

####错误原因：
系统中的gcc没有安装multilib 库,使用这个库可以在64位的机器上产生32位的程序

####解决方案：
sudo apt-get install gcc-multilib
sudo apt-get install g++-multilib