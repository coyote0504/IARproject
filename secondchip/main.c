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

//常用短语库
unsigned char code_phrase[10][50] = {
"OK",
"NO",
"SOS",
"HELLO",
"THANK YOU",
"RECEIVED",
"RETRANSMISSION",
"GOODBYE",
"HELP ME!", 
"TERMININATE"
};

// 全局变量
unsigned char current_col = 0;
unsigned char current_row = 0;
#define uchar unsigned char
#define uint unsigned int

// 函数原型声明
void lcd_init(void);
void clear_lcd(void);
void lcd_write_command(unsigned char cmd);
void lcd_write_data(unsigned char data);
void lcd_set_cursor(int row, int col);
void lcd_write_string(const char *str);
void uart_init(void);
void uart_write_char(char c);
char uart_read_char(void);
void send_morse_code(const char *morse_code);
void receive_morse_code(const char *morse_code);
void display_codewave(void);
void text_to_morse(const char *text, char *morse_buffer);
const char *find_morse_code(char text);
char find_text_from_morse(const char *morse);
void morse_to_text(const char *morse, char *text);
void setup_keypad(void);
char scan_keypad(void);
void menu(void);
void sending_confirmed_code_default(int i);

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // 停用看门狗
    lcd_init();// 初始化LCD
    lcd_set_cursor(0, 0);// 将光标设置到第一行，第一列
    uart_init(); // 初始化UART
    // 设置主时钟为 8MHz
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;
    __bis_SR_register(GIE);
    menu();
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

void clear_lcd(void)
{
    lcd_write_command(0x01); // 清除显示，并设置DDRAM地址为00H
    current_col = 0;
    current_row = 0;
}

void lcd_write_command(unsigned char cmd)
{
    P1OUT &= ~LCD_RS; // RS = 0，写入指令
    LCD_DATA = cmd; // 写入指令
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(5000); // 延时
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
    __delay_cycles(5000); // 延时等待执行完成
}

void lcd_write_string(const char *str)
{
    unsigned char row = current_row;
    unsigned char col = current_col;

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

void lcd_set_cursor(int row, int col)
{
    int address = 0;

    switch (row)
    {
        case 0:
            address = 0x80;
            break;
        case 1:
            address = 0xC0;
            break;
        case 2:
            address = 0x94;
            break;
        case 3:
            address = 0xD4;
            break;
        default:
            break;
    }

    address += col;
    lcd_write_command(address);
    current_row = row;
    current_col = col;
}

void uart_init(void)
{
    P3SEL &= ~(BIT4 | BIT5); // P3.4, P3.5设置为IO
    P3DIR |= BIT4;
    P3DIR |= ~BIT5;
    P3OUT = ~BIT4;

    UCA0CTL1 |= UCSWRST; // 复位 USCI_A0
    UCA0CTL1 |= UCSSEL_2; // 选择 SMCLK 作为时钟源
    UCA0BR0 = 0x41; // 波特率设置，8MHz 9600bps
    UCA0BR1 = 0x3;
    UCA0MCTL = UCBRS_2; // 设置调制控制寄存器

    UCA0CTL1 &= ~UCSWRST; // 清除 USCI_A0 的复位位
    IE2 |= UCA0RXIE; // 启用接收中断
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

void lcd_pulse(void)
{
    P1OUT |= LCD_E;
    __delay_cycles(40);
    P1OUT &= ~LCD_E;
    __delay_cycles(40);
}

void setup_keypad() 
{
    KEYPAD_PHONE_DIR |= KEYPAD_PHONE_PIN;
    KEYPAD_PHONE_OUT &= ~KEYPAD_PHONE_PIN;
    KEYPAD_PHONE_DIR &= ~KEYPAD_PHONE_PIN_COL; // 设置 P5.5~P5.7 口为输入
}

char scan_keypad()
{
    int row, col;
    for (row = 0; row < ROWS; row++) {
        KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_ROW; // 所有行输出高电平
        KEYPAD_PHONE_OUT &= ~(KEYPAD_PHONE_PIN_ROW_S << row); // 输出低电平
        __delay_cycles(1000);
        for (col = 0; col < COLS; col++) {
            if (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)))// 检测到按键按下
            {
                //等待按键释放
                while (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)));
                return keys[row][col]; // 返回按键对应的字符
            }
        }
    }
    return 0; // 没有按键按下
}

void menu(void)
{
    int flag_main = 1;
    int i = 0;
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    char buffer_1[64];
    memset(buffer_1, 0, sizeof(buffer_1));

    // 摩尔斯电码暂存
    char morse_buffer[256] = {'\0'};

    flag_main:
    setup_keypad();
    lcd_set_cursor(0,0);
    lcd_write_string("Welcome To Morsecode");
    lcd_write_string("Sending Systems!!!  ");
    lcd_write_string("1.Sending     2.Quit");
    while(flag_main)
    {
        switch (scan_keypad())
        {
        case '1'://Sending
            sending_menu:
            clear_lcd();
            lcd_write_string("Choose To Send:");
            lcd_set_cursor(1,0);
            lcd_write_string("0 1 2 3 4 5 6 7 8 9");
            lcd_set_cursor(3,0);
            lcd_write_string("Press * To Return");
            while(1)
            {
                switch (scan_keypad())
                {
                case '0':
                    clear_lcd();
                    lcd_write_string("Messege_0 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[0]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_0:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[0], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '1':
                    clear_lcd();
                    lcd_write_string("Messege_1 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[1]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_1:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[1], morse_buffer);
                            send_morse_code(morse_buffer);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '2':
                    clear_lcd();
                    lcd_write_string("Messege_2 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[2]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_2:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[2], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '3':
                    clear_lcd();
                    lcd_write_string("Messege_3 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[3]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_3:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[3], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '4':
                    clear_lcd();
                    lcd_write_string("Messege_4 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[4]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_4:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[4], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '5':
                    clear_lcd();
                    lcd_write_string("Messege_5 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[5]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_5:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[5], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '6':
                    clear_lcd();
                    lcd_write_string("Messege_6 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[6]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_6:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[6], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '7':
                    clear_lcd();
                    lcd_write_string("Messege_7 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[7]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_7:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[7], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '8':
                    clear_lcd();
                    lcd_write_string("Messege_8 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[8]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_8:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[8], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '9':
                    clear_lcd();
                    lcd_write_string("Messege_9 Is:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[9]);
                    lcd_set_cursor(2,0);
                    lcd_write_string("Send?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Confirmed 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1'://Confirmed
                            clear_lcd();
                            lcd_write_string("Messege_9:");
                            lcd_set_cursor(1,0);
                            lcd_write_string("Sending...");
                            text_to_morse(code_phrase[9], morse_buffer);
                            send_morse_code(morse_buffer);
                            for(i=0;i<256;i++)
                            {
                                morse_buffer[i] = '\0';
                            }
                            lcd_set_cursor(2,0);
                            lcd_write_string("Code Sent.");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning......");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto sending_menu;
                        case '2'://Return
                            goto sending_menu;
                        }
                    }
                case '*':
                    clear_lcd();
                    goto sending_menu;
                }
            }
        case '2':
            flag_main = 0;
            break;
        }
    }
}