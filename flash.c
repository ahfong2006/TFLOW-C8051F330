


//0-��� ���� ��������� ������ � ��������� ���� ������ ���� �������
//1-��� ���� ��������� ������ � ��������� ���� ����� �������
//2-������ ������� ��������
code unsigned int flash_address = 1;
code unsigned int flash_baud = 6;
code unsigned int corr = 0;
code unsigned int ref = 2455;
code unsigned int idac = 5017;
code unsigned char arr[512 -  sizeof(flash_address) - sizeof(flash_baud) - sizeof(corr) - sizeof(ref) - sizeof(idac)];

