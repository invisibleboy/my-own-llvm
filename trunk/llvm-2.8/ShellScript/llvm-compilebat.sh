CPPCLAGS=
RUN_OPTIONS=
BIN_PATH="/home/qali/Build/llvm-2.8/Debug+Asserts/bin/"
LLC=${BIN_PATH}"llc"
LLVM_LINK=${BIN_PATH}"llvm-link"
CC=gcc
TOOL=/home/qali/Develop/llvm-2.8/ShellScript/llvm-compile.sh

#llvm-compileBat.sh -5 5

if [ $# -lt 2 ]; then
	echo "lack of argument"
	exit
fi

for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi

	cd $DIR
	echo "Current in: $(pwd)"

	echo "${TOOL} $DIR $1 $2"
	${TOOL} $DIR $1	$2
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	cd ..
	

done
	
