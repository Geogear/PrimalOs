#ifndef SCANCODES_H
#define SCANCODES_H

#define ARROW_MSB_8 224
#define ARROW_LEFT 75
#define ARROW_RIGHT 77
#define CAPS_LOCK 58
#define LETTER_DIF 32

typedef enum{
    ASCII_NULL = 0,
    SOH = 1,
    STX = 2,
    ETX = 3,
    EOT = 4,
    ENQ = 5,
    ACK = 6,
    BEL = 7,
    BACKSPACE = 8,
    H_TAB = 9,
    LF = 10,
    V_TAB = 11,
    FF = 12,
    CR = 13,
    SO = 14,
    SI = 15,
    DLE = 16,
    DC1 = 17,
    DC2 = 18,
    DC3 = 19,
    DC4 = 20,
    NAK = 21,
    SYN = 22,
    ETB = 23,
    CAN = 24,
    EM = 25,
    SUB = 26,
    ESC = 27,
    FS = 28,
    GS = 29,
    RS = 30,
    US = 31,
    SPC = 32,
    EXCL = 33, //!
    D_QUOT = 34,
    HASH = 35,
    DOLLAR = 36,
    PERCENT = 37,
    AMP = 38, //&
    S_QUOT = 39,
    L_PAR = 40,
    R_PAR = 41,
    ASTERISK = 42,
    PLUS = 43,
    COMMA = 44,
    MINUS = 45,
    DOT = 46,
    SLASH = 47,
    ZERO = 48,
    COL = 58,
    SEM_COL = 59,
    LESS = 60,
    EQUAL = 61,
    GREATER = 62,
    QUESTION = 63,
    AT = 64,
    LET_A = 65,
    L_BRACKET = 91,
    BACK_SLASH = 92,
    R_BRACKET = 93,
    CARET = 94, //^
    UNDERSCORE = 95,
    GRAVE = 96, //`
    LET_a = 97,
    L_CURLY = 123,
    BAR = 124, // |
    R_CURLY = 125,
    TILDE = 126,
    DEL = 127,
    ASCII_NAME_COUNT = 128
}ascii_t;

static const uint8_t scancodes_ascii[] = {
    ASCII_NULL, //0x00
    ASCII_NULL, //0x01
    ASCII_NULL, //0x02
    ASCII_NULL, //0x03
    LET_A, //0x04 // Keyboard a and A
    LET_A + 1, //KEY_B 0x05 // Keyboard b and B
    LET_A + 2, //KEY_C 0x06 // Keyboard c and C
    LET_A + 3, //KEY_D 0x07 // Keyboard d and D
    LET_A + 4, //KEY_E 0x08 // Keyboard e and E
    LET_A + 5, //KEY_F 0x09 // Keyboard f and F
    LET_A + 6, //KEY_G 0x0a // Keyboard g and G
    LET_A + 7, //KEY_H 0x0b // Keyboard h and H
    LET_A + 8, //KEY_I 0x0c // Keyboard i and I
    LET_A + 9, //KEY_J 0x0d // Keyboard j and J
    LET_A + 10, //KEY_K 0x0e // Keyboard k and K
    LET_A + 11, //KEY_L 0x0f // Keyboard l and L
    LET_A + 12, //KEY_M 0x10 // Keyboard m and M
    LET_A + 13, //KEY_N 0x11 // Keyboard n and N
    LET_A + 14, //KEY_O 0x12 // Keyboard o and O
    LET_A + 15, //KEY_P 0x13 // Keyboard p and P
    LET_A + 16, //KEY_Q 0x14 // Keyboard q and Q
    LET_A + 17, //KEY_R 0x15 // Keyboard r and R
    LET_A + 18, //KEY_S 0x16 // Keyboard s and S
    LET_A + 19, //KEY_T 0x17 // Keyboard t and T
    LET_A + 20, //KEY_U 0x18 // Keyboard u and U
    LET_A + 21, //KEY_V 0x19 // Keyboard v and V
    LET_A + 22, //KEY_W 0x1a // Keyboard w and W
    LET_A + 23, //KEY_X 0x1b // Keyboard x and X
    LET_A + 24, //KEY_Y 0x1c // Keyboard y and Y
    LET_A + 25, //KEY_Z 0x1d // Keyboard z and Z
    ZERO + 1, //KEY_1 0x1e // Keyboard 1 and !
    ZERO + 2, // KEY_2 0x1f // Keyboard 2 and @
    ZERO + 3, // KEY_3 0x20 // Keyboard 3 and #
    ZERO + 4, // KEY_4 0x21 // Keyboard 4 and $
    ZERO + 5, // KEY_5 0x22 // Keyboard 5 and %
    ZERO + 6, // KEY_6 0x23 // Keyboard 6 and ^
    ZERO + 7, // KEY_7 0x24 // Keyboard 7 and &
    ZERO + 8, // KEY_8 0x25 // Keyboard 8 and *
    ZERO + 9, // KEY_9 0x26 // Keyboard 9 and (
    ZERO, // KEY_0 0x27 // Keyboard 0 and )
    LF, //KEY_ENTER 0x28 // Keyboard Return (ENTER)
    ASCII_NULL, //KEY_ESC 0x29 // Keyboard ESCAPE
    ASCII_NULL, //KEY_BACKSPACE 0x2a // Keyboard DELETE (Backspace)
    ASCII_NULL, //KEY_TAB 0x2b // Keyboard Tab
    SPC, //KEY_SPACE 0x2c // Keyboard Spacebar
    MINUS, //KEY_MINUS 0x2d // Keyboard - and _
    EQUAL, //KEY_EQUAL 0x2e // Keyboard = and +
    L_BRACKET, //KEY_LEFTBRACE 0x2f // Keyboard [ and {
    R_BRACKET,  //KEY_RIGHTBRACE 0x30 // Keyboard ] and }
    BACK_SLASH, //KEY_BACKSLASH 0x31 // Keyboard \ and |
    TILDE, //KEY_HASHTILDE 0x32 // Keyboard Non-US # and ~
    SEM_COL, //KEY_SEMICOLON 0x33 // Keyboard ; and :
    D_QUOT, //KEY_APOSTROPHE 0x34 // Keyboard ' and "
    S_QUOT, //KEY_GRAVE 0x35 // Keyboard ` and ~
    COMMA, //KEY_COMMA 0x36 // Keyboard , and <
    DOT, //KEY_DOT 0x37 // Keyboard . and >
    SLASH, //KEY_SLASH 0x38 // Keyboard / and ?
};

static const uint8_t scan_ascii_size = 57;

uint8_t get_ascii_from_sc(uint8_t id){
    return id < scan_ascii_size ? scancodes_ascii[id] : ASCII_NULL;
}

uint8_t in_printable_range(uint8_t ascii){
    return (ascii >= SPC && ascii < DEL) | (ascii == LF);
}

#endif