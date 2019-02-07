# Run with Python 2.7
##


import os
import sys
import datetime
import getopt

from postproc_spreadsheet import postproc_spreadsheet


######################################################################
##    START MAIN PROGRAM    ##
######################################################################


def usage():
    print "-h  help"
    print "-o  output"
    print "-s  short-form output"
    print "-l  long-form output"


def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "ho:sl", ["help", "output=", "short", "long"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print str(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    Output = None
    Long = False
    for o, a in opts:
        if o in ("-l", "--long"):
            Long = True
        elif o in ("-s", "--short"):
            # Nothing to do - use defaults
            pass
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-o", "--output"):
            Output = a
        else:
            assert False, "unhandled option"
    if (Output == None):
        # Have no output. Determine the output path and output file name based on the first argument.
        if (len(args) < 1):
            sys.exit(4)
        
        thePath = args[0]

        if not os.path.exists(thePath):
            sys.exit(3)

        if os.path.isdir(thePath):
            # It's a directory. Use its parent for output

            thePath = os.path.abspath(thePath)

            if Long:
                Output = os.path.dirname(thePath) + "/" + os.path.basename(thePath) +       "_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"
            else:
                Output = os.path.dirname(thePath) + "/" + os.path.basename(thePath) + "_Short_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"

        else:
            # It's a file. Use its parent as both the directory for input files
            # and for output files.

            thePath = os.path.abspath(thePath)

            if Long:
                Output = os.path.dirname(thePath) + "/" +            datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"
            else:
                Output = os.path.dirname(thePath) + "/" + "Short_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"

    # Go through all inputs adding on the found CSV files

    inputFiles = []

    for z in args:
        if os.path.isdir(z):
            inputFilesBare = sorted([y for y in os.listdir(z) if y.upper().endswith('.CSV')])
            for u in inputFilesBare:
                inputFiles.append(os.path.join(os.path.abspath(z), u))
        else:
            if z.upper().endswith('.CSV'):
                inputFiles.append(os.path.abspath(z))


    postproc_spreadsheet(*inputFiles, long=Long, output=Output)


if __name__=="__main__":
    main()
