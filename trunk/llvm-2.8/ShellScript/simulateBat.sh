TOOL=/home/qali/Develop/llvm-2.8/ShellScript/simulate.sh

if [ $# -lt 1 ]; then
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
	
	DIR=main

	echo "${TOOL} $DIR $1 $2"
	${TOOL} $DIR $1	$2
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	cd ..
done
