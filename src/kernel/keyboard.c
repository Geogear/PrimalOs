#include <kernel/interrupts.h>
#include <kernel/usb.h>
#include <kernel/kerio.h>
#include <kernel/scancodes.h>
#include <kernel/framebuffer.h>
#include <kernel/gpu.h>

//#define INPUT_BUFFER_SIZE 512

static usb_mdio_control_t* keyboard_control;
static usb_mdio_data_t* keyboard_data;
static usb_vbus_t* keyboard_vbus;
//static unsigned char input_buffer[INPUT_BUFFER_SIZE] = {};
static uint16_t cursor_pos = 0;
static uint8_t tab_length = 4;
static uint8_t capslock_on = 1;

static void keyboard_irq_handler(void){
    uint16_t upper8 = keyboard_data->mdio_data;
    printf("KEYBOARD HANDLER: %u\n", upper8);
    uint8_t bottom8 = upper8;
    upper8 = upper8 >> 8;

    if(bottom8 == ARROW_MSB_8){
        if(bottom8 == ARROW_LEFT && cursor_pos > 0){
            // On the line start, go a line up.
            //TODO go-up if LF
            if(fbinfo.chars_x == 0 && fbinfo.chars_y > 0){
                fbinfo.chars_x = fbinfo.chars_width-1;
                --fbinfo.chars_y;
            }else if (fbinfo.chars_x > 0){
                --fbinfo.chars_x;
            }
            --cursor_pos;
        }else if(bottom8 == ARROW_RIGHT
        && cursor_pos < INPUT_BUFFER_SIZE-1){
            // On the line end, go a line down.
            if(fbinfo.chars_x == fbinfo.chars_width-1){
                fbinfo.chars_x = 0;
                ++fbinfo.chars_y;
            }else{
                ++fbinfo.chars_x;
            }
            ++cursor_pos;
        }
    }else{
        uint8_t scan_code = get_scancode(bottom8);
        if(in_printable_range(scan_code)){
            if(!capslock_on)
                scan_code += LETTER_DIF;
            // Buffer is full, circle it.
            if(cursor_pos == INPUT_BUFFER_SIZE)
                cursor_pos = 0;
            input_buffer[cursor_pos] = scan_code;
            gpu_putc(input_buffer[cursor_pos]);
            ++cursor_pos;
        }else{
            switch (scan_code)
            {
                case H_TAB:
                    // Put space, tab length times.
                    for(int i = 0; i < tab_length; ++i){
                        if(cursor_pos == INPUT_BUFFER_SIZE)
                            cursor_pos = 0;
                        input_buffer[cursor_pos] = SPC;
                        gpu_putc(input_buffer[cursor_pos]);
                        ++cursor_pos;
                    }
                    break;
                case DEL:
                    // TODO
                    break;
                case BACKSPACE:
                    // On the line end, go a line down.
                    //TODO go up if LF
                    if(fbinfo.chars_x == 0 && fbinfo.chars_y > 0){
                        fbinfo.chars_x = fbinfo.chars_width-1;
                        --fbinfo.chars_y;
                    }else if (fbinfo.chars_x > 0){
                        --fbinfo.chars_x;
                    }
                    if(cursor_pos > 0 && fbinfo.chars_y > 0){
                        gpu_putc(SPC);
                        --cursor_pos;
                    }
                    break;
                case CAPS_LOCK:
                    capslock_on = !capslock_on;
                    break; 
                default:
                    // Do nothing for the unsupported values.
                    break;
            }
        }
    }
}