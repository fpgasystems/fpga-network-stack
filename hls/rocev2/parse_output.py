"""
This program parse the input file and filter according to user input
The program filters by module name and INSTID
User input by either stdin or a conf file which is in json format

The program is very fault-intolerant and also ignores all the lines that does not start with '['

Example conf file:
{
    "input": "./build/vitis_hls.log",
    "output": "output.log",
    "module": ["PROCESS RETRANSMISSION", "RX EXH FSM", "RX IBH FSM", "LOCAL REQ HANDLER"],
    "instid": [-1, 0, -1, 1],
    "bymodule": false
}
"""

import bisect
import sys
import json

def print_array(array, file = None):
    for x in array:
        if x["instid"] == -1:
            print("[{}]: {}".format(x["module"], x["msg"]))
            if file is not None:
                file.write("[{}]: {}\n".format(x["module"], x["msg"]))
        else:
            print("[{} {}]: {}".format(x["module"], x["instid"], x["msg"]))
            if file is not None:
                file.write("[{} {}]: {}\n".format(x["module"], x["instid"], x["msg"]))

def main():
    conf = None
    if len(sys.argv) > 1:
        # read in conf file
        f = open(sys.argv[1])
        conf = json.load(f)
        print(conf)
        f.close()

    if conf is None:
        file = input("input file? ")
        if file == "":
            file = "./build/vitis_hls.log"
    else:
        file = conf["input"]
        
    f = open(file, "r")

    file_out = input("output file? ") if conf is None else conf["output"]

    # for line in f:
    #     if "Generating csim.exe" in line: # start actual log
    #         break
    
    # data structure to store output
    # module, instid, msg, payload
    output = []
    # to store available modules and corresponding INSTIDs
    output_type = {}

    # parse file
    for line in f:
        if line[0] != "[":
            continue    # ignore unformatted log for now

        line = line[1:] # pop front
        index = 0

        # find module name
        for i, c in enumerate(line):
            if c.isdigit() or c == "]":
                index = i
                break
        module = line[:index] if line[index] == "]" else line[:index-1]
        line = line[index:]

        # find instid
        instid = 0
        if line[0] == "]":
            instid = -1  # no instid
        else:
            for i, c in enumerate(line):
                if not c.isdigit():
                    index = i
                    break
            instid = int(line[:index])
        
        # get msg
        for i in range(len(line) - 3):
            if line[i:i+3] == "]: ":
                index = i+2
        msg = line[index:].strip()

        if module not in output_type:
            # add new type in list
            output_type[module] = [instid] if instid != -1 else []
        else:
            # add instid if necessary
            if instid != -1 and instid not in output_type[module]:
                bisect.insort(output_type[module], instid)

        output.append({"module": module, "instid": instid, "msg": msg})

    f.close()

    # choose modules
    output_array = output.copy()
    for x in output_type:
        if conf is None:
            while True:
                instids = output_type[x]
                rsp = input("{}? (y/n/{}) ".format(x, instids))
                if rsp in ["Y","y","yes","Yes"]:
                    break
                elif rsp in ["N","n","no","No"]:
                    output_array = [y for y in output_array if y["module"] != x]
                    break
                elif int(rsp) in instids:
                    # filter by instid
                    output_array = [y for y in output_array if ( y["module"] != x or y["instid"] == int(rsp) )]
                    break
        else:
            if x not in conf["module"]:
                output_array = [y for y in output_array if y["module"] != x]
            elif conf["instid"][conf["module"].index(x)] != -1:
                # filter by instid
                output_array = [y for y in output_array if ( y["module"] != x or y["instid"] == conf["instid"][conf["module"].index(x)] )]


    # print output into log
    print("Finish parsing, printing output to log")
    if file_out != "":
        f = open(file_out,"w")
    else:
        f = None

    if conf is None:
        while True:
            rsp = input("Print by module? (y/n) ")
            if rsp in ["Y","y","yes","Yes"]:
                by_type = True
                break
            elif rsp in ["N","n","no","No"]:
                by_type = False
                break
    else:
        by_type = conf["bymodule"]
        

    if by_type:
        for type in output_type:
            tmp_array = [y for y in output_array if y["module"] == type]
            print_array(tmp_array,f)
    else:
        print_array(output_array,f)

if __name__ == '__main__':
    main()