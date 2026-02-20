


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
