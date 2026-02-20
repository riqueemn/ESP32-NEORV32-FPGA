/*
 Copyright (C) 2025  Altera Corporation. All rights reserved.
 Your use of Altera Corporation's design tools, logic functions 
 and other software and tools, and any partner logic 
 functions, and any output files from any of the foregoing 
 (including device programming or simulation files), and any 
 associated documentation or information are expressly subject 
 to the terms and conditions of the Altera Program License 
 Subscription Agreement, the Altera Quartus Prime License Agreement,
 the Altera IP License Agreement, or other applicable license
 agreement, including, without limitation, that your use is for
 the sole purpose of programming logic devices manufactured by
 Altera and sold by Altera or its authorized distributors.  Please
 refer to the Altera Software License Subscription Agreements 
 on the Quartus Prime software download page.
*/
MODEL
/*MODEL HEADER*/
/*
 This file contains Slow Corner delays for the design using part EP4CE6E22C8
 with speed grade 8, core voltage 1.2V, and temperature 0 Celsius

*/
MODEL_VERSION "1.0";
DESIGN "neorv32-teste-2";
DATE "02/19/2026 19:14:34";
PROGRAM "Quartus Prime";



INPUT rst_n;
INPUT clk;
INPUT uart_rxd;
INPUT btn_fire;
INPUT btn_intruder;
OUTPUT uart_txd;
OUTPUT led_lock;
OUTPUT led_alarm;
OUTPUT led_arcondicionado;
OUTPUT led_lampada;

/*Arc definitions start here*/
pos_btn_fire__clk__setup:		SETUP (POSEDGE) btn_fire clk ;
pos_btn_intruder__clk__setup:		SETUP (POSEDGE) btn_intruder clk ;
pos_uart_rxd__clk__setup:		SETUP (POSEDGE) uart_rxd clk ;
pos_btn_fire__clk__hold:		HOLD (POSEDGE) btn_fire clk ;
pos_btn_intruder__clk__hold:		HOLD (POSEDGE) btn_intruder clk ;
pos_uart_rxd__clk__hold:		HOLD (POSEDGE) uart_rxd clk ;
pos_clk__led_alarm__delay:		DELAY (POSEDGE) clk led_alarm ;
pos_clk__led_arcondicionado__delay:		DELAY (POSEDGE) clk led_arcondicionado ;
pos_clk__led_lampada__delay:		DELAY (POSEDGE) clk led_lampada ;
pos_clk__led_lock__delay:		DELAY (POSEDGE) clk led_lock ;
pos_clk__uart_txd__delay:		DELAY (POSEDGE) clk uart_txd ;

ENDMODEL
