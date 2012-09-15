TOOL=/home/qali/ShellScript/llvm-compile.sh
#TOOL=/home/qali/ShellScript/simulate.sh

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

	#cd $DIR
	echo "Deal with: $DIR"

	echo "${TOOL} $DIR $1 $2"
	${TOOL} $DIR $1	$2
	
	if [ $? -ne 0 ]; then
		echo "Error:"
		exit 
	fi

	#cd ..
done
	
