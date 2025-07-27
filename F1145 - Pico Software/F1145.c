#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include <pico/time.h>

#define PIN_RS  10
#define PIN_RW  9
#define PIN_E   8

#define DISPLAY_WIDTH   20

#define CUST_CMD_START  0x02

#define EVER    ;;

void dispSend(uint8_t b, bool rs);
void dispInit(void);
void dispHome(void);
void dispClear(void);
void dispWrite(const char *text);
void dispBrightness(uint8_t b);
void handleCommand(uint8_t cmd);

bool clearOnLn = true;     // false for only home on \n, true to clear
int currCol = 0;

int main()
{
    stdio_init_all();

    // PGIO0 to 10 are used for the display, set all to out
    gpio_set_function_masked(0x7FF, GPIO_FUNC_SIO);

    gpio_set_dir_out_masked(0x7FF);

    gpio_put(PIN_E, 0);

    dispInit();

    dispClear();
    // dispHome();
    // dispWrite("Hello!");
    // printf("Hello, world!\n");

    
    bool clearOnNext = false;
    bool cmdOnNext = false;
    for(EVER){
        char r = getchar();
        // if we get new line, set for next time to clear the display
        if(r == '\r' || r == '\n'){
            clearOnNext = true;
            continue;
        }
        if(r == CUST_CMD_START){     // if escape, we do our own command on next read
            cmdOnNext = true;
            continue;
        }

        if(cmdOnNext){
            handleCommand(r);
            cmdOnNext = false;
            continue;
        }
        
        if(clearOnNext){
            clearOnLn ? dispClear() : dispHome();
            clearOnNext = false;
            currCol = 0;
        }

        dispSend(r, 1);
        currCol++;
        if(currCol > DISPLAY_WIDTH){
            dispSend(0xC0, 0);   // move to next line
            currCol = 0;
        }
    }
}

void handleCommand(uint8_t cmd){
    bool ack = true;
    // set new line to clear display
    // 0x30 to 0x58 are to set the cursor position
    if(cmd >= 0x30 && cmd <= (0x30+(DISPLAY_WIDTH*2))){
        cmd -= 0x30;
        if(cmd >= DISPLAY_WIDTH){
            cmd -= DISPLAY_WIDTH;
            cmd |= 0x40;
        }
        currCol = cmd & 0x3F;
        dispSend(0x80 | cmd, 0);   // move to next line
    }
    //0x2C to 0x2F is for brightness
    else if(cmd >= 0x2C && cmd <= 0x2F){
        cmd -= 0x2C;
        dispBrightness(cmd);
    }
    else{
        switch(cmd){
            case 'c':
                clearOnLn = true;
                break;
            case 'h':
                clearOnLn = false;
                break;
            case 'x':
                dispClear();
                break;
            default:
                printf("nack\n");
                ack = false;
                break;
        }
    }

    if(ack){
        printf("ok\n");
    }
}

void dispClear(void){
    dispSend(0x01, 0);      // set address to 0
    currCol = 0;
    sleep_us(200);
}

void dispHome(void){
    dispSend(0x02, 0);      // set address to 0
    currCol = 0;
}

void dispWrite(const char *text){
    while(*text){
        dispSend(*text, 1);
        text++;
    }
}

void dispBrightness(uint8_t b){
    b &= 0x3;       // limit from 0 to 3

    dispSend(0x33, 0);      // set mode to 4 bits
    dispSend(b, 1);      // 100% brightness, must come after function set
}

void dispInit(void){
    dispSend(0x33, 0);      // set mode to 4 bits
    dispSend(0x00, 1);      // 100% brightness, must come after function set

    dispSend(0x01, 0);      // clear display
    sleep_ms(5);

    dispSend(0x02, 0);      // cursor home
    dispSend(0x0c, 0);      // turn display on
    dispSend(0x06, 0);      // mode entry, addr is incremented and cursor shift
}

// sends a byte to the display
// rs is whether to toggle the RS pin high or low, and b is the data
void dispSend(uint8_t b, bool rs){
    gpio_put(PIN_RS, rs);

    gpio_put_masked(0xFF, b);
    gpio_put(PIN_E, 1);
    sleep_us(1);
    gpio_put(PIN_E, 0);

    sleep_us(5);     // average execution time
}