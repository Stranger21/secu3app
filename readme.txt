    SECU-3 Application software. Distributed under GPL license

    Designed by Alexey A. Shabelnikov 2007. Ukraine, Gorlovka.
    Microprocessors systems - design & programming.
    http://secu-3.org e-mail: shabelnikov@secu-3.org


    How to compile the project
    ��� ������������� ������

    It is possible to compile project for ATMega16, ATMega32, ATMega64. Version
for ATMega64 compiles,but it will not work! You can compile the project using 
either IAR(MS Windows) or GCC(Linux, MS Windows).
    Under MS Windows: Run configure.bat with corresponding options (type of MCU
                      and type of compiler),it will create Makefile and start 
                      building.
    Under Linux:      Run configure.sh with option - type of MCU, it will create
                      Makefile and start building.

    ������ ����� �������������� ��� ATMega16, ATMega32, ATMega64. ��� ATMega64 
��� �������������, �� �������� �� �� �����! �� ������ ������������� ������ 
��������� IAR ��� GCC. ��������� configure.bat c ���������������� ������� (��� 
���������������� � ��� �����������), ����� ������ Makefile � �������� ������ 
�������.

    List of symbols which affects compilation:
    ������ �������� ����������� �����������:

    VPSEM                For using of starter blocking output for indication of 
                         idle economizer valve's state
                         ��� ��������� ��������� ������� ���� ������������ �����
                         ���������� ��������


    WHEEL_36_1           For using 36-1 crank (60-2 used by default)
                         ��� ������������� ��������� ����� 36-1 (�� ��������� 
                         60-2)


    INVERSE_IGN_OUTPUTS  Use for to invert ignition outputs
                         ��� �������������� ������� ���������� ����������


    DWELL_CONTROL        For direct controlling of dwell
                         ��� ������� ���������� ����������� ������� � �������� 
                         ���������


    COOLINGFAN_PWM       Use PWM for controlling of electric cooling fan
                         ������������ ��� ��� ��� ��� ���������� ��������� 
                         �����������

    REALTIME_TABLES      Allow editing of tables in realtime (use RAM)
                         ��������� �������������� ������ � �������� �������

    DEBUG_VARIABLES      For watching and editing of some firmware variables 
                         (used for debug by developers)
                         ��������� ����� ������� ����������� ����������� � 
                         ������ ��������� ���������� ��������

    PHASE_SENSOR         Use of phase (cam) sensor
                         (��������� ������������� ������� ���)
   

    PHASED_IGNITION      Use phased ignition. PHASE_SENSOR must be also used.
                         (��������� ������������ ���������)

    FUEL_PUMP            Electric fuel pump control
                         (���������� �������������������)

    TPS_SENSOR           Use TPS
                         (��������� ������������� ���� PA5)

    BL_BAUD_RATE         Baud rate for boot loader. Can be set to 9600, 14400,
                         19200, 28800, 38400, 57600. Note! Will not take effect without
                         reprogramming using ISP programmator.
                         (�������� �������� ������ ��� ����������)

    THERMISTOR_CS        Use a resistive temperature sensor
                         (������������ ������ ����������� ����������� ��������
                         ������������ ����)

    SECU3T               Build for SECU-3T unit. Additional functionality will be added
                         (������ ��� ���� SECU-3T. ����������� ���. ����������������)

    DIAGNOSTICS          Include hardware diagnostics functionality
                         (�������� ��������� ����������� ���������� �����)

Necessary symbols you can define in the preprocessor's options of compiler
(edit corresponding Makefile).
������ ��� ������� �� ������ ���������� � ������ ������������� ����������� 
(������������ ��������������� Makefile).
