#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SOF          0xAA
#define MAX_PAYLOAD  16

typedef enum
{
    WAIT_SOF,
    GET_CMD,
    GET_LEN,
    GET_PAYLOAD,
    GET_CHECKSUM
} State;

typedef struct
{
    State    state;
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  payload[MAX_PAYLOAD];
    uint8_t  index;
    uint8_t  checksum;
    uint32_t last_time;
    uint32_t timeout;
} Parser;

#define PARSER_OK       1
#define PARSER_BUSY     0
#define PARSER_CRC_ERR -1
#define PARSER_TIMEOUT -2



void parser_reset(Parser *p)
{
    p->state    = WAIT_SOF;
    p->cmd      = 0;
    p->len      = 0;
    p->index    = 0;
    p->checksum = 0;
    memset(p->payload, 0, sizeof(p->payload));
}

void parser_init(Parser *p, uint32_t timeout)
{
    parser_reset(p);
    p->timeout   = timeout;
    p->last_time = 0;
}

int parser_feed(Parser *p, uint8_t byte, uint32_t time)
{
    if (p->timeout &&
        p->state != WAIT_SOF &&
        (time - p->last_time) > p->timeout)
    {
        parser_reset(p);
        return PARSER_TIMEOUT;
    }

    p->last_time = time;

    switch (p->state)
    {
        case WAIT_SOF:
            if (byte == SOF)
                p->state = GET_CMD;
            break;

        case GET_CMD:
            p->cmd      = byte;
            p->checksum = byte;   
            p->state    = GET_LEN;
            break;

        case GET_LEN:
            if (byte > MAX_PAYLOAD)
            {
                parser_reset(p);
                break;
            }
            p->len      = byte;
            p->checksum ^= byte;
            p->index    = 0;
            p->state    = (byte == 0) ? GET_CHECKSUM : GET_PAYLOAD;
            break;

        case GET_PAYLOAD:
            p->payload[p->index++] = byte;
            p->checksum ^= byte;
            if (p->index >= p->len)
                p->state = GET_CHECKSUM;
            break;

        case GET_CHECKSUM:
            if (byte == p->checksum)
                return PARSER_OK;
            parser_reset(p);
            return PARSER_CRC_ERR;
    }

    return PARSER_BUSY;
}

void dump_frame(Parser *p)
{
    int i;

    printf("FRAME OK  CMD=0x%02X  LEN=%u  PAYLOAD=[", p->cmd, p->len);
    for (i = 0; i < p->len; i++)
    {
        printf("%02X", p->payload[i]);
        if (i < p->len - 1)
            printf(" ");
    }
    printf("]\n");
}

void feed_stream(Parser   *p,
                 uint8_t   data[],
                 uint32_t  times[],
                 int       count)
{
    int i, ret;

    for (i = 0; i < count; i++)
    {
        ret = parser_feed(p, data[i], times[i]);

        printf("t=%3ums  byte=0x%02X  ->  ", times[i], data[i]);

        if (ret == PARSER_BUSY)
        {
            printf("receiving...\n");
        }
        else if (ret == PARSER_OK)
        {
            dump_frame(p);
            parser_reset(p);
        }
        else if (ret == PARSER_CRC_ERR)
        {
            printf("CHECKSUM ERROR -- parser reset\n");
        }
        else if (ret == PARSER_TIMEOUT)
        {
            printf("TIMEOUT (%ums gap > %ums) -- parser reset\n",
                   times[i] - p->last_time,
                   p->timeout);

            ret = parser_feed(p, data[i], times[i]);
            printf("t=%3ums  byte=0x%02X  ->  receiving... (re-fed)\n",
                   times[i], data[i]);
        }
    }
}


int main(void)
{
    Parser p;

    printf("===== Test 1 : valid frame =====\n");

    parser_init(&p, 50);

    uint8_t  d1[] = { 0xAA, 0x01, 0x03, 0x10, 0x20, 0x30, 0x02 };
    uint32_t t1[] = {    0,    5,   10,   15,   20,   25,   30  };

    feed_stream(&p, d1, t1, 7);

    printf("\n===== Test 2 : timeout + recovery =====\n");

    parser_init(&p, 50);

    uint8_t d2[] =
    {
        0xAA, 0x01, 0x03, 0x10,   
        0xAA, 0x05, 0x01, 0x7F, 0x7B
    };
    uint32_t t2[] =
    {
           0,    5,   10,   15,
         200,  205,  210,  215,  220
    };

    feed_stream(&p, d2, t2, 9);

    printf("\n===== Test 3 : back-to-back frames =====\n");

    parser_init(&p, 50);

    uint8_t d3[] =
    {
        0xAA, 0x03, 0x01, 0x55, 0x57,
        0xAA, 0x04, 0x02, 0xAA, 0xBB, 0x17
    };
    uint32_t t3[] =
    {
        0, 5, 10, 15, 20,
        25, 30, 35, 40, 45, 50
    };

    feed_stream(&p, d3, t3, 11);

    printf("\n===== Test 4 : timeout disabled (same stream as test 2) =====\n");
    printf("(expect no timeout -- bad checksum instead)\n\n");

    parser_init(&p, 0);

    feed_stream(&p, d2, t2, 9);

    return 0;
}
