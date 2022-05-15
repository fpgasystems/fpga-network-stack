/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#include <iostream>
#include <random>
#include <string>
#include <fstream>
#include <math.h>


//Default values
//block size: 2
int main(int argc, char *argv[])
{
   if (argc != 5)
   {
      std::cerr << "ERROR number of arguments not correct\n";
      std::cerr << "Usage generate_random_table <number of tables> <block size> <length of key> <length of hash>\n";

      return -1;
   }

   int numTables = atoi(argv[1]);
   int blockSize = atoi(argv[2]);
   int keyLength = atoi(argv[3]);
   int hashLength = atoi(argv[4]);
   std::string fileName("tabulation_table.txt");
   std::ofstream outfile;
   outfile.open(fileName.c_str());
   if(!outfile)
   {
      std::cerr << "Could not open output file";
      return -1;
   }

   uint32_t maxKeyValue = pow(keyLength, 2) - 1;
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution(0, maxKeyValue);

   for (int i = 0; i < numTables; i++)
   {
      outfile << "{";
      for (int j = 0; j < blockSize; j++)
      {
         outfile << "{";
         for (int k = 0; k < keyLength; k++)
         {
            int randomNumber = distribution(generator);
            outfile << randomNumber;
            if (k < keyLength-1)
               outfile << ",";
         }
         outfile << "}";
         if (j < blockSize-1)
            outfile << ",";
      }
      outfile << "}";
      if (i < numTables-1)
         outfile << "," << std::endl;
   }

   outfile.close();
   std::cout << "Table succesfully generated" << std::endl;
   return 0;
}
