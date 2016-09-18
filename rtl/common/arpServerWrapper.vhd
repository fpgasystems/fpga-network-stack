library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity arpServerWrapper is
    generic ( keyLength     : integer       := 32;
              valueLength   : integer       := 48);
    Port ( aclk                             : in    STD_LOGIC;
           aresetn                          : in    STD_LOGIC;
           myMacAddress                     : in    std_logic_vector(47 downto 0);
           myIpAddress                      : in    std_logic_vector(31 downto 0);
           axi_arp_to_arp_slice_tvalid      : out   std_logic;
           axi_arp_to_arp_slice_tready      : in    std_logic;
           axi_arp_to_arp_slice_tdata       : out   std_logic_vector(63 downto 0);
           axi_arp_to_arp_slice_tkeep       : out   std_logic_vector(7 downto 0);
           axi_arp_to_arp_slice_tlast       : out   std_logic;
           axis_arp_lookup_reply_TVALID     : out   std_logic;
           axis_arp_lookup_reply_TREADY     : in    std_logic;
           axis_arp_lookup_reply_TDATA      : out   std_logic_vector(55 downto 0);
           axi_arp_slice_to_arp_tvalid      : in    std_logic;
           axi_arp_slice_to_arp_tready      : out   std_logic;
           axi_arp_slice_to_arp_tdata       : in    std_logic_vector(63 downto 0);
           axi_arp_slice_to_arp_tkeep       : in    std_logic_vector(7 downto 0);
           axi_arp_slice_to_arp_tlast       : in    std_logic;
           axis_arp_lookup_request_TVALID   : in    std_logic;
           axis_arp_lookup_request_TREADY   : out   std_logic;
           axis_arp_lookup_request_TDATA    : in    std_logic_vector(keyLength - 1 downto 0));
end arpServerWrapper;

architecture Structural of arpServerWrapper is

COMPONENT arp_server_ip
    PORT(regIpAddress_V             : IN  std_logic_vector(31 downto 0);
         myMacAddress_V             : in  std_logic_vector(47 downto 0);
         aresetn                    : IN  std_logic;
         aclk                       : IN  std_logic;
         macUpdate_resp_TDATA     : IN  std_logic_vector(55 downto 0);
         macUpdate_resp_TREADY    : OUT std_logic;
         macUpdate_resp_TVALID    : IN  std_logic;
         macUpdate_req_TDATA      : OUT std_logic_vector(87 downto 0);
         macUpdate_req_TREADY     : IN  std_logic;
         macUpdate_req_TVALID     : OUT  std_logic;
         macLookup_resp_TDATA     : IN  std_logic_vector(55 downto 0);
         macLookup_resp_TREADY    : OUT std_logic;
         macLookup_resp_TVALID    : IN  std_logic;     
         macLookup_req_TDATA      : OUT std_logic_vector(39 downto 0);
         macLookup_req_TREADY     : IN  std_logic;         
         macLookup_req_TVALID     : OUT std_logic;
         macIpEncode_rsp_TDATA    : OUT std_logic_vector(55 downto 0);
         macIpEncode_rsp_TREADY   : IN  std_logic;
         macIpEncode_rsp_TVALID   : OUT std_logic;
         arpDataOut_TLAST           : OUT std_logic;
         arpDataOut_TKEEP           : OUT std_logic_vector(7 downto 0);
         arpDataOut_TDATA           : OUT std_logic_vector(63 downto 0);
         arpDataOut_TREADY          : IN  std_logic;
         arpDataOut_TVALID          : OUT std_logic;
         macIpEncode_req_TDATA  : IN  std_logic_vector(31 downto 0);
         macIpEncode_req_TREADY : OUT std_logic;
         macIpEncode_req_TVALID : IN  std_logic;
         arpDataIn_TLAST            : IN  std_logic;
         arpDataIn_TKEEP            : IN  std_logic_vector(7 downto 0);
         arpDataIn_TDATA            : IN  std_logic_vector(63 downto 0);
         arpDataIn_TREADY           : OUT std_logic;
         arpDataIn_TVALID           : IN  std_logic);
END COMPONENT;
    
signal invertedReset    :   std_logic;

signal  lup_req_TVALID  :   std_logic;
signal  lup_req_TREADY  :   std_logic;
signal  lup_req_TDATA   :   std_logic_vector(39 downto 0);
signal  lup_req_TDATA_im:   std_logic_vector(keyLength downto 0);
signal  lup_rsp_TVALID  :   std_logic;
signal  lup_rsp_TREADY  :   std_logic;
signal  lup_rsp_TDATA   :   std_logic_vector(55 downto 0);
signal  lup_rsp_TDATA_im:   std_logic_vector(valueLength downto 0);
signal  upd_req_TVALID  :   std_logic;
signal  upd_req_TREADY  :   std_logic;
signal  upd_req_TDATA   :   std_logic_vector(87 downto 0);
signal  upd_req_TDATA_im:   std_logic_vector((keyLength + valueLength) + 1 downto 0);
signal  upd_rsp_TVALID  :   std_logic;
signal  upd_rsp_TREADY  :   std_logic;
signal  upd_rsp_TDATA   :   std_logic_vector(55 downto 0);
signal  upd_rsp_TDATA_im:   std_logic_vector(valueLength + 1 downto 0);
begin

lup_req_TDATA_im    <= lup_req_TDATA(32 downto 0);
lup_rsp_TDATA       <= "0000000" & lup_rsp_TDATA_im;
upd_req_TDATA_im    <= upd_req_TDATA((keyLength + valueLength) + 1 downto 0);
upd_rsp_TDATA       <= "000000" & upd_rsp_TDATA_im;
invertedReset       <= NOT aresetn;
-- SmartCam Wrapper
SmartCamCtl_inst: entity work.SmartCamCtlArp--(behavior)
                  port map(clk              =>  aclk,
                           rst              =>  invertedReset,
                           led0             =>  open,
                           led1             =>  open,
                           cam_ready        =>  open,
                           lup_req_valid    =>  lup_req_TVALID,
                           lup_req_ready    =>  lup_req_TREADY,
                           lup_req_din      =>  lup_req_TDATA_im,
                           lup_rsp_valid    =>  lup_rsp_TVALID,
                           lup_rsp_ready    =>  lup_rsp_TREADY,
                           lup_rsp_dout     =>  lup_rsp_TDATA_im,
                           upd_req_valid    =>  upd_req_TVALID,
                           upd_req_ready    =>  upd_req_TREADY,
                           upd_req_din      =>  upd_req_TDATA_im,
                           upd_rsp_valid    =>  upd_rsp_TVALID,
                           upd_rsp_ready    =>  upd_rsp_TREADY,
                           upd_rsp_dout     =>  upd_rsp_TDATA_im,
                           debug            =>  open);

-- ARP Server
arp_server_inst:   arp_server_ip 
                 port map (regIpAddress_V                 =>  myIpAddress, 
                           myMacAddress_V                 =>  myMacAddress,
                           arpDataIn_TVALID               =>  axi_arp_slice_to_arp_tvalid,                  
                           arpDataIn_TREADY               =>  axi_arp_slice_to_arp_tready,                    
                           arpDataIn_TDATA                =>  axi_arp_slice_to_arp_tdata,                      
                           arpDataIn_TKEEP                =>  axi_arp_slice_to_arp_tkeep,                     
                           arpDataIn_TLAST                =>  axi_arp_slice_to_arp_tlast,                     
                           macIpEncode_req_TVALID     =>  axis_arp_lookup_request_TVALID,    
                           macIpEncode_req_TREADY     =>  axis_arp_lookup_request_TREADY,    
                           macIpEncode_req_TDATA      =>  axis_arp_lookup_request_TDATA,     
                           arpDataOut_TVALID              =>  axi_arp_to_arp_slice_tvalid,                   
                           arpDataOut_TREADY              =>  axi_arp_to_arp_slice_tready,                    
                           arpDataOut_TDATA               =>  axi_arp_to_arp_slice_tdata,                      
                           arpDataOut_TKEEP               =>  axi_arp_to_arp_slice_tkeep,                     
                           arpDataOut_TLAST               =>  axi_arp_to_arp_slice_tlast,                      
                           macIpEncode_rsp_TVALID         =>  axis_arp_lookup_reply_TVALID, 
                           macIpEncode_rsp_TREADY         =>  axis_arp_lookup_reply_TREADY,  
                           macIpEncode_rsp_TDATA          =>  axis_arp_lookup_reply_TDATA,  
                           macLookup_req_TVALID           =>  lup_req_TVALID,
                           macLookup_req_TREADY           =>  lup_req_TREADY,
                           macLookup_req_TDATA            =>  lup_req_TDATA,
                           macLookup_resp_TVALID          =>  lup_rsp_TVALID,
                           macLookup_resp_TREADY          =>  lup_rsp_TREADY,
                           macLookup_resp_TDATA           =>  lup_rsp_TDATA,
                           macUpdate_req_TVALID         =>  upd_req_TVALID,
                           macUpdate_req_TREADY         =>  upd_req_TREADY,
                           macUpdate_req_TDATA          =>  upd_req_TDATA,
                           macUpdate_resp_TVALID        =>  upd_rsp_TVALID,
                           macUpdate_resp_TREADY        =>  upd_rsp_TREADY,
                           macUpdate_resp_TDATA         =>  upd_rsp_TDATA,
                           aclk                         =>  aclk,                             
                           aresetn                       =>  aresetn);    
end Structural;