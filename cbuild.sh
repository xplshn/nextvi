#!/bin/sh

cbuild_OPWD="$PWD"
BASE="$(dirname "$(realpath "$0")")"
if [ "$OPWD" != "$BASE" ]; then
	cd "$BASE" || exit 1
fi
trap 'cd "$cbuild_OPWD"' EXIT

# Color escape sequences
G="\033[32m" #    Green
Y="\033[33m" #    Yellow
R="\033[31m" #    Red
B="\033[34m" #    Blue
NC="\033[m"  #    Unset

log() {
	# shellcheck disable=SC2059 # Using %s with ANSII escape sequences is not possible
	printf "${1}->$NC "
	shift
	printf "%s\n" "$*"
}

require() {
	which "$1" >/dev/null 2>&1 || {
		log "$R" "[$1] is not installed. Please ensure the command is available [$1] and try again."
		exit 1
	}
}

run() {
	log "$B" "$*"
	eval "$*"
}

CFLAGS="\
-pedantic -Wall -Wextra \
-Wno-implicit-fallthrough \
-Wno-missing-field-initializers \
-Wno-unused-parameter \
-Wno-unused-result \
-Wfatal-errors -std=c99 \
-D_POSIX_C_SOURCE=200809L $CFLAGS"

: "${CC:=cc}"
: "${PREFIX:=/usr/local}"
: "${OS:=$(uname)}"
case "$OS" in
*BSD*) CFLAGS="$CFLAGS -D_BSD_SOURCE" ;;
*Darwin*) CFLAGS="$CFLAGS -D_DARWIN_C_SOURCE" ;;
esac

OPTIMIZE_FLAGS="-O2"
while [ $# -gt 0 ]; do
    case "$1" in
    "debug")
        shift
        log "$G" "Entering step: \"Apply debugging flags\""
        OPTIMIZE_FLAGS="-O0 -g"
        set -- build "$@"
        ;;
    "" | "build")
    	shift
    	require "${CC:=cc}"
    	log "$G" "Entering step: \"Build \"$(basename "$BASE")\" using \"$CC\""
    	run "$CC vi.c -o vi $OPTIMIZE_FLAGS $CFLAGS" || {
    		log "$R" "Failed during step: \"Build \"$(basename "$BASE")\" using \"$CC\""
    		exit 1
    	} && exit 0
    	;;
    "pgobuild")
    	shift
    	pgobuild() {
    		ccversion="$($CC --version)"
    		case "$ccversion" in *clang*) clang=1 ;; esac
    		if [ "$clang" = 1 ] && [ -z "$PROFDATA" ]; then
    			if command -v llvm-profdata >/dev/null 2>&1; then
    				PROFDATA=llvm-profdata
    			elif xcrun -f llvm-profdata >/dev/null 2>&1; then
    				PROFDATA="xcrun llvm-profdata"
    			fi
    			[ -z "$PROFDATA" ] && echo "pgobuild with clang requires llvm-profdata" && exit 1
    		fi
    		run "$CC vi.c -fprofile-generate=. -o vi -O2 $CFLAGS"
    		echo "qq" | ./vi -v ./vi.c >/dev/null
    		[ "$clang" = 1 ] && run "$PROFDATA" merge ./*.profraw -o default.profdata
    		run "$CC vi.c -fprofile-use=. -o vi -O2 $CFLAGS"
    		rm -f ./*.gcda ./*.profraw ./default.profdata
    	}
    	shift
    	require "${CC:=cc}"
    	log "$G" "Entering step: \"Build \"$(basename "$BASE")\" using \"$CC\" and PGO\""
    	pgobuild || {
    		log "$R" "Failed during step: \"Build \"$(basename "$BASE")\" using \"$CC\" and PGO\""
    		exit 1
    	} && exit 0
    	;;
    "clean")
    	shift
    	[ -x ./vi ] || {
    		log "$R" "./vi does not exist!"
    		exit 1
    	} && run rm ./vi
    	exit 0
    	;;
    "retrieve")
    	[ -x ./vi ] || {
    		log "$R" "./vi does not exist!"
    		exit 1
    	} && mv ./vi ./nextvi
    	readlink -f ./nextvi
    	exit 0
    	;;
    *)
        echo "Usage: $0 {build|debug|pgobuild|clean}"
        exit 1
        ;;
    esac
done
