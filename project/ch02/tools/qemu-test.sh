#!/bin/sh
# qemu-test.sh — boot an image headless and compare its serial output.
#
#   tools/qemu-test.sh QEMU IMAGE EXPECTED [INPUT]
#
# Boots IMAGE with no display, COM1 on stdin/stdout. INPUT (default: none)
# is fed to COM1 as if typed. We stop when QEMU exits by itself or when the
# last line of EXPECTED shows up in the output, whichever is first, or after
# TIMEOUT seconds. Then the output (CRs removed) must equal EXPECTED.
# The output is kept in build/serial.log.

set -u
QEMU=$1 IMAGE=$2 EXPECTED=$3 INPUT=${4:-/dev/null}
TIMEOUT=${TIMEOUT:-10}
LOG=build/serial.log
RAW=build/serial.raw

last=$(tail -n 1 "$EXPECTED")
rm -f "$LOG"
: >"$RAW"           # exists before the first poll

timeout "$TIMEOUT" $QEMU -drive format=raw,file="$IMAGE" \
    -display none -serial stdio -monitor none -no-reboot \
    <"$INPUT" >"$RAW" 2>build/qemu.err &
pid=$!

# Poll ten times a second until QEMU is gone or the last line arrived.
while kill -0 "$pid" 2>/dev/null; do
    if tr -d '\r' <"$RAW" | grep -qxF -- "$last"; then
        sleep 0.2                       # let anything after it arrive
        kill "$pid" 2>/dev/null
        break
    fi
    sleep 0.1
done
wait "$pid" 2>/dev/null

tr -d '\r' <"$RAW" >"$LOG"
if diff -u "$EXPECTED" "$LOG"; then
    echo "PASS: $IMAGE ($(wc -l <"$LOG") lines of serial output match $EXPECTED)"
else
    echo "FAIL: serial output differs from $EXPECTED (see $LOG)"
    exit 1
fi
