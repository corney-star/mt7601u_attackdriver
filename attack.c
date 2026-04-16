//study only!!!!!

#include "mt7601.h"
#include "tx.h"

static const u8 CTS_Frame[] = {
    0x14, 0x00,
    0xff, 0x7f,
    0x7c, 0x3d, 0x09, 0x80, 0x83, 0x53,
};

void mt7601u_cts_attack(struct mt7601u_dev *dev){
    struct sk_buff *skb;
    
}


