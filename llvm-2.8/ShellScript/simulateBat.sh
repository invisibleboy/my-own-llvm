TOOL=/home/qali/Develop/llvm-2.8/ShellScript/simulate.sh
#TOOL=/home/qali/Develop/llvm-2.8/ShellScript/floatPoint.sh

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
	

	echo "${TOOL} $DIR $1"
	${TOOL} $DIR $1
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	cd ..
done
