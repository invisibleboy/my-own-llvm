CPPCLAGS=
RUN_OPTIONS=
BIN_PATH="/home/qali/Build/my-llvm2.8/Debug+Asserts/bin/"
GCC="llvm-gcc"
GCC_OPT=" -I/usr/include/i386-linux-gnu"
LLVM_LINK=${BIN_PATH}"llvm-link"
LLC=${BIN_PATH}"llc"
LLC2=${BIN_PATH}"llc"
LLVM_LD=${BIN_PATH}"llvm-ld"
OPTI=-O3
CC=gcc
#ALPHA=8
#BETA=-2
	
	if [ $# -lt 2 ]; then
		echo "lack of argument"
		exit
	fi
	

	if [ -f $1"-"$2$3 ]; then
		rm $1"-"$2$3
	fi
	if [ -f log ]; then
		rm log	
	fi

	if [ $2 -ne 0 -o $3 -ne 0 ]; then
		LLC=$LLC2
	fi 

	ALPHA=$2
	BETA=$3
	DIR=$1
	cd $DIR
	echo "====Into: $(pwd)"

	if [[ $DIR == "SMG2000" ]]; then
		CPPFLAGS="-D_POSIX_SOURCE -DHYPRE_TIMING -DHYPRE_SEQUENTIAL -I."
		RUN_OPTIONS="-n 100 40 100 -c 0.1 1.0 10.0"
	elif [[ $DIR == "typeset" ]]; then
		CPPFLAGS="-DOS_UNIX=1 -DOS_DOS=0 -DOS_MAC=0 -DDB_FIX=0 -DUSE_STAT=1 -DSAFE_DFT=0 -DCOLLATE=1 -DLIB_DIR=\"lout.lib\" -DFONT_DIR=\"font\" -DMAPS_DIR=\"maps\" -DINCL_DIR=\"include\" -DDATA_DIR=\"data\" -DHYPH_DIR=\"hyph\" -DLOCALE_DIR=\"locale\" -DCHAR_IN=1 -DCHAR_OUT=0 -DLOCALE_ON=1 -DASSERT_ON=1 -DDEBUG_ON=0  -DPDF_COMPRESSION=0"
		RUN_OPTIONS="-x -I ./data/include -D ./data/data -F ./data/font -C ./data/maps -H ./data/hyph ./large.lout"
	elif [[ $DIR == "distray" ]]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"C\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [ "$DIR" == "mason" ]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"C\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [ "$DIR" == "neural" ]; then
		CPPFLAGS="-DVERSION='"1.00"' -DCOMPDATE="\"today\"" -DCFLAGS="\"C\"" -DHOSTNAME="\"thishost\"""
		RUN_OPTIONS=ref.in
	elif [[ $DIR == "drop3" ]]; then
		RUN_OPTIONS=input-file	
	elif [[ $DIR == "md5" ]]; then
		RUN_OPTIONS=10
	elif [[ $DIR == "url" ]]; then
		RUN_OPTIONS="medium_inputs 900"
	elif [[ $DIR == "patricia" ]]; then
		RUN_OPTIONS=large.udp
	elif [ "$DIR" == "flops" ] || [ "$DIR" == "mandel" ] || [ "$DIR" == "perlin" ]; then
		RUN_OPTIONS=
	elif [ $DIR == "test" ]; then
		RUN_OPTIONS=
	else
		echo "Not acceptable benchmark"
		exit 1
	fi
	
		

	#compile each c source file
	touch qali.c
	bcfiles=
	for CFILE in *; do
		if [[ $CFILE == *.c ]]; then
			bc_file=$(echo ${CFILE}|sed -e 's/\.c/\.bc/')
			echo "$GCC $GCC_OPT $OPTI -emit-llvm -c $CPPFLAGS $CFILE -o $bc_file"
			$GCC $GCC_OPT $OPTI -emit-llvm -c $CPPFLAGS $CFILE -o $bc_file 2>>log
			if [ $? -ne 0 ]; then
				exit 1
			fi
			bcfiles=${bcfiles}" "${bc_file}
		fi
	done

	#link the bc files into a single bc file
	echo "$LLVM_LINK $bcfiles -o main.bc 2>>log"
	$LLVM_LINK $bcfiles -o main.bc 2>>log
	
	#llc *.bc into *.s file
	echo "$LLC $OPTI -debug-only=ACCESS -alpha=$ALPHA -beta=$BETA main.bc -o main.s 2>>log"
	$LLC $OPTI -debug-only=ACCESS -alpha=$ALPHA -beta=$BETA main.bc -o main.s 2>>log

	#echo "$LLC $OPTI -debug-only=ACCESS  main.bc -o main.s 2>>log"
	#$LLC $OPTI -debug-only=ACCESS  main.bc -o main.s 2>>log

	#binary file name
	binName=$1"-"$2$3
	#assemble, link the main.bc into main.out
	echo "$CC -g main.s -static -lm -o $binName"
	$CC -g main.s -static -lm -o $binName 2>>log

	#rename .lock file with alpha-beta identifier
	echo "mv main.bc.lock $binName.lock"
	mv main.bc.lock $binName.lock
	
	cd ..
	echo "====Back to: $(pwd)"
	
	if [ $? -ne 0 ]; then
		echo "Error"
		exit 1
	fi	
