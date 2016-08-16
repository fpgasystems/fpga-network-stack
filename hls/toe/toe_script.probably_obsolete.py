import os
import sys
import subprocess
import filecmp



testfile = '/home/rcarley/toe/hls/toe/toe_prj/solution2/ctest.tcl'
number_of_tests = 5
projectname = 'toe_prj'
##projectname = '/home/rcarley/toe/hls/toe/toe_prj/'

vivado_path = "vivado_hls"
# infile = "in_toe.dat"
# outfile = "out_toe.dat"

curr_path = os.getcwd()
##curr_dir = os.getcwd().split(os.sep)[-1]

dir = curr_path+'/'
io_dir = '/home/rcarley/toe/tools/tcpPacketGenerate/'
# /home/rcarley/toe/tools/tcpPacketGenerate/in_ooo1.dat /home/rcarley/toe/tools/tcpPacketGenerate/rx_ooo1.dat /home/rcarley/toe/tools/tcpPacketGenerate/tx_ooo1.dat


def run_ctest(testNumber,ooo):
  testNumber = str(testNumber)
  retcode = 0;

  # if os.path.exists(testNumber+'.out'):
  # 	os.remove(testNumber+'.out')
  # testfile = dir+projectname+'/solution2/ctest.tcl'
  f = open(testfile, 'w')
  f.write('open_project '+projectname+'\n')
  f.write('open_solution "solution2"\n')
  # set_part {xc7vx690tffg1761-2}
  # create_clock -period 6.4 -name default
  # set_clock_uncertainty 0.83


  if (ooo == 0):
    stringOOO = ""
    sys.stdout.write("Running in-order test \""+testNumber+"\" \n")
  else:
    stringOOO = "_ooo"
    sys.stdout.write("Running OOO test \""+testNumber+"\" \n")
  sys.stdout.flush()
  

  inputFileName     =io_dir+"in"+stringOOO+testNumber+".dat "
  rxOutputFileName  =io_dir+"rx"+stringOOO+testNumber+".dat "
  txOutputFileName  =io_dir+"tx"+stringOOO+testNumber+".dat "
  goldRxFileName    =io_dir+"rx"+stringOOO+testNumber+".gold "
  goldTxFileName    =io_dir+"tx"+stringOOO+testNumber+".gold "


  # f.write('csim_design -argv {'+io_dir+'in'+stringOOO+testNumber+'.dat '+io_dir+'rx'+stringOOO+testNumber+'.dat '+io_dir+'tx'+stringOOO+testNumber+'.dat '+io_dir+'rx'+stringOOO+testNumber+'.gold} -clean\n')
  f.write('csim_design -argv {'+ inputFileName + rxOutputFileName + txOutputFileName + goldRxFileName +'} -clean\n')
  f.write('exit\n')
  f.close()
  DEVNULL = open('test.log', 'a')
  # subprocess.call([vivado_path, "-f", testfile], stdout=DEVNULL, stderr=subprocess.STDOUT)
  # print('csim_design -argv {'+ inputFileName + rxOutputFileName + txOutputFileName + goldRxFileName +'} -clean\n')
  
  # Uncomment this:::
  retcode = subprocess.call([vivado_path, "-f", testfile], stdout=DEVNULL, stderr=subprocess.STDOUT)
  DEVNULL.close()

  if(retcode == 0):
  #   print("Test Successful")
  # else:
  #   print("Test Unsuccessful")
    print '\033[32m[Test Successful]\033[0m'
  else:
    print '\033[31m[Test Unsuccessful]\033[0m'
  

  # if os.path.exists(testname+'.out'):
  # 	diff = filecmp.cmp(testname+'.out', testname+'.gold')
  # else:
  # 	diff = False
  # for i in range(len(testname), 50):
  # 	sys.stdout.write(' ')
  # if diff:
  # 	print '\033[32m[PASSED]\033[0m'
  # else:
  # 	print '\033[31m[FAILED]\033[0m'


# def test_dir(dir, projectname):
# 	print "Testing project \""+dir+"\"."
# 	for filename in os.listdir(dir):
# 		if (len(filename) > 5) and (filename[-5:] == ".gold"):
# 			#print "valid gold file: "+filename
# 			if os.path.exists(filename[:-5]+".in"):
# 				run_ctest(filename[:-5], dir+'/', projectname)

def main():
  print ('Testing TOE Project')
  # run_ctest(0) # run base test
  for testNumber in range(5,(number_of_tests+1)):
    run_ctest(testNumber,0)
    run_ctest(testNumber,1)

	# for filename in os.listdir(dir):
	# 	if filename[] and (filename[-5:] == ".gold"):
	# 		#print "valid gold file: "+filename
	# 		if os.path.exists(filename[:-5]+".in"):
	# 			run_ctest(filename[:-5], dir+'/', projectname)

	# valid_dir = False
	# for filename in os.listdir(os.getcwd()):
	# 	if filename == curr_dir+"_proj":
	# 		valid_dir = True
	# 		test_dir(os.getcwd(), filename)
	# 		break
	# if not valid_dir:
	# 	print "The directory \""+curr_dir+"\" is not valid"
	# 	sys.exit()

	#subprocess.call([vivado_path, "-f", solution_dir+"blub.tcl", "-tclargs", "blubl.in"])


if __name__ == '__main__':main()
