#!/usr/bin/python

import getopt
import socket
import sys
import datetime
import threading

def sender(s, pattern, max_length, iteration):
    start = datetime.datetime.now()
    str_send = ''
    for i in range(max_length):
        str_send += '5'
    for j in range(iteration):
        #print "Send start: Iteration ",  j
        s.sendall( str_send )
            #print "send: %s" % str_send
    stop = datetime.datetime.now()
    diff = stop - start
    bandwidth = max_length*iteration*1.0/(diff.total_seconds()*1048576)
    print "Testing bandwidth: %.2f MB/s" % bandwidth

def receiver(s, pattern, max_length, iteration):
    for j in range(iteration):
        recv_data = s.recv(max_length)
            #print "Recv: %s" % recv_data
        if (j&0xffff == 0) or (j > iteration-30):
            print "Received end: Iteration ", j


def usage():
    print str(sys.argv[0]), '[--ip=1.1.1.1] [--port=5001] [--pattern=1] [--max_length=9] [--iteration=2] [-h] [-u]'

def main():
    try:
        opts, args = getopt.getopt( sys.argv[1:], 'hu', ["ip=", "port=", "pattern=", "max_length=", "iteration="])
    except getopt.GetoptError as err:
        print str(err)
        usage()
        sys.exit(2)

    hostIPAddr = '1.1.1.1'
    port        = 640
    max_length  = 100
    iteration   = 2
    testUdp     = True
    pattern     = 1
    for (o, a) in opts:
        if o == "--ip":
            hostIPAddr = a
        elif o == "--port":
            port = int(a)
        elif o == "--pattern":
            pattern = int(a)
        elif o == "--max_length":
            max_length = int(a)
        elif o == "--iteration":
            iteration = int(a)
        elif o == "-h":
            usage()
            sys.exit()
        elif o == "-u":
            testUdp = True

    print "Testing Parameters:"
    print "\tip = ", hostIPAddr
    print "\tport = ", port
    print "\tpattern = ", pattern
    print "\tmax_length = ", max_length
    print "\titeration = ", iteration
    print "\ttestUdp = ", testUdp

    if testUdp == True:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    else:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect( (hostIPAddr, port) )

    sendThread = threading.Thread(target=sender, args=(s, pattern, max_length, iteration))
    receiverThread = threading.Thread(target=receiver, args=(s, pattern, max_length, iteration))

    sendThread.start()
    receiverThread.start()

    sendThread.join()
    receiverThread.join()

if __name__ == '__main__':
    main()



