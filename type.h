
/* ��������� ��� ���������� ���������� ����� � �������� */
struct baudrate{
	unsigned char tmod;
	unsigned char ckcon;
	unsigned char th1;
	unsigned int time15;
	unsigned int time35;
};
typedef struct baudrate baudrate;

struct modbus{
    unsigned char crcLo;
    unsigned char crcHi;
    unsigned char pdata * ptr;  //����� �� ������� ��� �����������
    unsigned char counter;    //�������� ���������/��������� �����
};
typedef struct modbus modbus;

/* ���������� �������� - ������� ����, �������*/
struct thread{
    unsigned char count;
    unsigned char pdata*ptr;
};
typedef struct thread thread;

/* ������ ������� ���� ��������� ����� � ������� �������*/
struct write_address_value{
    unsigned int address;
    unsigned int min;   //min. ��������
    unsigned int max;   //max. ��������
};
typedef struct write_address_value w_a_v;

//��� �������
struct SYS{
    unsigned int ADC;
    unsigned int T;
    unsigned int dac;
    unsigned int power;
    unsigned int U;//voltage
    unsigned int R[2];//resistance
    unsigned int rH;//humidity DHT11
    unsigned int tC;//temperature DHT11
};
typedef struct SYS SYS;

//NTC
struct NTC{
    char t;
    unsigned long R;
};
typedef struct NTC NTC;



