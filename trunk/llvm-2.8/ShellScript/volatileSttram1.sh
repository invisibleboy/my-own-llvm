#This script is used to evaluate the experiments for volatile stt-ram.



PIN_PATH=/home/qali/Ins/pin-2.12-54730-gcc.4.4.7-linux/
PIN=$PIN_PATH"pin"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/cacheWithBuffer.so"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/hybridCache.so"
#APP=$1"-"$2$3
#OUTPUT=$APP".hybrid"

APP=$1
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/qaliTrace.so"
#OUTPUT=$APP.trace

PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/volTrace2ilp.so"
PIN_TOOL_OPT=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/SymbolTraceOpti.so"
ALLOC=/home/qali/ShellScript/allocate

SWITCH=ilp1

SCHEME=$2
BLOCK_SIZE=$3
CACHE_SIZE=$4
RETENTION=$5
WRITE_LAT=$6

#Only evaluate the first $HEAD writes of the trace
HEAD=400

OPT_RETENTION="-r $RETENTION"
OPT_WRITE_LAT="-wl $WRITE_LAT"
OPT_HEAD="-head $HEAD"

#--volTrace2ilp
ITRACE=$APP.trace
OUTPUT=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE"_"$RETENTION"_"$WRITE_LAT.statBase
OTRACE=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE.traceILP
OGRAPH=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE.graph

#--allocate

DATAMAP=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE.alloc

#--SymbolTraceOpti
OUTPUT_OPT=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE"_"$RETENTION"_"$WRITE_LAT.statOpt
OUTPUT_OPT_ILP=$APP"_"$SCHEME"_"$BLOCK_SIZE"_"$CACHE_SIZE.statOptIlp


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
	
	#--SymbolTrace
	#echo "$PIN -t $PIN_TOOL -hw $SCHEME -it $ITRACE -og $OGRAPH -o $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -hw $SCHEME -it $ITRACE -og $OGRAPH -o $OUTPUT -- ./$APP >log	

	# from trace to graph as well as base result
	#volTrace2Ilp
	echo "$PIN -t $PIN_TOOL -hw $SCHEME -it $ITRACE -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT $OPT_HEAD -ot $OTRACE -og $OGRAPH -o $OUTPUT -- ./$APP >log"
	$PIN -t $PIN_TOOL -hw $SCHEME -it $ITRACE -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT $OPT_HEAD -ot $OTRACE -og $OGRAPH -o $OUTPUT -- ./$APP >log

	
	if [ $SWITCH = "ilp" ]; then		
		#from graph to allocation.result
		#echo "$ALLOC 1 $OTRACE"_funcs" >> log"
		#$ALLOC 1 $OTRACE"_funcs" >> log

		#from allocation.result to optiStat
		echo "$PIN -t $PIN_TOOL_OPT -hw $SCHEME -it $ITRACE -id $DATAMAP -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT -o $OUTPUT_OPT_ILP  -- ./$APP >>log"
		$PIN -t $PIN_TOOL_OPT -hw $SCHEME -it $ITRACE -id $DATAMAP -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT -o $OUTPUT_OPT_ILP  -- ./$APP >>log
	else
		#from graph to heuristic allocation results 
		echo "$ALLOC 0 $OGRAPH >> log"
		$ALLOC 0 $OGRAPH >> log

		#from allocation.result to optiStat
		echo "$PIN -t $PIN_TOOL_OPT -hw $SCHEME -it $ITRACE -id $DATAMAP -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT $OPT_HEAD -o $OUTPUT_OPT  -- ./$APP >>log"
		$PIN -t $PIN_TOOL_OPT -hw $SCHEME -it $ITRACE -id $DATAMAP -b $BLOCK_SIZE -c $CACHE_SIZE $OPT_RETENTION $OPT_WRITE_LAT $OPT_HEAD -o $OUTPUT_OPT  -- ./$APP >>log	
	fi

	#echo "$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log

	#echo "$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log"
	#$PIN -t $PIN_TOOL -it $APP.trace -og $APP.graph -o $OUTPUT -- ./$APP >log
	
	
	cd ..
	echo "====Back to: $(pwd)"

	if [ $? -ne 0 ]; then
		exit 
	fi
	
	
