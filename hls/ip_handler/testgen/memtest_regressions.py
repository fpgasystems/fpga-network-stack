#!/usr/bin/python

import memlib

## EDIT HERE ###################################################################

keySizes = [1,3,4,5,7,8,9,10,12,13,15,16,17,24,25,28,84,128]
#valueSizes = [1,3,4,5,8,9,10,12,13,16,17,24,28,184,208,1024]
valueSizes = [1,3,4,5,8,9,10,12,13,16,17,24,28,184,208, 256]

seq1repeat = 5

keyChars = map(chr, range(97, 123))
valueChars = map(chr, range(65, 91))

## EDIT FINISHED ###############################################################

DEBUG_SEQUENCES = False
PRINT_SEQUENCES = True

################################################################################

if DEBUG_SEQUENCES:
	keySizes = [1,2,3]
	valueSizes = [1,2]

def pair2kvpair(pair):
	return memlib.kv_pair(pair[0], pair[1], "EFBEADDE", 42)

def seq1(keys, values, repeat):
	if PRINT_SEQUENCES:
		print("--- SEQUENCE 1 repeat %-3s -----------------------------------------------------" % repeat)
	kv_pairs = []
	for key in keys:
		for value in values:
			kv_pairs.append( memlib.kv_pair(key, value, "EFBEADDE", 42) )

	requests = []
	responses = []
	for kv_pair in kv_pairs:
		if PRINT_SEQUENCES:
			print("Set [%d -> %d]: %s -> %s" % (len(kv_pair['key']), len(kv_pair['value']), kv_pair['key'], kv_pair['value']))
		requests.append( memlib.binarySetRequest( kv_pair , "00000000" ) )
		responses.append( memlib.binarySetResponse( kv_pair, "00000000" ) )

		for _ in range(repeat):
			if PRINT_SEQUENCES:
				print("Get [%d -> %d]: %s -> %s" % (len(kv_pair['key']), len(kv_pair['value']), kv_pair['key'], kv_pair['value']))
			requests.append( memlib.binaryGetRequest( kv_pair , "00000000"  ) )
			responses.append( memlib.binaryGetResponse( kv_pair , "00000000" ) )

	return (requests, responses)

def seq2(keys, values):
	if PRINT_SEQUENCES:
		print ("--- SEQUENCE 2 -----------------------------------------------------------------")
	requests = []
	responses = []
	for _ in range(len(values)):
		# for more keys than values, duplicate use of values
		values_used = values
		if len(keys) > len(values):
			while(len(keys) > len(values_used)):
				values_used = values_used + values
			values_used = values_used[0:len(keys)]

		# requests
		kv_pairs = map(pair2kvpair, zip(keys, values_used))
		for kv_pair in kv_pairs:
			if PRINT_SEQUENCES:
				print("Set [%d -> %d]: %s -> %s" % (len(kv_pair['key']), len(kv_pair['value']), kv_pair['key'], kv_pair['value']))
			requests.append( memlib.binarySetRequest(kv_pair, "00000000") )
			responses.append( memlib.binarySetResponse(kv_pair, "00000000") )
		for kv_pair in kv_pairs:
			if PRINT_SEQUENCES:
				print("Get [%d -> %d]: %s -> %s" % (len(kv_pair['key']), len(kv_pair['value']), kv_pair['key'], kv_pair['value']))
			requests.append( memlib.binaryGetRequest(kv_pair, "00000000") )
			responses.append( memlib.binaryGetResponse(kv_pair, "00000000") )

		# rotation
		values = values[1:] + values[0:1]

	return (requests, responses)

################################################################################

if len(keySizes) > len(keyChars):
	sys.exit("Error: Not enough key characters.")
if len(valueSizes) > len(valueChars):
	sys.exit("Error: Not enough value characters.")

keyPairs = zip(keySizes, keyChars)
valuePairs = zip(valueSizes, valueChars)

keys = map(lambda (size, char): char * size, keyPairs)
values = map(lambda (size, char): char * size, valuePairs)

SEQ1 = seq1(keys, values, seq1repeat)

# SEQ1
reqRx = open("rxInput.dat", "w")
reqRx.write( memlib.rxInput(SEQ1[0]) )
reqRx.close()
respRx = open("rxGoldenOutput.dat", "w")
respRx.write( memlib.rxOutput(SEQ1[0]) )	# Once for the input stream with checksum
respRx.close()
