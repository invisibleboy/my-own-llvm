#TOOL=/home/qali/ShellScript/llvm-compile.sh
#TOOL=/home/qali/ShellScript/run.sh
#TOOL=/home/qali/ShellScript/hybridCacheLock.sh
TOOL=/home/qali/ShellScript/volatileSttram1.sh
#llvm-compileBat.sh -5 5

if [ $# -lt 0 ]; then
	echo "lack of argument"
	exit
fi

for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." -o ! -d $DIR ]; then
		echo $DIR
		continue
	fi

	#cd $DIR
	echo "Deal with: $DIR"

	echo "${TOOL} $DIR $1 $2 $3 $4 $5"
	${TOOL} $DIR $1	$2 $3 $4 $5
	
	if [ $? -ne 0 ]; then
		echo "Error:"
		exit 
	fi

	#cd ..
done
	
