
if [ $# -lt 1 ]; then
	echo "lack of argument"
	exit
fi

APP=$1

for DIR in *; do 
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi

	cd $DIR
	echo "Current in: $(pwd)"

	echo "time ./$APP >>timer.log"
	time ./$APP >>timer.log
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	cd ..
	

done
	
