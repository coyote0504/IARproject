#include "io430.h"

// ��������
#define LCD_RS BIT0
#define LCD_E  BIT1
#define LCD_DATA P2OUT

// ����ַ���������
#define MAX_COLS 20
#define MAX_ROWS 4

//Ħ˹����ṹ��
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
void morse_to_text(const char *morse, char *text);

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // ͣ�ÿ��Ź�

    // ��ʼ��LCD
    lcd_init();

    // ��������õ���һ�У���һ��
    lcd_set_cursor(0, 0);

    // ��ʼ��UART
    uart_init();

    // ʹ��ȫ���ж�
    __enable_interrupt();

    // ����Ħ��˹���벢��ʾ����
    char morse_buffer[128];
    char text_buffer[128];
    int morse_index = 0;

    while (1)
    {
        char c = uart_read_char(); // ��ȡһ���ַ�
        if (c == '\n') // ����ǻ��з�����ʾĦ��˹�����������
        {
            morse_buffer[morse_index] = '\0'; // ����ַ���������־
            morse_to_text(morse_buffer, text_buffer); // ��Ħ��˹����ת��Ϊ����
            lcd_write_string(text_buffer); // ��LCD����ʾ����
            morse_index = 0; // ����Ħ��˹���뻺��������
        }
        else
        {
            morse_buffer[morse_index++] = c; // �����յ����ַ���ӵ�Ħ��˹���뻺����
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
    __delay_cycles(5000); // ��ʱ�ȴ�ִ�����

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
    while (*str)
    {
        lcd_write_data(*str++);
    }
}

void lcd_set_cursor(unsigned char row, unsigned char col)
{
    unsigned char addr;

    if (row == 0)
        addr = 0x80 + col; // ��һ�е�ַ��0x80��ʼ
    else
        addr = 0xC0 + col; // �ڶ��е�ַ��0xC0��ʼ

    lcd_write_command(addr); // ����DDRAM��ַ
}

void uart_init(void)
{
    P3SEL |= BIT0 + BIT1; // ѡ��P3.4��P3.5��ΪUARTģ�鹦��
    U0CTL |= CHAR; // 8λ����
    U0TCTL |= SSEL1; // ʹ��SMCLKʱ��Դ
    U0BR0 = 104; // ���ò�����Ϊ9600 (104��Ӧ9600������, SMCLK = 1MHz)
    U0BR1 = 0; // ���ò�����Ϊ9600
    UMCTL0  = 0x4A; // ���ò�����Ϊ9600
    U0CTL &= ~SWRST; // ��ʼ��UARTģ��
    IE1 |= URXIE0; // ����UART�����ж�
}

void uart_write_char(char c)
{
    while (!(IFG1 & UTXIFG0)); // �ȴ���һ���ַ��������
    U0TXBUF = c; // ���ַ�д�뷢�ͻ�����
}

char uart_read_char(void)
{
    while (!(IFG1 & URXIFG0)); // �ȴ����յ��ַ�
    return U0RXBUF; // �ӽ��ջ�������ȡ�ַ�
}

void morse_to_text(const char *morse, char *text)
{
    const char *morse_ptr = morse;
    int text_index = 0;
    int morse_len = strlen(morse);

    while (morse_ptr < morse + morse_len)
    {
        // ����ƥ���Ħ��˹����
        int found = 0;
        for (int i = 0; i < sizeof(morse_lookup_table) / sizeof(morse_lookup_table[0]); i++)
        {
            int code_len = strlen(morse_lookup_table[i].morse);
            if (strncmp(morse_ptr, morse_lookup_table[i].morse, code_len) == 0)
            {
                // ƥ��ɹ�������Ӧ���ַ�д��text
                text[text_index++] = morse_lookup_table[i].text;
                morse_ptr += code_len + 1; // ����ƥ���Ħ��˹����Ϳո�
                found = 1;
                break;
            }
        }

        // ���û���ҵ�ƥ���Ħ��˹���룬����һ���ַ�
        if (!found)
        {
            morse_ptr++;
        }
    }

    text[text_index] = '\0'; // ����ַ���������־
}
