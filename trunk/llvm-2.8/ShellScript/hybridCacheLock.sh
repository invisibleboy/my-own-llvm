PIN_PATH=/home/qali/Ins/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/
PIN=$PIN_PATH"pin"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/cacheWithBuffer.so"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/hybridCacheLock.so"
PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/FuncExecFreq.so"
#APP=$1"-"$2$3
APP=$1"-00"
#ChoiceN=25
#OUTPUT=$APP".lockStat"$ChoiceN
OUTPUT=$APP".freq"

	if [ $# -lt 1 ]; then
		echo "lack of argument"
		exit
	fi
	#if [ $# -eq 4 ]; then
	#	ChoiceN=$4
	#	OUTPUT=$APP".lockStat"$ChoiceN
	#fi

	DIR=$1
	cd $DIR
	echo "====Into: $(pwd)"	
	
	echo "$PIN -t $PIN_TOOL -lf $APP.lock -o $OUTPUT -- ./$APP >log"
	$PIN -t $PIN_TOOL -lf $APP.lock -o $OUTPUT -- ./$APP >log	
	
	
	cd ..
	echo "====Back to: $(pwd)"

	if [ $? -ne 0 ]; then
		exit 
	fi
	
	
