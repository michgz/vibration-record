# Run with Python 2.7
##


import zipfile
import shutil
import numpy
import os
import sys
import datetime
import getopt







######################################################################
##    START MAIN PROGRAM    ##
######################################################################


 
 
def dosage_spreadsheet(*args, **kwargs):
    with open("out.csv", "w") as f1:
        f1.write("Hello\r\n")





#######################################
# Now handle the command-line syntax.
#######################################





def usage():
    print "-h  help"
    print "-o  output"


def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "ho:", ["help", "output="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print str(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    Output = None
    Long = False
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-o", "--output"):
            Output = a
        else:
            assert False, "unhandled option"
    if (Output == None):
        # Have no output, cannot do anythin
        sys.exit(3)
    postproc_spreadsheet(args, output=Output)


if __name__=="__main__":
    main()

