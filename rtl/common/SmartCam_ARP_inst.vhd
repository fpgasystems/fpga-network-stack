--------------------------------------------------------------------------------
-- Company: 
-- Engineer:
--
-- Create Date:   14:02:04 10/11/2013
-- Design Name:   
-- Module Name:   C:/Users/mblott/Desktop/SmartCAM/toe_sessionLup/SmartCamTest.vhd
-- Project Name:  toe_sessionLup
-- Target Device:  
-- Tool versions:  
-- Description:   
-- 
-- VHDL Test Bench Created by ISE for module: SmartCamWrap
-- 
-- Dependencies:
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
--
-- Notes: 
-- This testbench has been automatically generated using types std_logic and
-- std_logic_vector for the ports of the unit under test.  Xilinx recommends
-- that these types always be used for the top-level I/O of a design in order
-- to guarantee that the testbench will bind correctly to the post-implementation 
-- simulation model.
--------------------------------------------------------------------------------
LIBRARY ieee;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
 
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--USE ieee.numeric_std.ALL;
 
 ENTITY SmartCamCtlArp IS
generic ( keyLength     : integer   := 32;
          valueLength   : integer   := 48);
port (
	led1           : out std_logic;
	led0           : out std_logic;
	rst            : in std_logic;
	clk            : in std_logic;
	cam_ready      : out std_logic;
	
	lup_req_valid  : in std_logic;
	lup_req_ready  : out std_logic;
	lup_req_din    : in std_logic_vector(keyLength downto 0);

	lup_rsp_valid  : out std_logic;
	lup_rsp_ready  : in std_logic;
	lup_rsp_dout   : out std_logic_vector(valueLength downto 0);
	
	upd_req_valid  : in std_logic;	
	upd_req_ready  : out std_logic;	
	upd_req_din    : in std_logic_vector((keyLength + valueLength) + 1 downto 0); -- This will include the key, the value to be updated and one bit to indicate whether this is a delete op

	upd_rsp_valid  : out std_logic;	
	upd_rsp_ready  : in std_logic;	
	upd_rsp_dout   : out std_logic_vector(valueLength + 1 downto 0);

	--new_id_valid   : in std_logic;
	--new_id_ready   : out std_logic;
	--new_id_din     : in std_logic_vector(13 downto 0);
	
	--fin_id_valid   : out std_logic;
	--fin_id_ready   : in std_logic;
	--fin_id_dout    : out std_logic_vector(13 downto 0);
	
	debug     : out std_logic_vector(255 downto 0)		
);
END SmartCamCtlArp;
 
