--
-------------------------------------------------------------------------------------------
-- Copyright  2011, Xilinx, Inc.
-- This file contains confidential and proprietary information of Xilinx, Inc. and is
-- protected under U.S. and international copyright and other intellectual property laws.
-------------------------------------------------------------------------------------------
--
-- Disclaimer:
-- This disclaimer is not a license and does not grant any rights to the materials
-- distributed herewith. Except as otherwise provided in a valid license issued to
-- you by Xilinx, and to the maximum extent permitted by applicable law: (1) THESE
-- MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX HEREBY
-- DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
-- INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT,
-- OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable
-- (whether in contract or tort, including negligence, or under any other theory
-- of liability) for any loss or damage of any kind or nature related to, arising
-- under or in connection with these materials, including for any direct, or any
-- indirect, special, incidental, or consequential loss or damage (including loss
-- of data, profits, goodwill, or any type of loss or damage suffered as a result
-- of any action brought by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the possibility of the same.
--
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-safe, or for use in any
-- application requiring fail-safe performance, such as life-support or safety
-- devices or systems, Class III medical devices, nuclear facilities, applications
-- related to the deployment of airbags, or any other applications that could lead
-- to death, personal injury, or severe property or environmental damage
-- (individually and collectively, "Critical Applications"). Customer assumes the
-- sole risk and liability of any use of Xilinx products in Critical Applications,
-- subject only to applicable laws and regulations governing limitations on product
-- liability.
--
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.
--
-------------------------------------------------------------------------------------------
--
--
--
--             _ ______ ____ ____ __ __ __
--             | |/ / ___| _ \/ ___|| \/ |/ /_
--             | ' / | | |_) \___ \| |\/| | '_ \
--             | . \ |___| __/ ___) | | | | (_) |
--             |_|\_\____|_| |____/|_| |_|\___/
-- 
-- 
--
-- This reference design is to illustrate a way in which a KCPSM6 processor can implement 
-- a PMBus protocol and communicate with the UCD9248 power supply controller
-- (Texas Instruments) on the KC705 board. The design also implements a bridge between a 
-- UART and a Block Memory (BRAM) within a device so that information associated with the 
-- power supply controller can be observed both outside of the device and by another 
-- circuit connected to the second port of the BRAM at some point in the future.
--
-- It implements a  115200 baud, 1 stop bit, no parity, no handshake UART connection 
-- providing simple text based commands which enable the BRAM treated as 1K words of 
-- 32-bits to be read from and written to. 
--
-- All data values are represented as 8-digit hexadecimal values.
--
-- Whilst this bridge design could be more efficiently implemented by exploiting the 
-- ability for the port of the BRAM to be configured as 9-bits (8-bits plus parity) a 
-- full 32-bit data path has been created. The reason for this is that it has been
-- designed initially for an application in which the second port of the BRAM will be 
-- used in a 32-bit application. It may be important that all bytes within a 32-bit 
-- location are read or written in one transaction (i.e. If four byte transactions 
-- were used by this bridge to write a new 32-bit value then the 32-bit application 
-- may observe a the intermediate values which would be undesirable).    
-- 
-- If a frequency applied to the bridge module is not 50MHz then the KCPSM6 program will 
-- require some adjustments to maintain the same communication settings.
-- 
-- IMPORTANT: The BRAM must be connected to this module using the same 50MHz clock
--            (Synchronous interface).
--
--
--
-------------------------------------------------------------------------------------------
--
-- Library declarations
--
-- Standard IEEE libraries
--
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
--
-- The Unisim Library is used to define Xilinx primitives. It is also used during
-- simulation. The source can be viewed at %XILINX%\vhdl\src\unisims\unisim_VCOMP.vhd
-- 
library unisim;
use unisim.vcomponents.all;
--
--
-------------------------------------------------------------------------------------------
--
--
entity clock_control is
  Port (       
              i2c_clk : inout std_logic;
               i2c_data : inout std_logic;
          i2c_mux_rst_n : out std_logic;
           si5324_rst_n : out std_logic;
                    rst : in std_logic;
                  clk50 : in std_logic);
  end clock_control;

--
-------------------------------------------------------------------------------------------
--
-- Start of test architecture
--
architecture Behavioral of clock_control is
--
-------------------------------------------------------------------------------------------
--
-- Components
--
-------------------------------------------------------------------------------------------
--

--
-- declaration of KCPSM6
--

  component kcpsm6 
    generic(                 hwbuild : std_logic_vector(7 downto 0) := X"00";  
                    interrupt_vector : std_logic_vector(11 downto 0) := X"3FF";
             scratch_pad_memory_size : integer := 64);
    port (                   address : out std_logic_vector(11 downto 0);
                         instruction : in std_logic_vector(17 downto 0);
                         bram_enable : out std_logic;
                             in_port : in std_logic_vector(7 downto 0);
                            out_port : out std_logic_vector(7 downto 0);
                             port_id : out std_logic_vector(7 downto 0);
                        write_strobe : out std_logic;
                      k_write_strobe : out std_logic;
                         read_strobe : out std_logic;
                           interrupt : in std_logic;
                       interrupt_ack : out std_logic;
                               sleep : in std_logic;
                               reset : in std_logic;
                                 clk : in std_logic);
  end component;

--
-- KCPSM6 Program Memory with option for JTAG Loader
--

  component clock_control_program
    generic(             C_FAMILY : string := "S6"; 
                C_RAM_SIZE_KWORDS : integer := 1;
             C_JTAG_LOADER_ENABLE : integer := 0);
    Port (      address : in std_logic_vector(11 downto 0);
            instruction : out std_logic_vector(17 downto 0);
                 enable : in std_logic;
                    rdl : out std_logic;                    
                    clk : in std_logic);
  end component;
  

--
-------------------------------------------------------------------------------------------
--
-- Signals
--
-------------------------------------------------------------------------------------------
--
--
-- Signals used to connect KCPSM6
--
signal          address : std_logic_vector(11 downto 0);
signal      instruction : std_logic_vector(17 downto 0);
signal      bram_enable : std_logic;
signal          in_port : std_logic_vector(7 downto 0);
signal         out_port : std_logic_vector(7 downto 0);
signal          port_id : std_logic_vector(7 downto 0);
signal     write_strobe : std_logic;
signal   k_write_strobe : std_logic;
signal      read_strobe : std_logic;
signal        interrupt : std_logic;
signal    interrupt_ack : std_logic;
signal     kcpsm6_sleep : std_logic;
signal     kcpsm6_reset : std_logic;
signal              rdl : std_logic;
--
--
--
--
signal             drive_i2c_clk : std_logic;
signal            drive_i2c_data : std_logic;

signal   i2c_mux_rst_b_int : std_logic;
signal    si5324_rst_n_int : std_logic; 

attribute CORE_GENERATION_INFO : string;
attribute CORE_GENERATION_INFO of Behavioral : architecture is "v7_xt_conn_trd,v7_xt_conn_trd_v1_1_clock_control,{clock_control=2013.2}";

-------------------------------------------------------------------------------------------
--
-- Start of circuit description
--
-------------------------------------------------------------------------------------------
--
begin

i2c_mux_rst_n <= i2c_mux_rst_b_int; -- active low reset
si5324_rst_n <= si5324_rst_n_int;
  --
  -----------------------------------------------------------------------------------------
  -- Instantiate KCPSM6 and connect to program ROM
  -----------------------------------------------------------------------------------------
  --
  -- The generics can be defined as required. In this case the 'hwbuild' value is used to 
  -- define a version using the ASCII code for the desired letter. The interrupt vector 
  -- has been set to address 7F0 which would provide 16 instructions to implement an 
  -- interrupt service route (ISR) before the end of a 2K program space. Interrupt is not 
  -- used in this design at this time but could be exploited in the future.
  --

  processor: kcpsm6
    generic map (                 hwbuild => X"41",    -- ASCII Character "A"
                         interrupt_vector => X"7F0",   
                  scratch_pad_memory_size => 64)
    port map(      address => address,
               instruction => instruction,
               bram_enable => bram_enable,
                   port_id => port_id,
              write_strobe => write_strobe,
            k_write_strobe => k_write_strobe,
                  out_port => out_port,
               read_strobe => read_strobe,
                   in_port => in_port,
                 interrupt => interrupt,
             interrupt_ack => interrupt_ack,
                     sleep => kcpsm6_sleep,
                     reset => kcpsm6_reset,
                       clk => clk50);
 
  kcpsm6_reset <= rdl or rst;
  kcpsm6_sleep <= '0';
  interrupt <= interrupt_ack;

  --
  -- Program memory up to 4k with JTAG Loader option
  -- 

  program_rom: clock_control_program
    generic map(             C_FAMILY => "7S", 
                    C_RAM_SIZE_KWORDS => 2,
                 C_JTAG_LOADER_ENABLE => 0)
    port map(      address => address,      
               instruction => instruction,
                    enable => bram_enable,
                       rdl => rdl,
                       clk => clk50);

  --
  -----------------------------------------------------------------------------------------
  -- Connections to I2C Bus
  -----------------------------------------------------------------------------------------
  --
  -- The data and clock should be treated as open collector bidirectional signals which 
  -- use a pull-up on the board to generate a High level.
  --

  i2c_clk  <= '0' when drive_i2c_clk = '0' else 'Z';
  i2c_data <= '0' when drive_i2c_data = '0' else 'Z';


  --
  -----------------------------------------------------------------------------------------
  -- General Purpose Output Ports 
  -----------------------------------------------------------------------------------------
  --

  output_ports: process(clk50)
  begin
    if clk50'event and clk50 = '1' then

      -- 'write_strobe' is used to qualify all writes to general output ports.
      if write_strobe = '1' then

        -- Write to status bits at port address 04 hex
        if port_id(2) = '1' then
          i2c_mux_rst_b_int <= out_port(0);
          si5324_rst_n_int <= out_port(1);
        end if;
        -- Write to I2C Bus at port address 08 hex
        if port_id(3) = '1' then
          drive_i2c_clk <= out_port(0);
          drive_i2c_data <= out_port(1);
        end if;
      end if;
     end if;
  end process;
  
  --
  -----------------------------------------------------------------------------------------
  -- General Purpose Input Ports. 
  -----------------------------------------------------------------------------------------
  --

  input_ports: process (clk50)
  begin
    if clk50'event and clk50 = '1' then

      case port_id(2 downto 0) is

        -- Read I2C Bus at port address 06 hex
        when "110" =>   in_port(0) <= i2c_clk;
                        in_port(1) <= i2c_data;
                        
        when others => in_port <= (others => '0');
      
      end case;
    end if;
  end process;
  
end Behavioral;

-------------------------------------------------------------------------------------------
--
-- END OF FILE clock_control.vhd
--
-------------------------------------------------------------------------------------------

