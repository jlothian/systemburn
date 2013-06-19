#!/usr/bin/python
"""
  This file is part of SystemBurn.

  Copyright (C) 2012, UT-Battelle, LLC.

  This product includes software produced by UT-Battelle, LLC under Contract No. 
  DE-AC05-00OR22725 with the Department of Energy. 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the New BSD 3-clause software license (LICENSE). 
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
  LICENSE for more details.

  For more information please contact the SystemBurn developers at: 
  systemburn-info@googlegroups.com
"""

import re
import shlex
from time import sleep
from optparse import OptionParser
import subprocess
import sys
MEM_PERCENT = 90
MON_FREQ = 5
NORM_TIME = 300
TEMPLATE = "load.tpl"
CPU_LOADS = ["PV1", "PV2", "PV3", "PV4", "FFT1D", "FFT2D", "RDGEMM", "DSTREAM", "LSTREAM", "ISORT", "TILT", "CBA"]
OTHER_LOADS = ["GUPS"]
EXTRA_SLOTS = 4
test_runtime = 300

class SBload:
    def __init__(self):
        self.name = ""
        self.body = ""
        self.vals = []
        self.minval = -1
        self.acgval = -1
        self.maxval = -1
        self.filename = ""
        self.hasrun = False
    def __str__(self):
        s = ""
        s += "\nbody    : "+self.body
        return s
    def __rep__(self):
        return str(self)

    def write(self,filename):
        f = open(filename,"w")
        f.write(self.body)
        f.close()

    def write_results(self, filename):
        f = open(filename, "w")
        f.write("Max: "+str(self.maxval))
        f.write("\nAvg: "+str(self.avgval))
        f.write("\nMin: "+str(self.minval))
        f.write("\nRaw: "+str(self.vals))
        f.write("\nLoad:\n"+self.body)
        f.close()


def check_output(*popenargs, **kwargs):
        r"""Run command with arguments and return its output as a byte string.

        Backported from Python 2.7 as it's implemented as pure python on stdlib.
        """
        process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
        output, unused_err = process.communicate()
        retcode = process.poll()
        if retcode:
            cmd = kwargs.get("args")
            if cmd is None:
                cmd = popenargs[0]
                error = subprocess.CalledProcessError(retcode, cmd)
                error.output = output
                raise error
        return output


#LOADFILES = [ "test.1", "test.2", "test.3", "test.4", "test.5" ]

def main():
    parser = OptionParser()
    parser.add_option("-c", "--check-command",
                      dest="cmnd", 
                      help="command to check power/temp usage")
    parser.add_option("-l", "--l2-cachesize",
                      dest="l2_cachesize",
                      help="L2 cache size")
    parser.add_option("-s", "--sockets",
                      dest="sockets",
                      help="number of sockets")
    parser.add_option("-p","--cores-per-socket",
                      dest="cores_per",
                      help="numbers of cores per socket")

    (options, args) = parser.parse_args()

    print args 

    cmnd = options.cmnd
    l2_cachesize = options.l2_cachesize

    cmnd_args = shlex.split(cmnd)
    averages = {}
    maximums = {}

    loads = {}

    for fname in args:
        s = SBload()
        s.filename = fname
        s.name = fname
        loads[fname] = s

    print loads 

    for l in loads:
        load = loads[l]
        f = load.filename
        readings = []
        print "Running on load: "+f
        burn = subprocess.Popen(["./systemburn.wrapper", f])
        while burn.poll() != 0:
            readings.append(float(check_output(cmnd)))
            sleep(MON_FREQ)

        print "Done running load: "+str(f)

        # now, get a total
        load.avgval = sum(readings)/len(readings)
        load.maxval = max(readings)
        load.minval = min(readings)
        
        load.vals = readings
        load.write_results(load.filename+".results")
        sleep(NORM_TIME)


    for l in loads:
        load = loads[l]
        print "load: "+load.name+" filename: "+load.filename+" avg: "+str(load.avgval)+" max: "+str(load.maxval)

if __name__ == "__main__":
            main()
