/**
  ******************************************************************************
  * @file        : key.h
  * @author      : xxx
  * @version     : V1.0
  * @date        : 2024-xx-xx
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
#include "sys_config.h"

/* Exported define -----------------------------------------------------------*/
/* Key Function Configuration Options */
#ifndef KEY_SUPPORT_REPEAT
#define KEY_SUPPORT_REPEAT         1    /* Enable/Disable repeat click function */
#endif

#ifndef KEY_SCAN_PERIOD_MS
#define KEY_SCAN_PERIOD_MS        10    /* Key scan period (ms) */
#endif

/* Default Key Parameters Configuration */
#define KEY_DEFAULT_DEBOUNCE      2     /* Default debounce time (2 scan cycles) */
#define KEY_DEFAULT_LONG_TIME    100    /* Default long press time (100 scan cycles) */
#define KEY_DEFAULT_REPEAT_TIME   20    /* Default repeat click interval (20 scan cycles) */
#define KEY_DEFAULT_HOLD_TIME    10     /* Default long press hold trigger period (10 scan cycles) */

/* Key Event Definitions */
enum key_event {
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
};

/* Key State Definitions */
enum key_state {
    KEY_STATE_NONE = 0,          /* No action */
    KEY_STATE_DOWN,              /* Pressed */
    KEY_STATE_UP,                /* Released */
    KEY_STATE_LONG,              /* Long pressed */
};

/* Exported typedef ----------------------------------------------------------*/
typedef void (*key_callback)(void *);   /* Key callback function type */

/**
 * @brief Key device structure
 */
typedef struct key_dev {
    const char *key_name;          /* Key name */
    const char *pin_name;          /* Key pin name */
    size_t pin;                    /* Key GPIO pin */
    list_t node;                  /* Key device list node */
    uint8_t active_level : 1;      /* Active level (range: 0-1) */
    uint8_t state : 2;             /* Current state (range: 0-3) */
    uint8_t last_level : 1;        /* Last level state (range: 0-1) */
    uint8_t debounce_time : 4;     /* Debounce time in scan cycles (range: 0-15) */
    uint8_t repeat_time : 5;       /* Repeat click interval in scan cycles (range: 0-31) */
    uint8_t repeat_count : 4;      /* Repeat click counter (range: 0-15) */
    uint16_t ticks;                /* Timer ticks */
    uint16_t long_time;            /* Long press time in scan cycles */
    uint8_t hold_time;             /* Long press hold trigger period in scan cycles */
    key_callback cb_func[KEY_EVENT_NUM];  /* Event callback function array */
} key_t;

/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int key_init(void);
key_t *key_find(const char *name);
void key_attach(key_t *key, enum key_event event, key_callback cb_func);
void key_detach(key_t *key, enum key_event event);
void key_start(key_t *key);
void key_stop(key_t *key);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */
