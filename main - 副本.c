#include "io430.h"

// 定义引脚
#define LCD_RS BIT0
#define LCD_E  BIT1
#define LCD_DATA P2OUT

// 最大字符数和行数
#define MAX_COLS 20
#define MAX_ROWS 4

//摩斯电码结构体
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
void morse_to_text(const char *morse, char *text);

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // 停用看门狗

    // 初始化LCD
    lcd_init();

    // 将光标设置到第一行，第一列
    lcd_set_cursor(0, 0);

    // 初始化UART
    uart_init();

    // 使能全局中断
    __enable_interrupt();

    // 接收摩尔斯密码并显示明文
    char morse_buffer[128];
    char text_buffer[128];
    int morse_index = 0;

    while (1)
    {
        char c = uart_read_char(); // 读取一个字符
        if (c == '\n') // 如果是换行符，表示摩尔斯密码输入完成
        {
            morse_buffer[morse_index] = '\0'; // 添加字符串结束标志
            morse_to_text(morse_buffer, text_buffer); // 将摩尔斯密码转换为明文
            lcd_write_string(text_buffer); // 在LCD上显示明文
            morse_index = 0; // 重置摩尔斯密码缓冲区索引
        }
        else
        {
            morse_buffer[morse_index++] = c; // 将接收到的字符添加到摩尔斯密码缓冲区
        }
    }

    return 0;
}

void lcd_init(void)
{
    P1DIR |= LCD_RS | LCD_E; // 设置P1.0(RS)和P1.1(E)为输出
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
    __delay_cycles(5000); // 延时等待执行完成

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
    while (*str)
    {
        lcd_write_data(*str++);
    }
}

void lcd_set_cursor(unsigned char row, unsigned char col)
{
    unsigned char addr;

    if (row == 0)
        addr = 0x80 + col; // 第一行地址从0x80开始
    else
        addr = 0xC0 + col; // 第二行地址从0xC0开始

    lcd_write_command(addr); // 设置DDRAM地址
}

void uart_init(void)
{
    P3SEL |= BIT0 + BIT1; // 选择P3.4和P3.5作为UART模块功能
    U0CTL |= CHAR; // 8位数据
    U0TCTL |= SSEL1; // 使用SMCLK时钟源
    U0BR0 = 104; // 设置波特率为9600 (104对应9600波特率, SMCLK = 1MHz)
    U0BR1 = 0; // 设置波特率为9600
    UMCTL0  = 0x4A; // 设置波特率为9600
    U0CTL &= ~SWRST; // 初始化UART模块
    IE1 |= URXIE0; // 允许UART接收中断
}

void uart_write_char(char c)
{
    while (!(IFG1 & UTXIFG0)); // 等待上一个字符发送完成
    U0TXBUF = c; // 将字符写入发送缓冲区
}

char uart_read_char(void)
{
    while (!(IFG1 & URXIFG0)); // 等待接收到字符
    return U0RXBUF; // 从接收缓冲区读取字符
}

void morse_to_text(const char *morse, char *text)
{
    const char *morse_ptr = morse;
    int text_index = 0;
    int morse_len = strlen(morse);

    while (morse_ptr < morse + morse_len)
    {
        // 查找匹配的摩尔斯密码
        int found = 0;
        for (int i = 0; i < sizeof(morse_lookup_table) / sizeof(morse_lookup_table[0]); i++)
        {
            int code_len = strlen(morse_lookup_table[i].morse);
            if (strncmp(morse_ptr, morse_lookup_table[i].morse, code_len) == 0)
            {
                // 匹配成功，将对应的字符写入text
                text[text_index++] = morse_lookup_table[i].text;
                morse_ptr += code_len + 1; // 跳过匹配的摩尔斯密码和空格
                found = 1;
                break;
            }
        }

        // 如果没有找到匹配的摩尔斯密码，跳过一个字符
        if (!found)
        {
            morse_ptr++;
        }
    }

    text[text_index] = '\0'; // 添加字符串结束标志
}
