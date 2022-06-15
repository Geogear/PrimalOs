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
    R_CURRLY = 125,
    TILDE = 126,
    DEL = 127,
    ASCII_NAME_COUNT = 128
}ascii_t;

static const uint8_t scancodes_ascii[] =
{
    ASCII_NULL,
    ESC,
    ZERO + 1,
    ZERO + 2,
    ZERO + 3,
    ZERO + 4,
    ZERO + 5,
    ZERO + 6,
    ZERO + 7,
    ZERO + 8,
    ZERO + 9,
    ZERO,
    MINUS,
    EQUAL,
    BACKSPACE, // 0E
    H_TAB,
    LET_A + 16, //Q
    LET_A + 22, //W
    LET_A + 4, //E
    LET_A + 17, //R
    LET_A + 19, //T
    LET_A + 24, //Y
    LET_A + 20, //U
    LET_A + 8, //I
    LET_A + 14, //O
    LET_A + 15, //P
    L_BRACKET,
    R_BRACKET,
    LF,
    ASCII_NAME_COUNT, // LEFT-CTRL 1D
    LET_A,
    LET_A + 18, //S
    LET_A + 3, //D
    LET_A + 5, //F
    LET_A + 6, //G
    LET_A + 7, //H
    LET_A + 9, //J
    LET_A + 10, //K
    LET_A + 11, //L
    SEM_COL,
    S_QUOT,
    GRAVE,
    ASCII_NAME_COUNT, //LETF-SHIFT
    BACK_SLASH, //2B
    LET_A + 25, //Z
    LET_A + 23, //X
    LET_A + 2, //C
    LET_A + 21, //V
    LET_A + 1, //B
    LET_A + 13, //N
    LET_A + 12, //M
    COMMA,
    DOT,
    SLASH,
    ASCII_NAME_COUNT, //RIGHT-SHIFT 36
    ASTERISK,
    ASCII_NAME_COUNT, //LEFT-ALT
    SPC,
    ASCII_NAME_COUNT //CAPSLOCK
};

static const uint8_t scan_ascii_size = 59;

uint8_t get_ascii_from_sc(uint8_t id){
    return id < scan_ascii_size ? scancodes_ascii[id] : ASCII_NAME_COUNT;
}

uint8_t in_printable_range(uint8_t ascii){
    return (ascii >= SPC && ascii < DEL) | ascii == LF;
}

#endif