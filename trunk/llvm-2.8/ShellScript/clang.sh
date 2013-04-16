CPPCLAGS=
RUN_OPTIONS=
GCC="clang"
#GCC_OPT=" -I/usr/include/i386-linux-gnu"
GCC_OPTION="-O0"
LLVM_LINK="llvm-link"
LOPT="opt"
LOPT_OPTION=-O3

LLC=${BIN_PATH}"llc"
LLC2=${BIN_PATH}"llc"
LLVM_LD=${BIN_PATH}"llvm-ld"

TARGET_FILE=$1


CC=gcc
#ALPHA=8
#BETA=-2
	
	if [ $# -lt 0 ]; then
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

	if [[ $DIR == "CRC32" ]]; then
		echo "#Succeed: no need special options"
	elif [[ $DIR == "SMG2000" ]]; then
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
	
		

	# 1. compile .c file to .bc file
	
	bcfiles=
	for CFILE in *; do
		if [[ $CFILE == *.c ]]; then
			bc_file=$(echo ${CFILE}|sed -e 's/\.c/\.bc/')
			echo "$GCC $GCC_OPTION  -emit-llvm -c $CFILE -o $bc_file"
			$GCC $GCC_OPTION -emit-llvm -c $CFILE -o $bc_file 2>>log
			if [ $? -ne 0 ]; then
				exit 1
			fi
			bcfiles=${bcfiles}" "${bc_file}
		fi
	done
	
	touch qali.bc
	#2. link the bc files into a single bc file
	echo "$LLVM_LINK $bcfiles -o $TARGET_FILE-all.bc 2>>log"
	$LLVM_LINK $bcfiles -o $TARGET_FILE-all.bc 2>>log

	#3. customize the optimization choices
	echo "$LOPT $LOPT_OPTION-all.bc -o  $TARGET_FILE-all-opt.bc 2>>log"
	$LOPT $LOPT_OPTION $TARGET_FILE-all.bc -o $TARGET_FILE-all-opt.bc 2>>log
	
	#4. compile *-opt.bc into *.out file
	echo "$GCC -static $TARGET_FILE-all-opt.bc -o $TARGET_FILE.out 2>>log"
	$GCC -static $TARGET_FILE-all-opt.bc -o $TARGET_FILE.out 2>>log

	
	
	cd ..
	echo "====Back to: $(pwd)"
	
	if [ $? -ne 0 ]; then
		echo "Error"
		exit 1
	fi	
