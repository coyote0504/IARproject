#include "io430.h"
#include <stddef.h>

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
void morse_to_text(const char *morse, char *text);

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
        strcpy(buffer, "CHINA \n I \n LOVE YOU!");
        // ��UART�����ַ���
        // while (1)
        // {
        //     char c = uart_read_char();
        //     lcd_write_data(c);
        //     if (c == '\n' || c == '\r')
        //     {
        //         break;
        //     }
        //     buffer[index++] = c;
        // }
        lcd_write_string(buffer);
        // ���ַ���ת��ΪĦ��˹����
        char morse_buffer[256];
        text_to_morse(buffer, morse_buffer);
        lcd_write_string(morse_buffer);
        // ����Ħ��˹����
        send_morse_code(morse_buffer);
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
    P3SEL |= BIT4 + BIT5; // ѡ��P3.4��P3.5��ΪUARTģ�鹦��
    U0CTL |= CHAR + SWRST; // 8λ����
    U0TCTL |= SSEL1; // ʹ��SMCLKʱ��Դ
    U0BR0 = 104; // ���ò�����Ϊ9600 (104��Ӧ9600������, SMCLK = 1MHz)
    U0BR1 = 0; // ���ò�����Ϊ9600
    U0MCTL = 0x49; // ���õ��������ƼĴ���
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

void send_morse_code(const char *morse_code)
{
    P3DIR |= BIT1; // ����P3.1Ϊ���ģʽ

    while (*morse_code)
    {
        if (*morse_code == '.')
        {
            P3OUT |= BIT1; // �ߵ�ƽ
            __delay_cycles(DOT_DURATION * 1000000);
            P3OUT &= ~BIT1; // �͵�ƽ
        }
        else if (*morse_code == '-')
        {
            P3OUT |= BIT1; // �ߵ�ƽ
            __delay_cycles(DASH_DURATION * 1000000);
            P3OUT &= ~BIT1; // �͵�ƽ
        }

        morse_code++;

        if (*morse_code == ' ')
        {
            __delay_cycles(MEDIUM_GAP_DURATION * 1000000);
            morse_code++;
        }
        else
        {
            __delay_cycles(SHORT_GAP_DURATION * 1000000);
        }
    }

    __delay_cycles(LONG_GAP_DURATION * 1000000);
}

void text_to_morse(const char *text, char *morse_buffer)
{
    while (*text)
    {
        if (*text == ' ')
        {
            strcat(morse_buffer, "000"); // 3��ʱ�䵥λ�ļ����MEDIUM_GAP_DURATION����ʾ���ʼ��
            text++;
            continue;
        }
        else if (*text == '\n')
        {
            strcat(morse_buffer, "0000000"); // 7��ʱ�䵥λ�ļ����LONG_GAP_DURATION����ʾ������
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
