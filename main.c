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

//���ö����
unsigned char code_phrase[10][50] = {
"OK", "NO", "SOS", "HELLO", "THANK YOU","RECEIVED",
"RETRANSMISSION", "GOODBYE", "HELP ME!", 
"TERMININATE"
};

// ȫ�ֱ���
unsigned char current_col = 0;
unsigned char current_row = 0;
#define uchar unsigned char
#define uint unsigned int

// ����ԭ������
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
void receive_morse_code(char *morse_buffer, char *text);
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
    WDTCTL = WDTPW + WDTHOLD; // ͣ�ÿ��Ź�
    lcd_init();// ��ʼ��LCD
    lcd_set_cursor(0, 0);// ��������õ���һ�У���һ��
    uart_init(); // ��ʼ��UART
    // ������ʱ��Ϊ 8MHz
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;
    __bis_SR_register(GIE);
    menu();
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

void clear_lcd(void)
{
    lcd_write_command(0x01); // �����ʾ��������DDRAM��ַΪ00H
    current_col = 0;
    current_row = 0;
}

void lcd_write_command(unsigned char cmd)
{
    P1OUT &= ~LCD_RS; // RS = 0��д��ָ��
    LCD_DATA = cmd; // д��ָ��
    P1OUT |= LCD_E; // E = 1
    __delay_cycles(5000); // ��ʱ
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
    __delay_cycles(5000); // ��ʱ�ȴ�ִ�����
}

void lcd_write_string(const char *str)
{
    unsigned char row = current_row;
    unsigned char col = current_col;

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
    P3SEL &= ~(BIT4 | BIT5); // P3.4, P3.5����ΪIO
    P3DIR |= BIT4;
    P3DIR |= ~BIT5;
    P3OUT = ~BIT4;

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

void receive_morse_code(char *morse_buffer, char *text)
{
    unsigned int signal_duration;
    unsigned int current_time;
    unsigned int gap_time;

    while (1)
    {
        // �ȴ� P3.5 ���
        while (!(P3IN & BIT5));

        // ��ʼ��¼ʱ��
        current_time = 0;

        // ��¼�ߵ�ƽʱ��
        while (P3IN & BIT5)
        {
            __delay_cycles(1000); // ��ʱ1ms
            current_time++;
        }

        if (current_time >= DOT_DURATION * 800 && current_time <= DOT_DURATION * 1200) // ��
        {
            strcat(morse_buffer, ".");
            lcd_write_string("-");
        }
        else if (current_time >= DASH_DURATION * 800 && current_time <= DASH_DURATION * 1200) // ��
        {
            strcat(morse_buffer, "-");
            lcd_write_string("---");
        }

        // ��¼�͵�ƽʱ��
        gap_time = 0;
        while (!(P3IN & BIT5))
        {
            __delay_cycles(1000); // ��ʱ1ms
            gap_time++;
            if(gap_time > LONG_GAP_DURATION * 1000 * 3)
            {
                clear_lcd();
                lcd_write_string("Received Text Is:");
                lcd_set_cursor(1,0);
                morse_to_text(morse_buffer, text);
                lcd_write_string(text);
                return;
            }
        }

        // ���ֲ�ͬ���ȵļ��
        if (gap_time >= SHORT_GAP_DURATION * 800 && gap_time <= SHORT_GAP_DURATION * 1200) // �̼��
        {
            lcd_write_string("_");
        }
        else if (gap_time >= MEDIUM_GAP_DURATION * 800 && gap_time <= MEDIUM_GAP_DURATION * 1200) // �еȼ��
        {
            strcat(morse_buffer, "0");
            lcd_write_string("___");
        }
        else if (gap_time >= LONG_GAP_DURATION * 800 && gap_time <= LONG_GAP_DURATION * 1200) // �����
        {
            strcat(morse_buffer, "000");
            lcd_write_string("_______");
        }
        else if (gap_time >= (2 * LONG_GAP_DURATION + 1) * 800 && gap_time <= (2 * LONG_GAP_DURATION + 1) * 1200) // �������
        {
            strcat(morse_buffer, "00000000");
            lcd_write_string("_______________");
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
    KEYPAD_PHONE_DIR |= KEYPAD_PHONE_PIN;
    KEYPAD_PHONE_OUT &= ~KEYPAD_PHONE_PIN;
    KEYPAD_PHONE_DIR &= ~KEYPAD_PHONE_PIN_COL; // ���� P5.5~P5.7 ��Ϊ����
}

char scan_keypad()
{
    int row, col;
    for (row = 0; row < ROWS; row++) {
        KEYPAD_PHONE_OUT |= KEYPAD_PHONE_PIN_ROW; // ����������ߵ�ƽ
        KEYPAD_PHONE_OUT &= ~(KEYPAD_PHONE_PIN_ROW_S << row); // ����͵�ƽ
        __delay_cycles(1000);
        for (col = 0; col < COLS; col++) {
            if (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)))// ��⵽��������
            {
                //�ȴ������ͷ�
                while (!(KEYPAD_PHONE_IN & (KEYPAD_PHONE_PIN_COL_S << col)));
                return keys[row][col]; // ���ذ�����Ӧ���ַ�
            }
        }
    }
    return 0; // û�а�������
}

void menu(void)
{
    int flag_main = 1;
    int i = 0;
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    char buffer_zero[64];
    memset(buffer_zero, 0, sizeof(buffer_zero));
    char buffer_1[64];
    memset(buffer_1, 0, sizeof(buffer_1));
    // Ħ��˹�����ݴ�
    char morse_buffer[256] = {'\0'};
    char morse_buffer_zero[256] = {'\0'};
    
    //����ɹ��û��Զ������Ϣ����
    unsigned char code_phrase_changeable[10][50] = {"","","","","","","","","",""};
    
    flag_main:
    setup_keypad();
    lcd_set_cursor(0,0);
    // strcpy(buffer, "CHINA ILOVEYOU!");
    // text_to_morse(buffer, morse_buffer);
    // morse_to_text(morse_buffer, buffer_1);
    // lcd_write_string(morse_buffer);
    // lcd_write_string(buffer_1);
    lcd_write_string("Welcome To Morsecode");
    lcd_write_string("Receiving Systems!!!");
    lcd_write_string("1.Receive     2.Save");
    lcd_write_string("3.Quit              ");
    while(flag_main)
    {
        switch (scan_keypad())
        {
        case '1'://Receiving
            clear_lcd();
            strcpy(morse_buffer, morse_buffer_zero);
            strcpy(buffer, buffer_zero);
            lcd_write_string("Receiving...");
            lcd_set_cursor(1,0);
            receive_morse_code(morse_buffer, buffer);//����Ħ��˹����
            receiving_menu:
            lcd_set_cursor(2,0);
            lcd_write_string("1.Reply   2.Relay   ");
            lcd_write_string("3.Save    4.Return  ");
            while(1)
            {   
                switch (scan_keypad())
                {
                case '1'://Reply
                    flag_reply:
                    clear_lcd();
                    lcd_write_string("Choose To Reply:");
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
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '1':
                            clear_lcd();
                            lcd_write_string("Messege_1 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[1]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '2':
                            clear_lcd();
                            lcd_write_string("Messege_2 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[2]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '3':
                            clear_lcd();
                            lcd_write_string("Messege_3 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[3]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '4':
                            clear_lcd();
                            lcd_write_string("Messege_4 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[4]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '5':
                            clear_lcd();
                            lcd_write_string("Messege_5 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[5]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '6':
                            clear_lcd();
                            lcd_write_string("Messege_6 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[6]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '7':
                            clear_lcd();
                            lcd_write_string("Messege_7 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[7]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '8':
                            clear_lcd();
                            lcd_write_string("Messege_8 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[8]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            }
                        case '9':
                            clear_lcd();
                            lcd_write_string("Messege_9 Is:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[9]);
                            lcd_set_cursor(2,0);
                            lcd_write_string("Reply?");
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
                                    goto receiving_menu;
                                case '2'://Return
                                    goto flag_reply;
                                }
                            } 
                        case '*':
                            clear_lcd();
                            goto receiving_menu;
                        }
                    }
                case '2'://Relay
                    clear_lcd();
                    lcd_write_string("Are You Sure To Relay The Messege?");
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Yes   2.No(Return)");
                    while(1)
                    {
                        switch(scan_keypad())
                        {
                        case '1'://replying
                            clear_lcd();
                            lcd_write_string("Relaying...");
                            send_morse_code(morse_buffer);
                            lcd_set_cursor(1,0);
                            lcd_write_string("Relayed");
                            lcd_set_cursor(3,0);
                            lcd_write_string("Returning...");
                            __delay_cycles(10000000);
                            clear_lcd();
                            goto receiving_menu;
                        case '2'://Return
                            clear_lcd();
                            goto receiving_menu;
                        }
                    }
                case '3'://Save
                    save_new_messege:
                    clear_lcd();
                    lcd_write_string("Choose Where To Save");
                    lcd_write_string("0 1 2 3 4 5 6 7 8 9 ");
                    lcd_set_cursor(3,0);
                    lcd_write_string("Press * To Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '0':
                            clear_lcd();
                            lcd_write_string("Messege_0:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[0]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_0:");
                                    strcpy(code_phrase_changeable[0], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[0]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '1':
                            clear_lcd();
                            lcd_write_string("Messege_1:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[1]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_1:");
                                    strcpy(code_phrase_changeable[1], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[1]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '2':
                            clear_lcd();
                            lcd_write_string("Messege_2:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[2]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_2:");
                                    strcpy(code_phrase_changeable[2], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[2]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '3':
                            clear_lcd();
                            lcd_write_string("Messege_3:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[3]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_3:");
                                    strcpy(code_phrase_changeable[3], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[3]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '4':
                            clear_lcd();
                            lcd_write_string("Messege_4:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[4]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_4:");
                                    strcpy(code_phrase_changeable[4], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[4]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '5':
                            clear_lcd();
                            lcd_write_string("Messege_5:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[5]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_5:");
                                    strcpy(code_phrase_changeable[5], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[5]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '6':
                            clear_lcd();
                            lcd_write_string("Messege_6:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[6]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_6:");
                                    strcpy(code_phrase_changeable[6], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[6]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '7':
                            clear_lcd();
                            lcd_write_string("Messege_7:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[7]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_7:");
                                    strcpy(code_phrase_changeable[7], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[7]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '8':
                            clear_lcd();
                            lcd_write_string("Messege_8:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[8]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_8:");
                                    strcpy(code_phrase_changeable[8], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[8]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '9':
                            clear_lcd();
                            lcd_write_string("Messege_9:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[9]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Save   2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    clear_lcd();
                                    lcd_write_string("Messege_9:");
                                    strcpy(code_phrase_changeable[9], buffer);//�洢���
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[9]);
                                    lcd_set_cursor(2,0);
                                    lcd_write_string("Messege Saved!");
                                    lcd_set_cursor(3,0);
                                    lcd_write_string("Returning...");
                                    __delay_cycles(10000000);
                                    clear_lcd();
                                    goto receiving_menu;
                                case '2':
                                    goto save_new_messege;
                                }
                            }
                        case '*':
                            clear_lcd();
                            goto receiving_menu;
                        }
                    }
                case '4'://Return
                    goto flag_main;
                }
            }
            break;
        case '2'://Save
        saving:
            clear_lcd();
            lcd_write_string("Saving Messeges:");
            lcd_set_cursor(1,0);
            lcd_write_string("0 1 2 3 4 5 6 7 8 9");
            lcd_set_cursor(2,0);
            lcd_write_string("# New Saved Messeges");
            lcd_set_cursor(3,0);
            lcd_write_string("Press * To Return");
            while(1)
            {
                switch (scan_keypad())
                {
                case '0':
                saving0:
                    clear_lcd();
                    lcd_write_string("Messege_0:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[0]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[0], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_0:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[0]);
                            goto saving0;
                        case '2':
                            goto saving;
                        }
                    }
                case '1':
                saving1:
                    clear_lcd();
                    lcd_write_string("Messege_1:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[1]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[1], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_1:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[1]);
                            goto saving1;
                        case '2':
                            goto saving;
                        }
                    }
                case '2':
                saving2:
                    clear_lcd();
                    lcd_write_string("Messege_2:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[2]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[2], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_2:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[2]);
                            goto saving2;
                        case '2':
                            goto saving;
                        }
                    }
                case '3':
                saving3:
                    clear_lcd();
                    lcd_write_string("Messege_3:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[3]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[3], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_3:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[3]);
                            goto saving3;
                        case '2':
                            goto saving;
                        }
                    }
                case '4':
                saving4:
                    clear_lcd();
                    lcd_write_string("Messege_4:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[4]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[4], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_4:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[4]);
                            goto saving4;
                        case '2':
                            goto saving;
                        }
                    }  
                case '5':
                saving5:
                    clear_lcd();
                    lcd_write_string("Messege_5:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[5]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[5], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_5:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[5]);
                            goto saving5;
                        case '2':
                            goto saving;
                        }
                    }
                case '6':
                saving6:
                    clear_lcd();
                    lcd_write_string("Messege_6:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[6]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[6], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_6:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[6]);
                            goto saving6;
                        case '2':
                            goto saving;
                        }
                    }
                case '7':
                saving7:
                    clear_lcd();
                    lcd_write_string("Messege_7:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[7]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[7], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_7:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[7]);
                            goto saving7;
                        case '2':
                            goto saving;
                        }
                    }
                case '8':
                saving8:
                    clear_lcd();
                    lcd_write_string("Messege_8:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[8]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[8], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_8:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[8]);
                            goto saving8;
                        case '2':
                            goto saving;
                        }
                    }
                case '9':
                saving9:
                    clear_lcd();
                    lcd_write_string("Messege_9:");
                    lcd_set_cursor(1,0);
                    lcd_write_string(code_phrase[9]);
                    lcd_set_cursor(3,0);
                    lcd_write_string("1.Delete 2.Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '1':
                            strcpy(code_phrase[9], buffer_zero);
                            clear_lcd();
                            lcd_write_string("Messege_9:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase[9]);
                            goto saving9;
                        case '2':
                            goto saving;
                        }
                    }
                case '#':
                saved:
                    clear_lcd();
                    lcd_write_string("Saved Messeges:");
                    lcd_set_cursor(1,0);
                    lcd_write_string("0 1 2 3 4 5 6 7 8 9");
                    lcd_set_cursor(3,0);
                    lcd_write_string("Press * To Return");
                    while(1)
                    {
                        switch (scan_keypad())
                        {
                        case '0':
                        saved0:
                            clear_lcd();
                            lcd_write_string("Messege_0:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[0]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[0], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_0:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[0]);
                                    goto saved0;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '1':
                        saved1:
                            clear_lcd();
                            lcd_write_string("Messege_1:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[1]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[1], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_1:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[1]);
                                    goto saved1;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '2':
                        saved2:
                            clear_lcd();
                            lcd_write_string("Messege_2:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[2]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[2], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_2:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[2]);
                                    goto saved2;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '3':
                        saved3:
                            clear_lcd();
                            lcd_write_string("Messege_3:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[3]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[3], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_3:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[3]);
                                    goto saved3;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '4':
                        saved4:
                            clear_lcd();
                            lcd_write_string("Messege_4:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[4]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[4], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_4:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[4]);
                                    goto saved4;
                                case '2':
                                    goto saved;
                                }
                            }  
                        case '5':
                        saved5:
                            clear_lcd();
                            lcd_write_string("Messege_5:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[5]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[5], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_5:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[5]);
                                    goto saved5;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '6':
                        saved6:
                            clear_lcd();
                            lcd_write_string("Messege_6:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[6]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[6], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_6:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[6]);
                                    goto saved6;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '7':
                        saved7:
                            clear_lcd();
                            lcd_write_string("Messege_7:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[7]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[7], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_7:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[7]);
                                    goto saved7;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '8':
                        saved8:
                            clear_lcd();
                            lcd_write_string("Messege_8:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[8]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[8], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_8:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[8]);
                                    goto saved8;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '9':
                        saved9:
                            clear_lcd();
                            lcd_write_string("Messege_9:");
                            lcd_set_cursor(1,0);
                            lcd_write_string(code_phrase_changeable[9]);
                            lcd_set_cursor(3,0);
                            lcd_write_string("1.Delete 2.Return");
                            while(1)
                            {
                                switch (scan_keypad())
                                {
                                case '1':
                                    strcpy(code_phrase_changeable[9], buffer_zero);
                                    clear_lcd();
                                    lcd_write_string("Messege_9:");
                                    lcd_set_cursor(1,0);
                                    lcd_write_string(code_phrase_changeable[9]);
                                    goto saved9;
                                case '2':
                                    goto saved;
                                }
                            }
                        case '*':
                            clear_lcd();
                            goto saving;
                        }          
                    }
                case '*':
                    clear_lcd();
                    goto flag_main;
                }          
            }
        case '3':
            flag_main = 0;
            break;
        }
    }
}
