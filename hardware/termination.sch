EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X04 P1
U 1 1 56C285EB
P 1450 1400
F 0 "P1" H 1369 1018 50  0000 C CNN
F 1 "CONN_01X04" H 1369 1110 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04" H 1369 1110 50  0001 C CNN
F 3 "" H 1450 1400 50  0000 C CNN
	1    1450 1400
	-1   0    0    1   
$EndComp
$Comp
L CONN_01X04 P2
U 1 1 56C286C4
P 2700 1400
F 0 "P2" H 2618 1018 50  0000 C CNN
F 1 "CONN_01X04" H 2618 1110 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x04" H 2700 1400 50  0001 C CNN
F 3 "" H 2700 1400 50  0000 C CNN
	1    2700 1400
	1    0    0    1   
$EndComp
Wire Wire Line
	1650 1250 2500 1250
Wire Wire Line
	1650 1350 2500 1350
Wire Wire Line
	1650 1450 2500 1450
Wire Wire Line
	1650 1550 2500 1550
$Comp
L R R2
U 1 1 56C2874D
P 1900 1750
F 0 "R2" H 1970 1796 50  0000 L CNN
F 1 "R" H 1970 1704 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 1830 1750 50  0001 C CNN
F 3 "" H 1900 1750 50  0000 C CNN
	1    1900 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1750 1550 1750 2200
Wire Wire Line
	1750 2200 2200 2200
Connection ~ 1750 1550
Connection ~ 1900 2200
Connection ~ 2200 2200
Wire Wire Line
	1900 1150 1900 1600
Connection ~ 1900 1250
Wire Wire Line
	2200 1600 2200 1250
Connection ~ 2200 1250
Wire Wire Line
	1900 1900 2050 1900
Wire Wire Line
	2050 1900 2050 1450
Connection ~ 2050 1450
Wire Wire Line
	2200 1900 2400 1900
Wire Wire Line
	2400 1900 2400 1350
Connection ~ 2400 1350
Connection ~ 1900 1900
Connection ~ 2200 1900
Text Label 1650 1550 0    60   ~ 0
GND
Text Label 1650 1250 0    60   ~ 0
+5V
$Comp
L R R3
U 1 1 56C289B6
P 1900 2050
F 0 "R3" H 1970 2096 50  0000 L CNN
F 1 "R" H 1970 2004 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 1830 2050 50  0001 C CNN
F 3 "" H 1900 2050 50  0000 C CNN
	1    1900 2050
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 56C289DA
P 2200 2050
F 0 "R5" H 2270 2096 50  0000 L CNN
F 1 "R" H 2270 2004 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 2130 2050 50  0001 C CNN
F 3 "" H 2200 2050 50  0000 C CNN
	1    2200 2050
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 56C28A01
P 2200 1750
F 0 "R4" H 2270 1796 50  0000 L CNN
F 1 "R" H 2270 1704 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 2130 1750 50  0001 C CNN
F 3 "" H 2200 1750 50  0000 C CNN
	1    2200 1750
	1    0    0    -1  
$EndComp
$Comp
L LED D1
U 1 1 56C28A27
P 2100 850
F 0 "D1" H 2100 604 50  0000 C CNN
F 1 "LED" H 2100 696 50  0000 C CNN
F 2 "LEDs:LED-0805" H 2100 696 50  0001 C CNN
F 3 "" H 2100 850 50  0000 C CNN
	1    2100 850 
	-1   0    0    1   
$EndComp
$Comp
L R R1
U 1 1 56C28AA5
P 1900 1000
F 0 "R1" H 1750 1150 50  0000 L CNN
F 1 "180" V 1800 950 50  0000 L CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 1830 1000 50  0001 C CNN
F 3 "" H 1900 1000 50  0000 C CNN
	1    1900 1000
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 850  2300 1550
Connection ~ 2300 1550
$Comp
L C C1
U 1 1 56C28BAF
P 2100 1150
F 0 "C1" V 1950 1050 50  0000 C CNN
F 1 "100n" V 2000 1300 50  0000 C CNN
F 2 "Capacitors_SMD:C_0805" H 2138 1000 50  0001 C CNN
F 3 "" H 2100 1150 50  0000 C CNN
	1    2100 1150
	0    1    1    0   
$EndComp
Wire Wire Line
	2250 1150 2300 1150
Connection ~ 2300 1150
Wire Wire Line
	1900 1150 1950 1150
Connection ~ 1900 1150
Text Label 1650 1450 0    60   ~ 0
D1
Text Label 1650 1350 0    60   ~ 0
D0
$EndSCHEMATC
