PIN_PATH=/home/qali/Ins/pin-2.12-54730-gcc.4.4.7-linux/
PIN=$PIN_PATH"pin"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/cacheWithBuffer.so"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/hybridCache.so"
#APP=$1"-"$2$3
#OUTPUT=$APP".hybrid"

APP=$1
PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/qaliTrace.so"
OUTPUT=$APP.trace

#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/SymbolTrace.so"
#OUTPUT=$APP.baseStat

#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/SymbolTraceOpti.so"
#OUTPUT=$APP.optiStat

	if [ $# -lt 1 ]; then
		echo "lack of argument"
		exit
	fi

	DIR=$1
	cd $DIR
	echo "====Into: $(pwd)"	
	
	##echo "$PIN -t $PIN_TOOL -b 32 -o $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -b 32 -o $OUTPUT -- ./$APP >log	
	
	echo "$PIN -t $PIN_TOOL -o $OUTPUT -- ./$APP >log"
	$PIN -t $PIN_TOOL -o $OUTPUT -- ./$APP >log	

	#echo "$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log

	#echo "$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log
	
	
	cd ..
	echo "====Back to: $(pwd)"

	if [ $? -ne 0 ]; then
		exit 
	fi
	
	
