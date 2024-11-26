# Linux DVB driver for Letv/Aiwa/CVB DtmbUSB DTMB demodulator USB dongle

#### ubuntu上使用openwrt SDK交叉编译

```shell
#下载SDK并解压
wget https://downloads.immortalwrt.org/releases/23.05.2/targets/x86/64/immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz
tar xvf immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz

# 下载源码
git clone https://github.com/nxdong520/DtmbUSB.git
cd DtmbUSB

# 设置环境变量(X86_64架构)
export ARCH=x86
export STAGING_DIR=../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-12.3.0_musl/bin/
export KERNEL_DIR=../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/build_dir/target-x86_64_musl/linux-x86_64/linux-5.15.150
export TOOLCHAIN="../immortalwrt-sdk-23.05.2-x86-64_gcc-12.3.0_musl.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-12.3.0_musl/bin/x86_64-openwrt-linux-"

# 编译准备
make prepare

#开始编译
make
```

#### 将生成的内核驱动程序ko文件复制到目标机器
```shell
#加载驱动程序
insmod dvb-core.ko
insmod dvb-usb.ko.ko
insmod dtmbusb-fe.ko
insmod dtmbusb-dev.ko

root@ImmortalWrt:~# dmesg
[61682.709315] dvb-usb: found a 'DtmbUSB DTMB demodulator' in warm state.
[61682.715857] dvb-usb: will pass the complete MPEG2 transport stream to the software demuxer.
[61682.724357] dvbdev: DVB: registering new adapter (DtmbUSB DTMB demodulator)
[61682.732048] usb 1-1.1: DtmbUSB DTMB demodulator firmware version:3.8.3268
[61682.738889] usb 1-1.1: DVB: registering adapter 0 frontend 0 (Letv DtmbUSB DTMB demodulator)...
[61682.747695] dvb-usb: DtmbUSB DTMB demodulator successfully initialized and connected.
[61682.755659] usbcore: registered new interface driver DtmbUSB

root@ImmortalWrt:~# ls /dev/dvb/adapter0/ -al
drwxr-xr-x    2 root     root           120 Nov 25 16:29 .
drwxr-xr-x    4 root     root            80 Nov 24 23:20 ..
crw-------    1 root     root      212,   4 Nov 25 16:29 demux0
crw-------    1 root     root      212,   5 Nov 25 16:29 dvr0
crw-------    1 root     root      212,   3 Nov 25 16:29 frontend0
crw-------    1 root     root      212,   7 Nov 25 16:29 net0

```

#### 说明

支持“乐视/爱华/CVB” DTMB棒   
驱动成功加载后出现/dev/dvb/adapterx/目录   
提供给所有支持标准linux dvb api的软件使用   
本源码通讯协议通过usb抓包取得，并参考网上的LeDtmb windows软件源码，感谢提供者！   
