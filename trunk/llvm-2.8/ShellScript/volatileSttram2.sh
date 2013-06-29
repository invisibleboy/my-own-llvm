PIN_PATH=/home/qali/Ins/pin-2.12-54730-gcc.4.4.7-linux/
PIN=$PIN_PATH"pin"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/cacheWithBuffer.so"
#PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/hybridCache.so"
#APP=$1"-"$2$3
#OUTPUT=$APP".hybrid"

APP=$1



PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-intel64/LifetimeofWrite.so"

HEAD=400000
SCHEME=$2
BLOCK_SIZE=$3
CACHE_SIZE=$4
RETENTION=$5
WRITE_LAT=$6

OPT_RETENTION="-r $RETENTION"
OPT_WRITE_LAT="-wl $WRITE_LAT"
OPT_HEAD="-head $HEAD"

#--volTrace2ilp
ITRACE=$APP.trace
OUTPUT=$APP"_"$BLOCK_SIZE"_"$CACHE_SIZE"_"$WRITE_LAT.lifetime2



	if [ $# -lt 1 ]; then
		echo "lack of argument"
		exit
	fi

	DIR=$1
	cd $DIR
	echo "====Into: $(pwd)"	
	
	#
	echo "$PIN -t $PIN_TOOL -it $ITRACE -b $BLOCK_SIZE -c $CACHE_SIZE -r $RETENTION $OPT_WRITE_LAT $OPT_HEAD -o $OUTPUT -- ./$APP >log"
	$PIN -t $PIN_TOOL -it $ITRACE -b $BLOCK_SIZE -c $CACHE_SIZE -r $RETENTION $OPT_WRITE_LAT $OPT_HEAD -o $OUTPUT -- ./$APP >log	
	
	cd ..
	echo "====Back to: $(pwd)"

	if [ $? -ne 0 ]; then
		exit 
	fi
	
	
