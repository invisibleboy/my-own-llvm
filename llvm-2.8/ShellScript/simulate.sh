PIN_PATH=/home/qali/Ins/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/
PIN=$PIN_PATH"pin"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/cacheWithBuffer.so"
PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/hybridCache.so"
APP=$1"-"$2$3
OUTPUT=$APP".hybrid"

	if [ $# -lt 3 ]; then
		echo "lack of argument"
		exit
	fi

	DIR=$1
	cd $DIR
	echo "====Into: $(pwd)"	
	
	echo "$PIN -t $PIN_TOOL -b 32 -o $OUTPUT -- ./$APP >log"
	$PIN -t $PIN_TOOL -b 32 -o $OUTPUT -- ./$APP >log	
	
	
	cd ..
	echo "====Back to: $(pwd)"

	if [ $? -ne 0 ]; then
		exit 
	fi
	
	
