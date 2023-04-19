#include "io430.h"
#include <stddef.h>
#include <stdbool.h>

// ��������
#define LCD_RS BIT0
#define LCD_E  BIT1
#define LCD_DATA P2OUT

// ����ַ���������
#define MAX_COLS 20
#define MAX_ROWS 4

//Ħ��˹�����ƽʱ��
#define DOT_DURATION 1
#define DASH_DURATION 3
#define SHORT_GAP_DURATION 1
#define MEDIUM_GAP_DURATION 3
#define LONG_GAP_DURATION 7
//Ħ��˹����ṹ��
//Ϊ�˷���Ĵ���ֱ�ӽ������
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


// ȫ�ֱ���
unsigned char current_col = 0;
unsigned char current_row = 0;

// ����ԭ������
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

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // ͣ�ÿ��Ź�
    lcd_init();// ��ʼ��LCD
    lcd_set_cursor(0, 0);// ��������õ���һ�У���һ��
    uart_init(); // ��ʼ��UART
    __enable_interrupt(); // �����ж�

    while (1)
    {
        char buffer[64];
        int index = 0;
        memset(buffer, 0, sizeof(buffer));
        char buffer_1[64];
        memset(buffer_1, 0, sizeof(buffer_1));
        strcpy(buffer, "CHINA I\nLOVEYOU!");

        // ���ַ���ת��ΪĦ��˹����
        char morse_buffer[256];
        text_to_morse(buffer, morse_buffer);

        //����Ħ��˹����
        while(1)
        {
            lcd_write_string(buffer);
            lcd_write_string(morse_buffer);
            send_morse_code(morse_buffer);
        }
    }

    return 0;
}

void lcd_init(void)
{
    P1DIR |= LCD_RS | LCD_E; // ����P1.0(RS)��P1.1(E)Ϊ���
    P2DIR |= 0xFF; // ����P2.0��P2.7Ϊ���

    // LCD��ʼ��
    __delay_cycles(50000); // ��ʱ�ȴ�LCD�ϵ�
    lcd_write_command(0x38); // 8λ���ݽӿڣ�2����ʾ��5x8����
    __delay_cycles(1); // ��ʱ
    lcd_write_command(0x06); // �������뷽ʽ��������������
    __delay_cycles(1); // ��ʱ
    lcd_write_command(0x0C); // ��ʾ�����޹�꣬����˸
    __delay_cycles(1); // ��ʱ
    lcd_write_command(0x01); // �����ʾ��������DDRAM��ַΪ00H
    __delay_cycles(2000); // ��ʱ�ȴ��������
}

void lcd_write_command(unsigned char cmd)
{
    P1OUT &= ~LCD_RS; // RS = 0��д��ָ��
    LCD_DATA = cmd; // д��ָ��
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(1); // ��ʱ
    P1OUT &= ~LCD_E; // E = 0
    __delay_cycles(5000); // ��ʱ�ȴ�ִ�����
}

void lcd_write_data(unsigned char data)
{
    P1OUT |= LCD_RS; // RS = 1��д������
    LCD_DATA = data; // д������
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(1); // ��ʱ
    P1OUT &= ~LCD_E; // E = 0
    __delay_cycles(500); // ��ʱ�ȴ�ִ�����

    // ���¹��λ��
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
        if (col == 20) // ���ﵽһ�е�20���ַ�ʱ����
        {
            col = 0;
            row++;

            if (row == 4) // ����������4��ʱ���ص���һ��
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
    // ����UARTģ���Խ�������
    U0CTL |= SWRST;

    // ����UART�Ĺ���ģʽ
    U0CTL |= CHAR; // 8λ����

    // ���ò�����
    U0BR0 = 104;
    U0BR1 = 0;
    U0MCTL = 0x49;

    // ����P3.5ΪTXD��P3.4ΪGPIO����
    P3SEL |= BIT5;
    P3SEL &= ~BIT4;
    P3DIR |= BIT4;

    // ����UARTģ��
    U0CTL &= ~SWRST;

    // ���ý����ж�
    IE1 |= URXIE0;
}

void send_morse_code(const char *morse_code)
{
    P3DIR = BIT4;
    while (*morse_code)
    {
        switch (*morse_code)
        {
            case '.':
                P3OUT |= BIT4;  // �ߵ�ƽ
                __delay_cycles(1000000 * DOT_DURATION);  // �����ʱ��
                P3OUT &= ~BIT4; // �͵�ƽ
                break;
            case '-':
                P3OUT |= BIT4;   // �ߵ�ƽ
                __delay_cycles(1000000 * DASH_DURATION); // ������ʱ��
                P3OUT &= ~BIT4;  // �͵�ƽ
                break;
            case '0':
                __delay_cycles(1000000 * SHORT_GAP_DURATION); // ���
                break;
        }

        morse_code++;

        if (*morse_code)
        {
            __delay_cycles(1000000 * SHORT_GAP_DURATION); // �㻮֮��Ķ̼��
        }
    }
}

void text_to_morse(const char *text, char *morse_buffer)
{
    while (*text)
    {
        if (*text == ' ')
        {
            strcat(morse_buffer, "00"); // 2+1��ʱ�䵥λ�ļ����MEDIUM_GAP_DURATION����ʾ���ʼ��
            text++;
            continue;
        }
        else if (*text == '\n')
        {
            strcat(morse_buffer, "000000"); // 6+1��ʱ�䵥λ�ļ����LONG_GAP_DURATION����ʾ������
            text++;
            continue;
        }

        const char *morse_code = find_morse_code(toupper(*text));

        if (morse_code)
        {
            strcat(morse_buffer, morse_code);
            strcat(morse_buffer, "0"); // 1��ʱ�䵥λ�ļ����SHORT_GAP_DURATION����ʾ��ĸ�ڲ��ĵ㻮���
        }

        text++;
    }

    size_t morse_buffer_length = strlen(morse_buffer);
    if (morse_buffer_length > 0)
    {
        morse_buffer[morse_buffer_length - 1] = '\0'; // �Ƴ����һ������Ҫ�ļ��
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