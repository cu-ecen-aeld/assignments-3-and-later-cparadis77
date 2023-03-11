#!/bin/sh
#
# Small script that is looking for files in specified folders (and recursively in sub folders)
#
#

# Check the number of parameters
if [ $# -ne 2 ]; then
  echo "Usage: $0 [filesdir] [searchstr]"
  echo
  echo "filesdir	is a path to a directory on the filesystem"
  echo "searchstr	is a text string which will be searched within these files"
  echo 
  exit 1
fi

# Ensure 'filesdir' does represent a directory on the filesystem
if [ ! -d $1 ]; then
  echo "Error: $1 is not a directory"
  exit 1
fi

# Debug only
#echo "Debug: $0, $1, $2"
#echo "Debug: $@"

filesdir=$1
searchstr=$2

# Debug only
#echo "Debug: filesdir=$filesdir"
#echo "Debug: searchstr=$searchstr"

# Find all files in the specified directory
files_found=$(find $filesdir -type f)
num_lines=$(echo "$files_found" | wc -l)

# Debug only
#echo "Debug: files_found=$files_found"
#echo "Debug: num_lines=$num_lines"

# Find all match
#matches_found=$(find $filesdir -type f | grep $searchstr)      # BAD
#matches_found=$(echo $files_found | grep $searchstr)           # BAD
#pushd $filesdir &>/dev/null                                    # BAD can not use it with 'sh' interpreter, but I can with 'bash' intrepreter
                                                                #     For assignment 3 in qemu, I need to use a 'sh' interpreter
cd $filesdir
matches_found=$(grep $searchstr *)
num_matches=$(echo "$matches_found" | wc -l)

#popd &>/dev/null                                               # BAD can not use it with 'sh' interpreter, but I can with 'bash' intrepreter
                                                                #     For assignment 3 in qemu, I need to use a 'sh' interpreter
#Debug only
#echo "Debug: matches_found=$matches_found"
#echo "Debug: num_matches=$num_matches"

# The number of files are X and the number of matching lines are Y
#	where X is the number of files in the directory and all subdirectories
# 	where Y is the number of matching lines found in respective files, where a matching line refers to a line which contains searchstr (and may also contain additional content)

echo "The number of files are $num_lines and the number of matching lines are $num_matches"

