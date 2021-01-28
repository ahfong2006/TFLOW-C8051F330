

extern code baudrate br[];
extern bit renewFlash;
extern modbus mb;

extern xdata union{
    unsigned char modbuf[modbufsize];
    struct{
        unsigned char address;
        unsigned char function;
        unsigned char exeption;        
    }exception;//повідомлення про виключну ситуацію

    struct{
        unsigned char address;
        unsigned char function;
        unsigned int start_address;
        unsigned int quantity;
    }read_req;//читання Запит

    struct{
        unsigned char address;
        unsigned char function;
        unsigned char byte_count;        
    }read_resp;//читання відповідь

    struct{
        unsigned char address;
        unsigned char function;
        unsigned int register_address;
        unsigned int register_value;
    }write_req;//запис одного регістра Запит

    struct{
        unsigned char address;
        unsigned char function;
        unsigned int register_address;
        unsigned int register_value;
    }write_resp;//запис одного регістра відповідь

    struct{
        unsigned char address;
        unsigned char function;
        unsigned int start_address;
        unsigned int quantity;
        unsigned char byte_count;
        unsigned int reg[7];
    }write_mult_req;//запис регістрів Запит

    struct{
        unsigned char address;
        unsigned char function;
        unsigned int start_address;
        unsigned int quantity;
    }write_mult_resp;//запис регістрів відповідь

}buf;


//регістри модбаса
//для читання
extern xdata union{
    unsigned int reg[9];
    SYS sys;
}in;//

//для запису
extern xdata union{
  unsigned int reg[8];
  struct{
    unsigned int address;
    unsigned int baud;
    unsigned int corr;
    unsigned int ref;// voltage of reference in milivolts 2453 coresponds 2.453V
    unsigned int idac;// microamphere x 10 i.m. 5000 coresponds 0.5mA 
    unsigned int command;
  }out;
}out;//160 32-байти;

void modbus_init();
