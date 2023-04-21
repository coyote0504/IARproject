#include "MSP430.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// �ֻ��������п��
#define ROWS 4
#define COLS 3

// �ֻ����������������ӵ�P5.1~P5.4, P5.5~P5.7��
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

// ��������
#define LCD_RS BIT0
#define LCD_E  BIT1
#define LCD_DATA P2OUT
#define LCD_RS1 BIT3
#define LCD_E1 BIT2
#define LCD_DATA1 P4OUT

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
char find_text_from_morse(const char *morse);
void morse_to_text(const char *morse, char *text);
void setup_keypad();
char scan_keypad();


int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // ͣ�ÿ��Ź�
    int i = 10;
    lcd_init();// ��ʼ��LCD
    lcd_set_cursor(0, 0);// ��������õ���һ�У���һ��
    uart_init(); // ��ʼ��UART
    // ������ʱ��Ϊ 8MHz
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
                lcd_write_command(0x01); // �����ʾ��������DDRAM��ַΪ00H
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
                lcd_write_command(0x01); // �����ʾ��������DDRAM��ַΪ00H
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
    P1DIR |= BIT0 | BIT1; // ����P1.0(RS)��P1.1(E)Ϊ���
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
    P3SEL |= BIT4 | BIT5; // P3.4 = UCA0TXD, P3.5 = UCA0RXD
    UCA0CTL1 |= UCSWRST; // ��λ USCI_A0
    UCA0CTL1 |= UCSSEL_2; // ѡ�� SMCLK ��Ϊʱ��Դ
    UCA0BR0 = 0x41; // ���������ã�8MHz 9600bps
    UCA0BR1 = 0x3;
    UCA0MCTL = UCBRS_2; // ���õ��ƿ��ƼĴ���

    UCA0CTL1 &= ~UCSWRST; // ��� USCI_A0 �ĸ�λλ
    IE2 |= UCA0RXIE; // ���ý����ж�
}

void uart_write_char(char c)
{
    while (!(IFG2 & UCA0TXIFG)); // �ȴ���һ���ַ��������
    UCA0TXBUF = c; // ���ַ�д�뷢�ͻ�����
}

char uart_read_char(void)
{
    while (!(IFG2 & UCA0RXIFG)); // �ȴ����յ��ַ�
    return UCA0RXBUF; // �ӽ��ջ�������ȡ�ַ�
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
    unsigned char morse_buffer[8]; // ���ڴ洢����Ħ��˹�ַ��Ļ�����
    size_t morse_buffer_pos = 0;
    size_t zero_count = 0;

    while (*morse)
    {
        if (*morse == '0')
        {
            zero_count++;
            if (zero_count >= 7) // ���� 0000000��ת��Ϊ�س�
            {
                *text++ = '\n';
                zero_count = 0;
            }
            else if (zero_count == 3) // ���� 000����ӿո�
            {
                *text++ = ' ';
            }
        }
        else
        {
            zero_count = 0;
            // ������򻮣���ӵ�������
            morse_buffer[morse_buffer_pos++] = *morse;

            // �����һ���ַ���0���ַ���ĩβ�������Ħ��˹�ַ�
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
    *text = '\0'; // ȷ���ַ����� null �ַ���β
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
    KEYPAD_PHONE_DIR &= ~KEYPAD_PHONE_PIN_COL; // ���� P5.5~P5.7 ��Ϊ����
    KEYPAD_PHONE_DIR |= KEYPAD_PHONE_PIN_ROW; // ���� P5.1~P5.4 ��Ϊ���

    // KEYPAD_PHONE_REN |= KEYPAD_PHONE_PIN_COL; // ʹ�� P5.5~P5.7 ������������
    // KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_COL; // ���� P5.5~P5.7 ������
}

char scan_keypad()
{
    int row, col;
    for (row = 0; row < ROWS; row++) {
        KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_ROW; // ����������ߵ�ƽ
        KEYPAD_PHONE_OUT &= ~(KEYPAD_PHONE_PIN_ROW_S << row); // ����͵�ƽ
        __delay_cycles(800000);
        for (col = 0; col < COLS; col++) {
            if (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)))// ��⵽��������
            {
                return keys[row][col]; // ���ذ�����Ӧ���ַ�
            }
        }
    }
    return 0; // û�а�������
}