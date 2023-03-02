#!/bin/bash
#
# Small script that create a file and write a string in it
#
#

# Check the number of parameters
if [ $# -ne 2 ]; then
  echo "Usage: $0 [writefile] [writestr]"
  echo
  echo "writefile	is a full path to a file (including filename) on the filesystem"
  echo "writestr	is a text string which will be written within this file"
  echo 
  exit 1
fi

# Debug only
#echo "Debug: $0, $1, $2"
#echo "Debug: $@"


writefile=$1
writestr=$2

directory=$(dirname "$writefile")
filename=$(basename "$writefile")

#Debug only
#echo "Debug: directory=$directory"
#echo "Debug: filename=$filename"

# Create directory if needed
if [ ! -d $directory ]; then
  mkdir -p $directory
fi

# Change directory (cd) into the new directory
pushd $directory &>/dev/null

# Create file if needed
if [ ! -e $filename ]; then
  touch $filename

  if [ $? -ne 0 ]; then
    echo "Failed to create file: $filename"
    exit 1
  fi

fi

# Write a string into a file, note here we do not append into the file, we overwrite the content of the file
echo "$writestr" > $writefile


# Change directory into where the script has been called
popd &>/dev/null

