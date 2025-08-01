/**
  ******************************************************************************
  * @file        : key.h
  * @author      : xxx
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : GPIO Key Driver Header File
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.Implementation of GPIO-based key driver
  *
  ******************************************************************************
  */

#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "my_list.h"

/* Exported define -----------------------------------------------------------*/
/* Key Configuration Options */
#ifndef KEY_SUPPORT_REPEAT
    #define KEY_SUPPORT_REPEAT      1    /* Enable/Disable repeat click function */
#endif // KEY_SUPPORT_REPEAT

#ifndef KEY_SCAN_PERIOD_MS
    #define KEY_SCAN_PERIOD_MS      10    /* Key scan period (ms) */
#endif // KEY_SCAN_PERIOD_MS

#ifndef KEY_DEBOUNCE_TIME
    #define KEY_DEBOUNCE_TIME       2     /* Default debounce time (in KEY_SCAN_PERIOD_MS) */
#endif // KEY_DEBOUNCE_TIME

#ifndef KEY_LONG_TIME
    #define KEY_LONG_TIME           100   /* Default long press time (in KEY_SCAN_PERIOD_MS) */
#endif // KEY_LONG_TIME

#ifndef KEY_REPEAT_TIME
    #define KEY_REPEAT_TIME         20    /* Default repeat click interval (in KEY_SCAN_PERIOD_MS) */
#endif // KEY_REPEAT_TIME

#ifndef KEY_HOLD_TIME
    #define KEY_HOLD_TIME           10    /* Default long press hold trigger period (in KEY_SCAN_PERIOD_MS) */
#endif // KEY_HOLD_TIME

/* Key Event Definitions */
typedef enum key_event {
    KEY_EVENT_DOWN = 0,           /* Key press down */
    KEY_EVENT_UP,                 /* Key release up */
    KEY_EVENT_LONG_START,         /* Long press start */
    KEY_EVENT_LONG_HOLD,          /* Long press holding */
    KEY_EVENT_LONG_FREE,          /* Long press release */
#if KEY_SUPPORT_REPEAT
    KEY_EVENT_REPEAT,             /* Key repeat click */
#endif
    KEY_EVENT_NUM,                /* Number of key events */
    KEY_EVENT_NONE                /* No event */
}key_event_t;

/* Key State Definitions */
typedef enum key_state {
    KEY_STATE_NONE = 0,          /* No action */
    KEY_STATE_DOWN,              /* Pressed */
    KEY_STATE_UP,                /* Released */
    KEY_STATE_LONG,              /* Long pressed */
}key_state_t;

/* 定义事件消息队列中的消息结构 */ 
typedef struct {
    uint8_t id;                 // 哪个按键
    key_event_t event;          // 发生了什么事件
} key_event_msg_t;

/* Exported typedef ----------------------------------------------------------*/
typedef void (*key_callback)(void *);   /* Key callback function type */

struct key_dev; /* 前向声明 */

/**
 * @brief Key Device Structure
 */
typedef struct key_dev {
    uint8_t id;                     /**< Unique ID for the key */
    key_state_t state;              /**< Current state in the finite state machine */
    uint8_t last_level;             /**< Last stable physical level (0 or 1) after debounce */
    list_t node;                    /**< Linked list node for the active key list */

    /* Timing parameters in multiples of KEY_SCAN_PERIOD_MS */
    uint16_t long_time;             /**< Time until a press becomes a long press */
    uint8_t hold_time;              /**< Period for generating hold events */
    uint8_t debounce_time;          /**< Time required for a level change to be considered stable */
    uint8_t repeat_time;            /**< Maximum time between presses for a repeat click */
    
    /* Internal state counters */
    uint16_t ticks;                 /**< Timer tick counter for the current state */
    uint8_t repeat_count;           /**< Counter for repeat clicks */

    void *hw_context;               /**< Pointer to hardware-specific context (e.g., GPIO pin info) */
    uint32_t (*read_state)(struct key_dev *self); /**< Function pointer to read the key's raw physical state */
} key_t;

/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int  key_device_register(key_t *key);
int  key_init(void);
int  key_get_event(key_event_msg_t *msg);
void key_start(key_t *key);
void key_stop(key_t *key);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */
