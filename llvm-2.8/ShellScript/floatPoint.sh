PIN_PATH=/home/qali/Develop/my-own-pin/pin-2.10-45467-gcc.3.4.6-ia32_intel64-linux/
PIN=$PIN_PATH"pin"
PIN_TOOL=$PIN_PATH"source/tools/SimpleExamples/obj-ia32/floatPoint.so"
APP=$1$2$3
OUTPUT=$1$2$3".floatPoint"

	if [ $# -lt 1 ]; then
		echo "lack of argument"
		exit
	fi

	DIR=$1
	echo "Current in: $(pwd)"

	if [[ $DIR == "SMG2000" ]]; then
		CPPFLAGS="-D_POSIX_SOURCE -DHYPRE_TIMING -DHYPRE_SEQUENTIAL -I."
		RUN_OPTIONS="-n 100 40 100 -c 0.1 1.0 10.0"
	elif [[ $DIR == "consumer-typeset" ]]; then
		CPPFLAGS="-DOS_UNIX=1 -DOS_DOS=0 -DOS_MAC=0 -DDB_FIX=0 -DUSE_STAT=1 -DSAFE_DFT=0 -DCOLLATE=1 -DLIB_DIR=\"lout.lib\" -DFONT_DIR=\"font\" -DMAPS_DIR=\"maps\" -DINCL_DIR=\"include\" -DDATA_DIR=\"data\" -DHYPH_DIR=\"hyph\" -DLOCALE_DIR=\"locale\" -DCHAR_IN=1 -DCHAR_OUT=0 -DLOCALE_ON=1 -DASSERT_ON=1 -DDEBUG_ON=0  -DPDF_COMPRESSION=0"
		RUN_OPTIONS="-x -I ./data/include -D ./data/data -F ./data/font -C ./data/maps -H ./data/hyph ./large.lout"
	elif [[ $DIR == "distray" ]]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [ "$DIR" == "mason" ]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [ "$DIR" == "neural" ]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [[ $DIR == "drop3" ]]; then
		RUN_OPTIONS=input-file	
	elif [[ $DIR == "enc-md5" ]]; then
		RUN_OPTIONS=10
	elif [[ $DIR == "netbench-url" ]]; then
		RUN_OPTIONS="medium_inputs 900"
	elif [[ $DIR == "network-patricia" ]]; then
		RUN_OPTIONS=large.udp
	#elif [[ $DIR == "flops" -o $DIR == "mandel" -o $DIR == "perlin" ]]; then
	fi

	echo "$PIN -t $PIN_TOOL -o $OUTPUT -- ./$APP >log 2>log2"
	$PIN -t $PIN_TOOL -o $OUTPUT -- ./$APP >log 2>log2	
	
	if [ $? -ne 0 ]; then
		exit 
	fi

	
