
#include "VPatmos.h"
#include "verilated.h"
#if VM_TRACE
#include "verilated_vcd_c.h"
#endif
#include <iostream>

class Emulator
{
  unsigned long m_tickcount;
  VPatmos *c;

  // For Uart:
  bool UART_on;
  int baudrate;
  int freq;
  char in_byte;
  char out_byte;
  int sample_counter;
  int bit_counter;
  char state;

public:
  Emulator(void)
  {
    c = new VPatmos;
    m_tickcount = 0l;

    //for UART
    UART_on = false;
    baudrate = 0;
    freq = 0;
    in_byte = 0;
    out_byte = 0;
    sample_counter = 0;
    bit_counter = 0;
    state = 'i'; // 0:idle 1:receiving
  }

  ~Emulator(void)
  {
    delete c;
    c = NULL;
  }

  void reset(void)
  {
    c->reset = 1;
    // Make sure any inheritance gets applied
    this->tick();
    c->reset = 0;
  }

  void tick(void)
  {
    // Increment our own internal time reference
    m_tickcount++;

    // Make sure any combinatorial logic depending upon
    // inputs that may have changed before we called tick()
    // has settled before the rising edge of the clock.
    c->clock = 0;
    c->eval();

    // Toggle the clock

    // Rising edge
    c->clock = 1;
    c->eval();

    // Falling edge
    c->clock = 0;
    c->eval();

    //UART emulation

    if (UART_on)
    {
      if (state == 'i')
      { // idle wait for start bit
        if (c->io_UartCmp_tx == 0)
        {
          state = 'r'; //receiving
        }
      }
      else if (state == 'r')
      {
        sample_counter++;
        if (sample_counter == ((freq / baudrate) + 1))
        { //+1 as i to go one clock tick to futher before sampling
          UART_read_bit();
          sample_counter = 0;
        }
      }
    }
  }

  long int get_tick_count(void)
  {
    return m_tickcount;
  }

  bool done(void) { return (Verilated::gotFinish()); }

  // UART methods below
  void UART_init(int set_baudrate, int set_freq)
  {
    baudrate = set_baudrate;
    freq = set_freq;
    UART_on = true;
  }

  void UART_read_bit(void)
  {
    bit_counter++;
    if (bit_counter == 9)
    {
      printf("%c", out_byte);
      out_byte = 0;
      bit_counter = 0;
      state = 'i';
    }
    else
    {
      out_byte = (c->io_UartCmp_tx << (bit_counter - 1)) | out_byte;
    }
  }
};

// Override Verilator definition so first $finish ends simulation
// Note: VL_USER_FINISH needs to be defined when compiling Verilator code
void vl_finish(const char *filename, int linenum, const char *hier)
{
  Verilated::flushCall();
  exit(0);
}

int main(int argc, char **argv, char **env)
{
  Verilated::commandArgs(argc, argv);
  Emulator *emu = new Emulator();
  emu->reset();
  emu->tick();
  emu->UART_init(115200, 80000000);

  while (!emu->done() && emu->get_tick_count() != 10000000)
  {
    emu->tick();
    //printf("%d \n", emu->get_tick_count());
  }
  printf("This is a hacked harness \n");
  exit(EXIT_SUCCESS);

  /*#if VM_TRACE
    Verilated::traceEverOn(true);
    VL_PRINTF("Enabling waves..");
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open(vcdfile.c_str());
#endif

#if VM_TRACE
    if (tfp) tfp->close();
    delete tfp;
#endif
    delete top;
    exit(0);*/
}