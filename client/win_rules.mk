#CROSS-COMPILATION SCRIPT
#Pick just one, and only one, compiler to use; pick none to use system gcc
CCW32=0
CCW64=0
CCW32BE=1
CCW64BE=0

if [ $CCW32 = 1 ]; then
CCPATH="~/mw32/bin/:"
PREFIX="i686-w64-mingw32-"
SIZEOPT="-m32"

elif [ $CCW64 = 1 ]; then
CCPATH="~/mw64/bin/:"
PREFIX="x86_64-w64-mingw32-"
SIZEOPT="-m64"

elif [ $CCW32BE = 1 ]; then
CCPATH="/usr/i686-w64-mingw32/bin/:"
PREFIX="i686-w64-mingw32-"
SIZEOPT="-m32"

elif [ $CCW64BE = 1 ]; then
CCPATH="/usr/x86_64-w64-mingw32/bin/:"
PREFIX="x86_64-w64-mingw32-"
SIZEOPT="-m64"

else
CCPATH=""
PREFIX=""
SIZEOPT=""
fi

#Add path to cross-compiler at beginning of PATH
PATH=$CCPATH$PATH #Works for all compiler calls within the shell of this script
#export PATH  #Will not take unless you are root but may be useful if you call other scripts from this one

#Get information on versions
$PREFIX"gcc" --version
$PREFIX"gcc" -dumpmachine

#Use the compiler
$PREFIX"gcc" -static $SIZEOPT client.c ../win_lib.c -Wall -pedantic -lws2_32 -o client.exe


#PATH will revert to the system value when the script exits
