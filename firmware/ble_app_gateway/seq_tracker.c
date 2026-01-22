#include "stddef.h"
#include "stdint.h"
#include "seq_tracker.h"

#define SEQ_TRACK_MAX_DEVICES 16

typedef struct
{
    uint16_t device_id;
    uint8_t  last_seq;
    bool     valid;
} seq_track_entry_t;

static seq_track_entry_t m_table[SEQ_TRACK_MAX_DEVICES];

void seq_tracker_reset(void)
{
    for (int i = 0; i < SEQ_TRACK_MAX_DEVICES; i++)
        m_table[i].valid = false;
}

/* 回傳 true = 接受（新資料） */
bool seq_tracker_accept(uint16_t device_id, uint8_t seq)
{
    seq_track_entry_t * free_slot = NULL;

    for (int i = 0; i < SEQ_TRACK_MAX_DEVICES; i++)
    {
        if (m_table[i].valid)
        {
            if (m_table[i].device_id == device_id)
            {
                int8_t diff = (int8_t)(seq - m_table[i].last_seq);

                if (diff <= 0)
                {
                    /* duplicate or old */
                    return false;
                }

                /* new */
                m_table[i].last_seq = seq;
                return true;
            }
        }
        else if (!free_slot)
        {
            free_slot = &m_table[i];
        }
    }

    /* 新 device */
    if (free_slot)
    {
        free_slot->device_id = device_id;
        free_slot->last_seq  = seq;
        free_slot->valid     = true;
        return true;
    }

    /* table full → 保守策略：丟 */
    return false;
}
