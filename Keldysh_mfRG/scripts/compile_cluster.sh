#!/bin/bash

# Shell script for the compilation process on the ASC-cluster, the KCS-cluster and the SuperMUC
# Usage: ...

# check option and load necessary modules
if [ "$1" == "--ASC" ]
  then
  export CLUSTER=ASC
  module load cmake
elif [ "$1" == "--KCS" ]
then
  export CLUSTER=KCS
  module unload gcc # in case some old version has been loaded previously
  module load gcc/9
  module unload hdf5 # in case some old version has been loaded previously
  module load hdf5/1.8.20-cxx-frt-threadsafe
  module load fftw
  module load gsl
  module load boost/1.61_icc
  module load cmake
elif [ "$1" == "--JSC" ]
then
  export CLUSTER=JSC
  module load Stages/2022 Intel/2021.4.0 OpenMPI/4.1.2
  module load HDF5/1.12.1 # could be problematic, because already version 1.10 (instead of 1.08) and no threadsafe option
  module load FFTW
  module load GSL
  module load Boost
  module load CMake/3.21.1
else
  echo "Invalid cluster option! Needs to be --ASC, --KCS or --JSC"
  exit
fi

export LANG=C
export LC_ALL=C

dir_path=$(dirname "$(realpath "$0")")    # directory of this script
src_dir="$dir_path"/..                    # source directory
build_dir="$src_dir"/build                # build directory
mkdir "$build_dir"

# navigate to the directory with the CMakeLists.txt file
cd "$build_dir" || exit


# load cmake project
cmake -DWORKSTATION=OFF -D$CLUSTER=ON -S$src_dir -B$build_dir


# build
TARGET=Keldysh_mfRG
cmake --build . --target $TARGET -j6

mv ./$TARGET $src_dir

GREEN='\033[1;32m' # green
NC='\033[0m' # no color
echo -e "\n Moved executable ${GREEN}$TARGET ${NC}to Keldysh_mfRG source directory.\n"