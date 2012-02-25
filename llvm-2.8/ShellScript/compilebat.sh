

CPPCLAGS=
RUN_OPTIONS=
CC=gcc
COMPILE_TOOL=/home/qali/Tool/compile.sh

for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi

	cd $DIR
	echo "Current in: $(pwd)"

	echo "${COMPILE_TOOL} $DIR "
	${COMPILE_TOOL} $DIR 	
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	cd ..
	

done
	