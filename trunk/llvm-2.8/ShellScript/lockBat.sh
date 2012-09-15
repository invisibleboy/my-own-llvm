#TOOL=/home/qali/ShellScript/llvm-compile.sh
#TOOL=/home/qali/ShellScript/simulate.sh
TOOL=/home/qali/ShellScript/hybridCacheLock.sh

#llvm-compileBat.sh -5 5

if [ $# -lt 3 ]; then
	echo "lack of argument"
	exit
fi

for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi

	#cd $DIR
	echo "Deal with: $DIR"

	echo "${TOOL} $DIR $1 $2 $3"
	${TOOL} $DIR $1	$2 $3
	
	if [ $? -ne 0 ]; then
		echo "Error:"
		exit 
	fi

	#cd ..
done
	
