#!/usr/bin/python

import getopt
import socket
import sys
import datetime
import threading

MAX_LENGTH = 4096

#HOST = '1.1.2.20'
#HOST = '1.1.1.1'
#HOST = '172.16.0.14'
def length_test(hostIPAddr, threadId):
    HOST = hostIPAddr
    PORT = 7

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect( (HOST, PORT) )

    str_send = ''
    char = str( threadId % 10 )
    for i in range(535):
        #print 'Thread', threadId, 'Testing length', i
        #generating outgoing string
        str_send += char
        #if char == 9:
        #    char = 0
        #else:
        #    char += 1

        start = datetime.datetime.now()
        #print( str_send )
        s.sendall( str_send )

        #receiving data
        recv_bytes = 0
        recv_data = ''
        data = ''
        while recv_bytes != i+1:
            data = s.recv(MAX_LENGTH)
            recv_bytes += len( data )
            recv_data += data
            if recv_bytes > i+1:
                print 'Thread %d ERROR: Receiving unexpected data, i = %d, recv_bytes = %d' % (threadId, i, recv_bytes)
                s.close()
                sys.exit()
        #verifying data
        if recv_data != str_send:
            print 'Thread %d ERROR: Data error in length %d: \nstr_send:\n%s \nrecv_data: \n%s' % (threadId, i,  str_send, recv_data)
            s.close()
            sys.exit()
        stop = datetime.datetime.now()
        diff = stop - start
        #print diff.microseconds, 'us'
    print 'Thread %d Exiting...' % (threadId)
    s.close()

def usage():
    print str(sys.argv[0]), '[--ip=1.1.1.1] [--numThread=2] [-h]'

def main():
    try:
        opts, args = getopt.getopt( sys.argv[1:], 'h', ["ip=", "numThread="] )
    except getopt.GetoptError as err:
        print str(err)
        usage()
        sys.exit(2)

    hostIPAddr = '1.1.1.1'
    numThread = 2
    for (o, a) in opts:
        if o == "--ip":
            hostIPAddr = a
        elif o == "--numThread":
            numThread = int(a)
        elif o == "-h":
            usage()
            sys.exit()
#    while 1:
        #print hostIPAddr, numThread
    threads = []
    for i in range(numThread):
        t = threading.Thread(target=length_test, args=(hostIPAddr, i) )
        threads.append(t)

    start = datetime.datetime.now()
    for i in range(numThread):
        threads[i].start()

    for i in range(numThread):
        threads[i].join()
    stop = datetime.datetime.now()
    diff = stop - start
    print numThread, 'threads run for', diff.seconds, 's'

if __name__ == '__main__':
    main()
