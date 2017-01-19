///////////////////////////////////////////////////////////////////////////////
// UART register and field definitions
///////////////////////////////////////////////////////////////////////////////

#define UART_RBR        0x0             // Receive buffer register
#define UART_THR        0x0             // Transmit Holding Register
#define UART_DLL        0x0             // Divisor latch low register
#define UART_IER        0x4             // Interrupt Enable Register
#define UART_DLH        0x4             // Divisor latch high register
#define UART_IIR        0x8             // Interrupt identify register

#define UART_FCR        0x8             // FIFO control register
#define UART_FCR_FIFO_EN        _SET_ONE_(0)    // FIFO enable
#define UART_FCR_RX_FIFO_RST    _SET_ONE_(1)    // Rx FIFO RST
#define UART_FCR_TX_FIFO_RST    _SET_ONE_(2)    // Tx FIFO RST
#define UART_FCR_DMA_MODE       _SET_ONE_(3)    // DMA mode
#define UART_FCR_TX_EMPTY_TRIG_S          4
#define UART_FCR_TX_EMPTY_TRIG_W          2
#define UART_FCR_TX_EMPTY_TRIG_M          _SET_ONES_(UART_FCR_TX_EMPTY_TRIG_S,UART_FCR_TX_EMPTY_TRIG_W)
#define UART_FCR_RX_EMPTY_TRIG_S          6
#define UART_FCR_RX_EMPTY_TRIG_W          2
#define UART_FCR_RX_EMPTY_TRIG_M          _SET_ONES_(UART_FCR_RX_EMPTY_TRIG_S,UART_FCR_RX_EMPTY_TRIG_W)

#define UART_LCR        0xc             // Line control Register
#define UART_LCR_NUMBITS_S             0       // bits per charecter
#define UART_LCR_NUMBITS_W             2
#define UART_LCR_NUMBITS_M             _SET_ONES_(UART_LCR_NUMBITS_S,UART_LCR_NUMBITS_W)
#define UART_LCR_DLAB   _SET_ONE_(7)    // Divisor Latch Address Bit
#define UART_LCR_PE     _SET_ONE_(3)    // Parity Enable bit
#define UART_LCR_PS_EVEN  _SET_ONE_(4)  // Parity Select ( even or odd )

#define UART_MCR        0x10            // Modem control Register
#define UART_MCR_RTS                    _SET_ONE_(1)
#define UART_MCR_AUTO_FC_EN             _SET_ONE_(5)

#define UART_LSR        0x14            // Line Status Register
#define UART_LSR_DR     _SET_ONE_(0)    // Data ready bit
#define UART_LSR_OE     _SET_ONE_(1)    // Overrun error indication : clear on read
#define UART_LSR_PE     _SET_ONE_(2)    // Parity Error indication  : clear on read
#define UART_LSR_FE     _SET_ONE_(3)    // Framing Error indication : clear on read
#define UART_LSR_BI     _SET_ONE_(4)    // Break Interrupt bit      : clear on read
#define UART_LSR_THRE   _SET_ONE_(5)    // Transmit Holding Reg Empty
#define UART_LSR_TEMT   _SET_ONE_(6)    // Transmitter Empty 
#define UART_LSR_FERR   _SET_ONE_(7)    // Error in Rx FIFO

#define UART_MSR        0x18            // Modem status Register
#define UART_MSR_CTS    _SET_ONE_(4)    // Status of CTS_N

// Griffin specific registers

#define UART_SCR        0x1c            // Scratch Register
#define UART_CCR        0x100           // clock configuration register
#define UART_RC         0x104           // reset control register
#define UART_CTL        0x108           // control register
#define UART_ISR        0x10c           // Interrupt status register
#define UART_IMR        0x110           // Interrupt mask register
#define UART_EX_IER     0x114           // interrupt enable register for external UART logic
#define UART_ETFSTS     0x118           // extended transmit FIFO status register
#define UART_ERFSTS     0x11c           // extended receive FIFO status register

// Data access UART registers

#define UART_ETF_PUSH11 0x400           // byte extended transmit FIFO write access ( min)
#define UART_ETF_PUSH12 0x5fc           // byte extended transmit FIFO write access ( max)
#define UART_ETF_POP11  0x600           // byte extended receive FIFO read access (min)
#define UART_ETF_POP12  0x7fc           // byte extended receive FIFO read access (max)
#define UART_ETF_PUSH41 0x800           // word extended transmit FIFO write access ( min)
#define UART_ETF_PUSH42 0xbfc           // word extended transmit FIFO write access ( max)
#define UART_ETF_POP41  0xc00           // word extended receive FIFO read access (min)
#define UART_ETF_POP42  0xffc           // word extended receive FIFO read access (max)







