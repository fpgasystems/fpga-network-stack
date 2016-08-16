import binascii

def split_string_into_len(str, size=8):
	return [ str[i:i+size] for i in range(0, len(str), size) ]

def hexstream_reverse_bytes(hexstream):
	bytes = split_string_into_len(hexstream, 2)
	reversed_bytes = bytes[::-1]
	return "".join(reversed_bytes)

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

ip_header = "37363534333231302f2e2d2c2b2a292827262524232221201f1e1d1c1b1a1918171615141312111000000000000d188a0000000051cd55b80100b463b7b80008"
#ip_header = "0ADF0024FA"
print(ip_header)
ip_header = hexstream_reverse_bytes(ip_header)
ip_header = ip_header.upper()
binascii.hexlify(ip_header)
print(ip_header)
cs = hex(calculateChecksum(ip_header, len(ip_header)))[2:]
print(cs)