#!/usr/bin/python
import sys
import binascii

## kv_pair data structure ######################################################

# Example kv_pair: (key, value, flags, expiration)
ex_kv_pair = {
	"key"        : "ExampleKey",
	"value"      : "ExampleValue",
	"flags"      : "01234567",     # 32bit, hex-encoded
	"expiration" : 10
}

def kv_pair(key, value, flags, expiration):
	if (type(key) != str) | (type(value) != str) | (type(expiration) != int) | (type(flags) != str):
		raise Exception("Error: Wrong types for kv_pair")
	if len(flags) != 8:
		raise Exception("Error: Wrong flags length: %d. Should be 8." % len(flags))
	return {
		"key"        : key,
		"value"      : value,
		"flags"      : flags,
		"expiration" : expiration
	}

## binary protocol packet generators ###########################################

def binaryResponseTemplate(opcode, opaque, status="00"):
	magic             = "81"
	key_length_bin    = "0000"
	extras_length_bin = "00"
	data_type         = "00"
	status            = "00" + status
	body_length       = "00000000"
	cas               = "0000000000000000"

	response = magic + opcode + key_length_bin +        \
	           extras_length_bin + data_type + status + \
	           body_length +                            \
	           opaque +                                 \
	           cas

	return response

def binaryGetRequest(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	magic          = "80"
	opcode         = "00"
	key            = kv_pair["key"]
	key_bin        = key.encode('hex')
	key_length     = len(key)
	key_length_bin = "%04x" % key_length
	extras_length  = "00"
	data_type      = "00"
	vbucket_id     = "0000"
	body_length    = "%08x" % key_length
	cas            = "0000000000000000"

	request = magic + opcode + key_length_bin +        \
	          extras_length + data_type + vbucket_id + \
		  body_length +                            \
		  opaque +                                 \
		  cas +                                    \
		  key_bin

	return request

def binaryFailedGetResponse(opaque="01234567"):
	magic             = "81"
	opcode            = "00"
	key_length_bin    = "0000"
	extras_length_bin = "00"
	data_type         = "00"
	status            = "0001"
	body_length       = "00000008"
	cas               = "0000000000000000"
	message           = "ERROR 01".encode('hex')

	response = magic + opcode + key_length_bin +        \
	           extras_length_bin + data_type + status + \
	           body_length +                            \
	           opaque +                                 \
	           cas +                                    \
		   message

	return response

def binaryGetResponse(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	magic             = "81"
	opcode            = "00"
	key_length_bin    = "0000"
	extras            = kv_pair["flags"]
	extras_length     = len(extras) / 2
	extras_length_bin = "%02x" % extras_length
	data_type         = "00"
	status            = "0000"
	value             = kv_pair["value"]
	value_bin         = value.encode('hex')
	value_length      = len(value)
	body_length       = "%08x" % (extras_length + value_length)
	cas               = "0000000000000000"

	response = magic + opcode + key_length_bin +        \
	           extras_length_bin + data_type + status + \
	           body_length +                            \
	           opaque +                                 \
	           cas +                                    \
	           extras +                                 \
	           value_bin

	return response

def binarySetRequest(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	magic          = "80"
	opcode         = "01"
	key            = kv_pair["key"]
	key_bin        = key.encode('hex')
	key_length     = len(key)
	key_length_bin = "%04x" % key_length
	extras_length  = "08"
	data_type      = "00"
	vbucket_id     = "0000"
	value          = kv_pair["value"]
	value_bin      = value.encode('hex')
	value_length   = len(value)
	body_length    = "%08x" % (key_length + value_length + 8)
	cas            = "0000000000000000"
	flags          = kv_pair["flags"]
	expiration     = kv_pair["expiration"]
	expiration_bin = "%08x" % expiration
	extras         = flags + expiration_bin

	request = magic + opcode + key_length_bin +        \
	          extras_length + data_type + vbucket_id + \
		  body_length +                            \
		  opaque +                                 \
		  cas +                                    \
		  extras +                                 \
		  key_bin +                                \
		  value_bin

	return request

def binarySetResponse(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))
	return binaryResponseTemplate("01", opaque)

def binaryFailedSetResponse(opaque="01234567"):
	magic             = "81"
	opcode            = "01"
	key_length_bin    = "0000"
	extras_length_bin = "00"
	data_type         = "00"
	status            = "0001"
	body_length       = "00000008"
	cas               = "0000000000000000"
	message           = "ERROR 01".encode('hex')

	response = magic + opcode + key_length_bin +        \
	           extras_length_bin + data_type + status + \
	           body_length +                            \
	           opaque +                                 \
	           cas +                                    \
		   message

	return response

def binaryDeleteRequest(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	magic          = "80"
	opcode         = "04"
	key            = kv_pair["key"]
	key_bin        = key.encode('hex')
	key_length     = len(key)
	key_length_bin = "%04x" % key_length
	extras_length  = "00"
	data_type      = "00"
	vbucket_id     = "0000"
	body_length    = "%08x" % key_length
	cas            = "0000000000000000"

	request = magic + opcode + key_length_bin +        \
	          extras_length + data_type + vbucket_id + \
		  body_length +                            \
		  opaque +                                 \
		  cas +                                    \
		  key_bin

	return request

def binaryFailedDeleteResponse(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	magic             = "81"
	opcode            = "04"
	key_length_bin    = "0000"
	extras_length_bin = "00"
	data_type         = "00"
	status            = "0001"
	body_length       = "00000008"
	cas               = "0000000000000000"
	message           = "ERROR 01".encode('hex')

	response = magic + opcode + key_length_bin +        \
	           extras_length_bin + data_type + status + \
	           body_length +                            \
	           opaque +                                 \
	           cas +                                    \
		   message

	return response

def binaryDeleteResponse(kv_pair, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))
	return binaryResponseTemplate("04", opaque)

def binaryFlushRequest(expiration=0, opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))

	if(expiration!=0):
		extras_length = "04"
		expiration_bin = "%08x" % expiration
		body_length   = "00000004"

	magic          = "80"
	opcode         = "08"
	key_length_bin = "0000"
	extras_length  = "00"
	data_type      = "00"
	vbucket_id     = "0000"
	body_length    = "00000000"
	cas            = "0000000000000000"
	expiration_bin = ""

	if(expiration!=0):
		extras_length  = "04"
		body_length    = "00000004"
		expiration_bin = "%08x" % expiration

	request = magic + opcode + key_length_bin +        \
	          extras_length + data_type + vbucket_id + \
		  body_length +                            \
		  opaque +                                 \
		  cas +                                    \
		  expiration_bin

	return request

def binaryFlushResponse(opaque="01234567"):
	if len(opaque) != 8:
		raise Exception("Error: Wrong opaque length: %d. Should be 8." % len(opaque))
	return binaryResponseTemplate("08", opaque)

## text protocol packet generators #############################################

def textSetRequest(kv_pair):
	txt_req = "set %s %d %d %d\r\n%s\r\n" % (
		kv_pair['key'],
		int(kv_pair['flags'], 16),
		kv_pair['expiration'],
		len(kv_pair['value']),
		kv_pair['value'])
	return txt_req.encode('hex')

def textSetResponse(kv_pair):
	return "STORED\r\n".encode('hex')

def textGetRequest(kv_pair):
	txt_req = "get %s\r\n" % kv_pair['key']
	return txt_req.encode('hex')

def textGetResponse(kv_pair):
	txt_res = "VALUE %s %d %d\r\n%s\r\nEND\r\n" % (
		kv_pair['key'],
		int(kv_pair['flags'], 16),
		len(kv_pair['value']),
		kv_pair['value'])
	return txt_res.encode('hex')

def textFailedGetResponse():
	return "END\r\n".encode('hex')

def textDeleteRequest(kv_pair):
	txt_req = "delete %s\r\n" % kv_pair['key']
	return txt_req.encode('hex')

def textDeleteResponse(kv_pair):
	return "DELETED\r\n".encode('hex')

def textFailedDeleteResponse(kv_pair):
	return "NOT_FOUND\r\n".encode('hex')

def textFlushRequest():
	return "flush_all\r\n".encode('hex')

def textFlushResponse():
	return "OK\r\n".encode('hex')

## mixed protocol converter (from text) ########################################

def goMixed(txt_rq, rq_id=0):
	frame = "%04x000000010000" % rq_id
	return (frame + txt_rq)

## simulation marshalling ######################################################

network_sop = "D000ADB002020203719A0E0002054" # why '4' and not '0' {EOP, MOD}?
network_any = "D000ADB002020203719A0E0002044"
network_eop = "D000ADB002020203719A0E000204C"
network_sep = "D000ADB002020203719A0E000205C"

network_out_x = "D000ADB002020203719A0E00020"
network_out_sop = network_out_x + "50"
network_out_any = network_out_x + "40"

network_out_x_hls = "D0000000000000000000000000000"
network_out_sop_hls = "D0000ADB002020203719A0E0002050" #
network_out_any_hls = network_out_x_hls + "00"

source_ip_hls = "01020304" # Used for HLS packets. Only the network data are included. SOP & EOP signals are provided separately.
destination_ip_hls = "0A0B0C0D"

source_ip_hls_any_short = "00000000"
source_ip_hls_any = "00000000"

source_port = "0056"
destination_port = "0080"
checksum = "0000"

def split_string_into_len(str, size=8):
	return [ str[i:i+size] for i in range(0, len(str), size) ]

def hexstream_reverse_bytes(hexstream):
	bytes = split_string_into_len(hexstream, 2)
	reversed_bytes = bytes[::-1]
	return "".join(reversed_bytes)

# Empty: Control signals in the end set for 'empty' in simulation.
#        Use before W statement.

# expects list of hexstreams
def requests12Gbps(requests):
	requests = requests[:] # copy of list
	requests[0:-1] = map(lambda r: simulationInput(r, False), requests[0:-1])
	requests[-1] = simulationInput(requests[-1], True)
	return "".join(requests)

# expects list of hexstreams
def requests1Gbps(requests):
	input = map(lambda r: simulationInput(r, True), requests)
	pkg_count = map(lambda r: len(r) / 16, requests)
	pkg_wait_count = map(lambda c: 9*c, pkg_count)
	pkg_wait = map(lambda wc: "W%d\n" % wc, pkg_wait_count) # wait statement for each of the rqs.
	input_pairs = zip(pkg_wait, input)
	input_pairs = map(lambda p: "".join(p), input_pairs)    # wait and request combined for each rq
	return "".join(input_pairs)                             # 1 string w/ all rqs

# expects list of hexstreams
def responses(res):
	res = map(simulationOutput, res)
	return "".join(res)

###################### Modified functions to generate data for the UDP simulations ###########################

def calculateChecksum(ip_header, size):
    while (len(ip_header) % 4) != 0:
        ip_header = ip_header + "0"
        size = size + 1

    cksum = 0
    pointer = 0

    #The main loop adds up each set of 2 bytes. They are first converted to strings and then concatenated
    #together, converted to integers, and then added to the sum.
    while size > 1:
        temp = ip_header[pointer:pointer+4]
        temp2 = int(temp, 16)
        cksum += temp2
        size -= 4
        pointer += 4
    if size: #This accounts for a situation where the header is odd
        cksum += int(ip_header[pointer], 16)

    cksum = (cksum >> 16) + (cksum & 0xffff)
    cksum += (cksum >>16)

    return (~cksum) & 0xFFFF

def calculateWrongChecksum(ip_header, size):
    while (len(ip_header) % 4) != 0:
        ip_header = ip_header + "0"
        size = size + 1

    cksum = 0
    pointer = 0

    #The main loop adds up each set of 2 bytes. They are first converted to strings and then concatenated
    #together, converted to integers, and then added to the sum.
    while size > 1:
        temp = ip_header[pointer:pointer+4]
        temp2 = int(temp, 16)
        cksum -= temp2
        size -= 4
        pointer += 4
    if size: #This accounts for a situation where the header is odd
        cksum += int(ip_header[pointer], 16)

    cksum = (cksum >> 16) + (cksum & 0xffff)
    cksum += (cksum >>16)

    return (~cksum) & 0xFFFF

def simulationInput_Rx(hexstream, empty=True):

    lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
    #hexstream = hexstream.upper()
    requestLength = str((len(hexstream)/2) + 8)
    while len(requestLength) % 4 != 0:
        requestLength = requestLength + "0"

    ipLength = hex(28 + (len(hexstream)/2))
    #print(ipLength)
    ipLength = ipLength[2:]
    #print(ipLength)
    while len(ipLength) % 4 != 0:
        ipLength = "0" + ipLength
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    ip_header = "4500" + ipLength + "00000000FF0600000A0A0A0A01010101"
    udp_header = source_port + destination_port + udpLength + checksum
    #Calculate checksum for UDP packet. First construct the pseudoheader
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    checksumPacket = "0A0A0A0A01010101" + "00" + "11" + udpLength + udp_header + hexstream
    #print(checksumPacket)
    pseudoLength = len(checksumPacket)
    #print(pseudoLength)
    checksumPacket = checksumPacket.upper()
    binascii.hexlify(checksumPacket)
    checksumResults = hex(calculateChecksum(checksumPacket, pseudoLength)) # Checksum result is in integer format now, convert to hex
    checksumResults = checksumResults[2:]
    while (len(checksumResults) % 4) != 0:
        checksumResults = "0" + checksumResults
    udp_header = source_port + destination_port + udpLength + checksumResults

    hexstream = ip_header + udp_header + hexstream
    hexstream = hexstream.upper()
    residue = (len(hexstream) % 16) / 2
    if (residue != 8):
        keepSignal = lastDict[residue]
    else:
        keepSignal = "FF"
    while len(hexstream) % 16 != 0:
        hexstream = hexstream + "00"

    words = split_string_into_len(hexstream, 16)
    words = map(hexstream_reverse_bytes, words)

    if(len(words)==1):
        l2 =  words[0] + " " + '1' + " " + keepSignal + "\n"
        input = l2
    elif(len(words)==2):
        l1 = words[0] + " " + '0' + " " + "FF" + "\n"
        l2 = words[1] + " " + '1' + " " + "FF" + "\n"
        input = l1 + l2
    else:
        words[0] = words[0] + " " + '0' + " " + "FF" + "\n"
        words[1:-2] = map(lambda s: s + " " + "0" + " " + "FF" + "\n", words[1:-2])
        words[-2] = words[-2] + " " + '0' + " " + "FF" + "\n"
        words[-1] = words[-1] + " " + '1' + " " + keepSignal + "\n"
        input = "".join(words)
    return input

def simulationInput_Rx_wrongCS(hexstream, empty=True):

    lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
    #hexstream = hexstream.upper()
    requestLength = str((len(hexstream)/2) + 8)
    while len(requestLength) % 4 != 0:
        requestLength = requestLength + "0"

    ipLength = hex(28 + (len(hexstream)/2))
    #print(ipLength)
    ipLength = ipLength[2:]
    #print(ipLength)
    while len(ipLength) % 4 != 0:
        ipLength = "0" + ipLength
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    ip_header = "4500" + ipLength + "00000000FF0600000A0A0A0A01010101"
    udp_header = source_port + destination_port + udpLength + checksum
    #Calculate checksum for UDP packet. First construct the pseudoheader
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    checksumPacket = "0A0A0A0A01010101" + "00" + "11" + udpLength + udp_header + hexstream
    #print(checksumPacket)
    pseudoLength = len(checksumPacket)
    #print(pseudoLength)
    checksumPacket = checksumPacket.upper()
    binascii.hexlify(checksumPacket)
    checksumResults = hex(calculateWrongChecksum(checksumPacket, pseudoLength)) # Checksum result is in integer format now, convert to hex
    checksumResults = checksumResults[2:]
    while (len(checksumResults) % 4) != 0:
        checksumResults = "0" + checksumResults
    udp_header = source_port + destination_port + udpLength + checksumResults

    hexstream = ip_header + udp_header + hexstream
    hexstream = hexstream.upper()
    residue = (len(hexstream) % 16) / 2
    if (residue != 8):
        keepSignal = lastDict[residue]
    else:
        keepSignal = "FF"
    while len(hexstream) % 16 != 0:
        hexstream = hexstream + "00"

    words = split_string_into_len(hexstream, 16)
    words = map(hexstream_reverse_bytes, words)

    if(len(words)==1):
        l2 =  words[0] + " " + '1' + " " + keepSignal + "\n"
        input = l2
    elif(len(words)==2):
        l1 = words[0] + " " + '0' + " " + "FF" + "\n"
        l2 = words[1] + " " + '1' + " " + "FF" + "\n"
        input = l1 + l2
    else:
        words[0] = words[0] + " " + '0' + " " + "FF" + "\n"
        words[1:-2] = map(lambda s: s + " " + "0" + " " + "FF" + "\n", words[1:-2])
        words[-2] = words[-2] + " " + '0' + " " + "FF" + "\n"
        words[-1] = words[-1] + " " + '1' + " " + keepSignal + "\n"
        input = "".join(words)
    return input

def simulationInput_Rx_noCS(hexstream, empty=True):

    lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
    #hexstream = hexstream.upper()
    requestLength = str((len(hexstream)/2) + 8)
    while len(requestLength) % 4 != 0:
        requestLength = requestLength + "0"

    ipLength = hex(28 + (len(hexstream)/2))
    #print(ipLength)
    ipLength = ipLength[2:]
    #print(ipLength)
    while len(ipLength) % 4 != 0:
        ipLength = "0" + ipLength
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    ip_header = "4500" + ipLength + "00000000FF0600000A0A0A0A01010101"
    udp_header = source_port + destination_port + udpLength + checksum
    #Calculate checksum for UDP packet. First construct the pseudoheader
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    udp_header = source_port + destination_port + udpLength + "0000"

    hexstream = ip_header + udp_header + hexstream
    hexstream = hexstream.upper()
    residue = (len(hexstream) % 16) / 2
    if (residue != 8):
        keepSignal = lastDict[residue]
    else:
        keepSignal = "FF"
    while len(hexstream) % 16 != 0:
        hexstream = hexstream + "00"

    words = split_string_into_len(hexstream, 16)
    words = map(hexstream_reverse_bytes, words)

    if(len(words)==1):
        l2 =  words[0] + " " + '1' + " " + keepSignal + "\n"
        input = l2
    elif(len(words)==2):
        l1 = words[0] + " " + '0' + " " + "FF" + "\n"
        l2 = words[1] + " " + '1' + " " + "FF" + "\n"
        input = l1 + l2
    else:
        words[0] = words[0] + " " + '0' + " " + "FF" + "\n"
        words[1:-2] = map(lambda s: s + " " + "0" + " " + "FF" + "\n", words[1:-2])
        words[-2] = words[-2] + " " + '0' + " " + "FF" + "\n"
        words[-1] = words[-1] + " " + '1' + " " + keepSignal + "\n"
        input = "".join(words)
    return input

def simulationOutput_Rx(hexstream, empty=True):

    lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
    hexstream = hexstream.upper()

    while len(hexstream) % 16 != 0:
        hexstream = hexstream + "00"

    words = split_string_into_len(hexstream, 16)
    words = map(hexstream_reverse_bytes, words)

    if(len(words)==1):
        l2 =  words[0] + " " + '1' + "\n"
        input = l2
    elif(len(words)==2):
        l1 = words[0] + " " + '0' + "\n"
        l2 = words[1] + " " + '1' + "\n"
        input = l1 + l2
    else:
        words[0] = words[0] + " " + '0' + "\n"
        words[1:-2] = map(lambda s: s + " " + "0" + "\n", words[1:-2])
        words[-2] = words[-2] + " " + '0' + "\n"
        words[-1] = words[-1] + " " + '1' + "\n"
        input = "".join(words)
    return input

def simulationInput_Tx(hexstream):
	lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
	requestLength = hex(len(hexstream) / 2)
	requestLength = requestLength[2:].upper()
	while (len(requestLength) % 4) != 0:
        	requestLength = "0" + requestLength
	hexstream = hexstream.upper()
    	firstWordString = destination_port + " " + source_port + " 01010101 0A0B0C0D " + requestLength + " "
    	otherWordString = "0000 0000 00000000 00000000 0000 "
	residue = (len(hexstream) % 16) / 2
        keepSignal = lastDict[residue]
	while len(hexstream) % 16 != 0:
		hexstream = hexstream + "00"

	words = split_string_into_len(hexstream, 16)
	words = map(hexstream_reverse_bytes, words)

	if(len(words)==1):
		l2 =  firstWordString + words[0] + " FF " + '1' + "\n"
		input = l2
	elif(len(words)==2):
		l1 = firstWordString + words[0] + " FF " + '0' + "\n"
		l2 = source_ip_hls + " " + words[1] + " FF " + '1' + "\n"
		input = l1 + l2
	else:
		words[0] = firstWordString + words[0] + " FF " + '0' + "\n"
		words[1:-2] = map(lambda s: otherWordString + s + " FF " + '0' + "\n", words[1:-2])
		words[-2] = otherWordString + words[-2] + " FF " + '0' + "\n"
		words[-1] = otherWordString + words[-1] + " " + keepSignal + " 1" + "\n"
		input = "".join(words)

	return input

def simulationOutput_Tx(hexstream, empty=True):

    lastDict = {0 : "FF", 1 : "01", 2 : "03", 3 : "07", 4 : "0F", 5 : "1F", 6 : "3F", 7 : "7F"}
    #hexstream = hexstream.upper()
    requestLength = str((len(hexstream)/2) + 8)
    while len(requestLength) % 4 != 0:
        requestLength = requestLength + "0"

    ipLength = hex(28 + (len(hexstream)/2))
    ipLength = ipLength[2:]
    #print(ipLength)
    while len(ipLength) % 4 != 0:
        ipLength = "0" + ipLength
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    ip_header = "4500" + ipLength + "00000000FF110000010101010A0B0C0D"
    udp_header = destination_port + source_port + udpLength + checksum
    #Calculate checksum for UDP packet. First construct the pseudoheader
    udpLength = hex(8 + (len(hexstream)/2))
    udpLength = udpLength[2:]
    #print(ipLength + " - " + udpLength)
    while (len(udpLength) % 4) != 0:
        udpLength = "0" + udpLength
    checksumPacket = "010101010A0B0C0D" + "00" + "11" + udpLength + udp_header + hexstream
    #print(checksumPacket)
    pseudoLength = len(checksumPacket)
   #binascii.hexlify(b'checksumPacket')
    binascii.hexlify(checksumPacket)
    checksumResults = hex(calculateChecksum(checksumPacket, pseudoLength)) # Checksum result is in integer format now, convert to hex
    checksumResults = checksumResults[2:]
    while (len(checksumResults) % 4) != 0:
        checksumResults = "0" + checksumResults
    udp_header = destination_port + source_port + udpLength + checksumResults

    hexstream = ip_header + udp_header + hexstream
    hexstream = hexstream.upper()
    residue = (len(hexstream) % 16) / 2
    if (residue != 8):
        keepSignal = lastDict[residue]
    else:
        keepSignal = "FF"
    while len(hexstream) % 16 != 0:
        hexstream = hexstream + "00"

    words = split_string_into_len(hexstream, 16)
    words = map(hexstream_reverse_bytes, words)

    if(len(words)==1):
        l2 =  words[0] + " " + '1' + " " + keepSignal + "\n"
        input = l2
    elif(len(words)==2):
        l1 = words[0] + " " + '0' + " " + "FF" + "\n"
        l2 = words[1] + " " + '1' + " " + "FF" + "\n"
        input = l1 + l2
    else:
        words[0] = words[0] + " " + '0' + " " + "FF" + "\n"
        words[1:-2] = map(lambda s: s + " " + "0" + " " + "FF" + "\n", words[1:-2])
        words[-2] = words[-2] + " " + '0' + " " + "FF" + "\n"
        words[-1] = words[-1] + " " + '1' + " " + keepSignal + "\n"
        input = "".join(words)
    return input

# expects list of hexstreams
def rxInput(requests):
	requests = requests[:] # copy of list
	requests[0:-1] = map(lambda r: simulationInput_Rx(r, False), requests[0:-1])
	requests[-1] = simulationInput_Rx(requests[-1], True)
	return "".join(requests)

def rxInput_noCS(requests):
	requests = requests[:] # copy of list
	requests[0:-1] = map(lambda r: simulationInput_Rx_noCS(r, False), requests[0:-1])
	requests[-1] = simulationInput_Rx(requests[-1], True)
	return "".join(requests)

def rxInput_wrongCS(requests):
	requests = requests[:] # copy of list
	requests[0:-1] = map(lambda r: simulationInput_Rx_wrongCS(r, False), requests[0:-1])
	requests[-1] = simulationInput_Rx(requests[-1], True)
	return "".join(requests)

def rxOutput(requests):
	requests = requests[:] # copy of list
	requests[0:-1] = map(lambda r: simulationOutput_Rx(r, False), requests[0:-1])
	requests[-1] = simulationOutput_Rx(requests[-1], True)
	return "".join(requests)

# expects list of hexstreams
def txInput(res):
	res = map(simulationInput_Tx, res)
	return "".join(res)

# expects list of hexstreams
def txOutput(res):
	res = map(simulationOutput_Tx, res)
	return "".join(res)

########################################################################################################

## Helpers for quick testcase scenario setup ###################################

# These functions allow you to specify a series of requests without specifying
# any protocol details. In the end, they produce
#
#   * the specified requests using the BINARY protocol
#   * the specified requests using the ASCII protocol
#   * at both, back-to-back and 1Gbps input rate
#   * the corresponding golden resoponse for automatic verification

def newTestset():
	return {
		'list'   : [],
		'bin_rq' : [],
		'bin_rs' : [],
		'txt_rq' : [],
		'txt_rs' : []
	}

def setSuccess(pair, set):
	set['list'].append("set [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binarySetRequest(pair) )
	set['bin_rs'].append( binarySetResponse(pair) )
	set['txt_rq'].append( textSetRequest(pair) )
	set['txt_rs'].append( textSetResponse(pair) )

def setFail(pair, set):
	set['list'].append("set [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binarySetRequest(pair) )
	set['bin_rs'].append( binaryFailedSetResponse(pair) )
	set['txt_rq'].append( textSetRequest(pair) )
	set['txt_rs'].append( textFailedSetResponse(pair) )		# Not supported at this stage

def getSuccess(pair, set):
	set['list'].append("getSuccess [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binaryGetRequest(pair) )
	set['bin_rs'].append( binaryGetResponse(pair) )
	set['txt_rq'].append( textGetRequest(pair) )
	set['txt_rs'].append( textGetResponse(pair) )

def getFail(pair, set):
	set['list'].append("getFail [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binaryGetRequest(pair) )
	set['bin_rs'].append( binaryFailedGetResponse() )
	set['txt_rq'].append( textGetRequest(pair) )
	set['txt_rs'].append( textFailedGetResponse() )

def delete(pair, set):
	set['list'].append("delete [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binaryDeleteRequest(pair) )
	set['bin_rs'].append( binaryDeleteResponse(pair) )
	set['txt_rq'].append( textDeleteRequest(pair) )
	set['txt_rs'].append( textDeleteResponse(pair) )

def deleteFail(pair, set):
	set['list'].append("deleteFail [%d -> %d; %s]: %s -> %s" % (
		len(pair['key']),
		len(pair['value']),
		pair['flags'],
		pair['key'],
		pair['value'] ))

	set['bin_rq'].append( binaryDeleteRequest(pair) )
	set['bin_rs'].append( binaryFailedDeleteResponse(pair) )
	set['txt_rq'].append( textDeleteRequest(pair) )
	set['txt_rs'].append( textFailedDeleteResponse(pair) )

def flush(set):
	set['list'].append("flush")

	set['bin_rq'].append( binaryFlushRequest() )
	set['bin_rs'].append( binaryFlushResponse() )
	set['txt_rq'].append( textFlushRequest() )
	set['txt_rs'].append( textFlushResponse() )
