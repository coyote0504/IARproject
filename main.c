#include "MSP430.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// 手机键盘行列宽高
#define ROWS 4
#define COLS 3

// 手机键盘行列引脚连接到P5.1~P5.4, P5.5~P5.7口
#define KEYPAD_PHONE_IN        P5IN
#define KEYPAD_PHONE_OUT       P5OUT
#define KEYPAD_PHONE_DIR       P5DIR
#define KEYPAD_PHONE_REN       P5REN
#define KEYPAD_PHONE_PIN_ROW_S BIT1
#define KEYPAD_PHONE_PIN_COL_S BIT5
#define KEYPAD_PHONE_PIN_ROW   (BIT1 | BIT2 | BIT3 | BIT4)
#define KEYPAD_PHONE_PIN_COL   (BIT5 | BIT6 | BIT7)
#define KEYPAD_PHONE_PIN       (BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7)

const char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// 定义引脚
#define LCD_RS BIT0
#define LCD_E  BIT1
#define LCD_DATA P2OUT
#define LCD_RS1 BIT3
#define LCD_E1 BIT2
#define LCD_DATA1 P4OUT

// 最大字符数和行数
#define MAX_COLS 20
#define MAX_ROWS 4

//摩尔斯电码电平时间
#define DOT_DURATION 1
#define DASH_DURATION 3
#define SHORT_GAP_DURATION 1
#define MEDIUM_GAP_DURATION 3
#define LONG_GAP_DURATION 7
//摩尔斯电码结构体
//为了方便寄存器直接进行理解
typedef struct {
    const char *morse;
    char text;
} MorseEntry;
const MorseEntry morse_lookup_table[] = {
    {".-",   'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..",  'D'}, {".",    'E'}, {"..-.", 'F'},
    {"--.",  'G'}, {"....", 'H'}, {"..",   'I'}, {".---", 'J'}, {"-.-",  'K'}, {".-..", 'L'},
    {"--",   'M'}, {"-.",   'N'}, {"---",  'O'}, {".--.", 'P'}, {"--.-", 'Q'}, {".-.",  'R'},
    {"...",  'S'}, {"-",    'T'}, {"..-",  'U'}, {"...-", 'V'}, {".--",  'W'}, {"-..-", 'X'},
    {"-.--", 'Y'}, {"--..", 'Z'}, {".-",   'a'}, {"-...", 'b'}, {"-.-.", 'c'}, {"-..",  'd'},
    {".",    'e'}, {"..-.", 'f'}, {"--.",  'g'}, {"....", 'h'}, {"..",   'i'}, {".---", 'j'},
    {"-.-",  'k'}, {".-..", 'l'}, {"--",   'm'}, {"-.",   'n'}, {"---",  'o'}, {".--.", 'p'},
    {"--.-", 'q'}, {".-.",  'r'}, {"...",  's'}, {"-",    't'}, {"..-",  'u'}, {"...-", 'v'},
    {".--",  'w'}, {"-..-", 'x'}, {"-.--", 'y'}, {"--..", 'z'},
    {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
    {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}, {".-.-.-", '.'}, {"--..--", ','},
    {"..--..", '?'}, {".----.", '\''}, {"-.-.--", '!'}, {"-..-.", '/'}, {"-.--.", '('}, {"-.--.-", ')'},
    {".-...", '&'}, {"---...", ':'}, {"-.-.-.", ';'}, {"-...-", '='}, {".-.-.", '+'}, {"-....-", '-'},
    {"..--.-", '_'}, {".-..-.", '\"'}, {"...-..-", '$'}, {".--.-.", '@'}
};


// 全局变量
unsigned char current_col = 0;
unsigned char current_row = 0;

// 函数原型声明
void lcd_init(void);
void lcd_write_command(unsigned char cmd);
void lcd_write_data(unsigned char data);
void lcd_set_cursor(unsigned char row, unsigned char col);
void lcd_write_string(const char *str);
void uart_init(void);
void uart_write_char(char c);
char uart_read_char(void);
void send_morse_code(const char *morse_code);
void text_to_morse(const char *text, char *morse_buffer);
const char *find_morse_code(char text);
char find_text_from_morse(const char *morse);
void morse_to_text(const char *morse, char *text);
void setup_keypad();
char scan_keypad();


int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // 停用看门狗
    int i = 10;
    lcd_init();// 初始化LCD
    lcd_set_cursor(0, 0);// 将光标设置到第一行，第一列
    uart_init(); // 初始化UART
    // 设置主时钟为 8MHz
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;
    __bis_SR_register(GIE);
    char buffer[64];
    // int index = 0;
    memset(buffer, 0, sizeof(buffer));
    char buffer_1[64];
    memset(buffer_1, 0, sizeof(buffer_1));
    strcpy(buffer, "CHINA I\nLOVEYOU!");
    setup_keypad();
    lcd_write_string("Welcome to morsecodereceiving systems!!!1.revceive and reply2.relay 3.save 4.off");
    while(1)
    {
        switch (scan_keypad())
        {
        case '1':
            while(1)
            {
                lcd_init();
                __delay_cycles(1000);
                lcd_write_string("settings");
                __delay_cycles(1000);
                lcd_write_command(0x01); // 清除显示，并设置DDRAM地址为00H
                __delay_cycles(1000);
            }
            break;
        case '2':
            while(1)
            {
                lcd_init();
                __delay_cycles(1000);
                lcd_write_string("settings");
                __delay_cycles(1000);
                lcd_write_command(0x01); // 清除显示，并设置DDRAM地址为00H
                __delay_cycles(1000);
            }
            break;
            break;
        default:
            break;
        }
    }
    
    scan_keypad();
}

void lcd_init(void)
{
    P1DIR |= BIT0 | BIT1; // 设置P1.0(RS)和P1.1(E)为输出
    P2DIR |= 0xFF; // 设置P2.0到P2.7为输出

    // LCD初始化
    __delay_cycles(50000); // 延时等待LCD上电
    lcd_write_command(0x38); // 8位数据接口，2行显示，5x8点阵
    __delay_cycles(1); // 延时
    lcd_write_command(0x06); // 设置输入方式：增量，不移屏
    __delay_cycles(1); // 延时
    lcd_write_command(0x0C); // 显示开，无光标，无闪烁
    __delay_cycles(1); // 延时
    lcd_write_command(0x01); // 清除显示，并设置DDRAM地址为00H
    __delay_cycles(2000); // 延时等待清屏完成
}

void lcd_write_command(unsigned char cmd)
{
    P1OUT &= ~LCD_RS; // RS = 0，写入指令
    LCD_DATA = cmd; // 写入指令
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(1); // 延时
    P1OUT &= ~LCD_E; // E = 0
    __delay_cycles(5000); // 延时等待执行完成
}

void lcd_write_data(unsigned char data)
{
    P1OUT |= LCD_RS; // RS = 1，写入数据
    LCD_DATA = data; // 写入数据
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(1); // 延时
    P1OUT &= ~LCD_E; // E = 0
    __delay_cycles(500); // 延时等待执行完成

    // 更新光标位置
    current_col++;
    if (current_col >= MAX_COLS)
    {
        current_col = 0;
        current_row++;
        if (current_row >= MAX_ROWS)
        {
            current_row = 0;
        }
        lcd_set_cursor(current_row, current_col);
    }
}

void lcd_write_string(const char *str)
{
    static unsigned char row = 0;
    static unsigned char col = 0;

    while (*str)
    {
        if (col == 20) // 当达到一行的20个字符时换行
        {
            col = 0;
            row++;

            if (row == 4) // 当行数超过4行时，回到第一行
            {
                row = 0;
            }
        }

        lcd_set_cursor(row, col);
        lcd_write_data(*str);

        col++;
        str++;
    }
}

void lcd_set_cursor(unsigned char row, unsigned char col)
{
    unsigned char address;

    switch (row)
    {
        case 0:
            address = 0x80 + col;
            break;
        case 1:
            address = 0xC0 + col;
            break;
        case 2:
            address = 0x94 + col;
            break;
        case 3:
            address = 0xD4 + col;
            break;
        default:
            address = 0x80 + col;
            break;
    }

    lcd_write_command(address);
}

void uart_init(void)
{
    P3SEL |= BIT4 | BIT5; // P3.4 = UCA0TXD, P3.5 = UCA0RXD
    UCA0CTL1 |= UCSWRST; // 复位 USCI_A0
    UCA0CTL1 |= UCSSEL_2; // 选择 SMCLK 作为时钟源
    UCA0BR0 = 0x41; // 波特率设置，8MHz 9600bps
    UCA0BR1 = 0x3;
    UCA0MCTL = UCBRS_2; // 设置调制控制寄存器

    UCA0CTL1 &= ~UCSWRST; // 清除 USCI_A0 的复位位
    IE2 |= UCA0RXIE; // 启用接收中断
}

void uart_write_char(char c)
{
    while (!(IFG2 & UCA0TXIFG)); // 等待上一个字符发送完成
    UCA0TXBUF = c; // 将字符写入发送缓冲区
}

char uart_read_char(void)
{
    while (!(IFG2 & UCA0RXIFG)); // 等待接收到字符
    return UCA0RXBUF; // 从接收缓冲区读取字符
}

void send_morse_code(const char *morse_code)
{
    P3DIR = BIT4;
    while (*morse_code)
    {
        switch (*morse_code)
        {
            case '.':
                P3OUT |= BIT4;  // 高电平
                __delay_cycles(1000000 * DOT_DURATION);  // 点持续时间
                P3OUT &= ~BIT4; // 低电平
                break;
            case '-':
                P3OUT |= BIT4;   // 高电平
                __delay_cycles(1000000 * DASH_DURATION); // 划持续时间
                P3OUT &= ~BIT4;  // 低电平
                break;
            case '0':
                __delay_cycles(1000000 * SHORT_GAP_DURATION); // 间隔
                break;
        }

        morse_code++;

        if (*morse_code)
        {
            __delay_cycles(1000000 * SHORT_GAP_DURATION); // 点划之间的短间隔
        }
    }
}

void text_to_morse(const char *text, char *morse_buffer)
{
    while (*text)
    {
        if (*text == ' ')
        {
            strcat(morse_buffer, "00"); // 2+1个时间单位的间隔（MEDIUM_GAP_DURATION）表示单词间隔
            text++;
            continue;
        }
        else if (*text == '\n')
        {
            strcat(morse_buffer, "000000"); // 6+1个时间单位的间隔（LONG_GAP_DURATION）表示段落间隔
            text++;
            continue;
        }

        const char *morse_code = find_morse_code(toupper(*text));

        if (morse_code)
        {
            strcat(morse_buffer, morse_code);
            strcat(morse_buffer, "0"); // 1个时间单位的间隔（SHORT_GAP_DURATION）表示字母内部的点划间隔
        }

        text++;
    }

    size_t morse_buffer_length = strlen(morse_buffer);
    if (morse_buffer_length > 0)
    {
        morse_buffer[morse_buffer_length - 1] = '\0'; // 移除最后一个不必要的间隔
    }
}

const char *find_morse_code(char text)
{
    for (int i = 0; i < sizeof(morse_lookup_table) / sizeof(MorseEntry); i++)
    {
        if (morse_lookup_table[i].text == text)
        {
            return morse_lookup_table[i].morse;
        }
    }
    return 0;
}

char find_text_from_morse(const char *morse)
{
    for (int i = 0; i < sizeof(morse_lookup_table) / sizeof(MorseEntry); i++)
    {
        if (strcmp(morse_lookup_table[i].morse, morse) == 0)
        {
            return morse_lookup_table[i].text;
        }
    }
    return 0;
}

void morse_to_text(const char *morse, char *text)
{
    unsigned char morse_buffer[8]; // 用于存储单个摩尔斯字符的缓冲区
    size_t morse_buffer_pos = 0;
    size_t zero_count = 0;

    while (*morse)
    {
        if (*morse == '0')
        {
            zero_count++;
            if (zero_count >= 7) // 遇到 0000000，转换为回车
            {
                *text++ = '\n';
                zero_count = 0;
            }
            else if (zero_count == 3) // 遇到 000，添加空格
            {
                *text++ = ' ';
            }
        }
        else
        {
            zero_count = 0;
            // 遇到点或划，添加到缓冲区
            morse_buffer[morse_buffer_pos++] = *morse;

            // 如果下一个字符是0或字符串末尾，则解码摩尔斯字符
            if (*(morse + 1) == '0' || *(morse + 1) == '\0')
            {
                morse_buffer[morse_buffer_pos] = '\0';
                char decoded_char = find_text_from_morse(morse_buffer);
                if (decoded_char != '\0')
                {
                    *text++ = decoded_char;
                }
                morse_buffer_pos = 0;
            }
        }
        morse++;
    }
    *text = '\0'; // 确保字符串以 null 字符结尾
}

void lcd_pulse(void)
{
    P1OUT |= LCD_E;
    __delay_cycles(40);
    P1OUT &= ~LCD_E;
    __delay_cycles(40);
}

void setup_keypad() 
{
    KEYPAD_PHONE_DIR &= ~KEYPAD_PHONE_PIN_COL; // 设置 P5.5~P5.7 口为输入
    KEYPAD_PHONE_DIR |= KEYPAD_PHONE_PIN_ROW; // 设置 P5.1~P5.4 口为输出

    // KEYPAD_PHONE_REN |= KEYPAD_PHONE_PIN_COL; // 使能 P5.5~P5.7 口上下拉电阻
    // KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_COL; // 设置 P5.5~P5.7 口上拉
}

char scan_keypad()
{
    int row, col;
    for (row = 0; row < ROWS; row++) {
        KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_ROW; // 所有行输出高电平
        KEYPAD_PHONE_OUT &= ~(KEYPAD_PHONE_PIN_ROW_S << row); // 输出低电平
        __delay_cycles(800000);
        for (col = 0; col < COLS; col++) {
            if (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)))// 检测到按键按下
            {
                return keys[row][col]; // 返回按键对应的字符
            }
        }
    }
    return 0; // 没有按键按下
}