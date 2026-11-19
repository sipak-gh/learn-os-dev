#!/bin/sh
# qemu-test.sh — boot an image headless, type into COM1, compare the output.
#
#   tools/qemu-test.sh QEMU IMAGE EXPECTED [INPUT]
#
# Boots IMAGE with no display, COM1 on stdin/stdout. Each line of INPUT
# is typed into COM1 (ending in CR, like Enter on a terminal), but only
# once the shell has asked for it: line n is sent when the n-th prompt
# has appeared. We stop when QEMU exits by itself or when the last line
# of EXPECTED shows up in the output, whichever is first, or after TIMEOUT
# seconds. Then the output (CRs removed) must equal EXPECTED.
# The output is kept in build/serial.log.

set -u
QEMU=$1 IMAGE=$2 EXPECTED=$3 INPUT=${4:-}
TIMEOUT=${TIMEOUT:-10}
PROMPT=${PROMPT:-"sprout> "}
LOG=build/serial.log
RAW=build/serial.raw
FIFO=build/serial.in

last=$(tail -n 1 "$EXPECTED")
rm -f "$LOG" "$FIFO"
: >"$RAW"           # exists before the first poll
mkfifo "$FIFO"

# QEMU reads COM1 input from the FIFO. Opening a FIFO blocks until the
# other end is opened too, so start QEMU first, then open our end (fd 3).
timeout "$TIMEOUT" $QEMU -drive format=raw,file="$IMAGE" \
    -display none -serial stdio -monitor none -no-reboot \
    <"$FIFO" >"$RAW" 2>build/qemu.err &
pid=$!
exec 3>"$FIFO"

prompts() {
    grep -o -F -- "$PROMPT" "$RAW" | wc -l
}

# Type the input, one line per prompt.
if [ -n "$INPUT" ]; then
    n=0
    while IFS= read -r line; do
        n=$((n + 1))
        while [ "$(prompts)" -lt "$n" ]; do
            kill -0 "$pid" 2>/dev/null || break 2
            sleep 0.1
        done
        printf '%s\r' "$line" >&3
    done <"$INPUT"
fi

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
exec 3>&-
rm -f "$FIFO"

tr -d '\r' <"$RAW" >"$LOG"
if diff -u "$EXPECTED" "$LOG"; then
    echo "PASS: $IMAGE ($(wc -l <"$LOG") lines of serial output match $EXPECTED)"
else
    echo "FAIL: serial output differs from $EXPECTED (see $LOG)"
    exit 1
fi
