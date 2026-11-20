#!/bin/sh
set -eu
bin=${1:?pass the path to the mocked executable}
result=$(MOCK_I2C=normal "$bin" /dev/null 0x69)
test "$result" = 'id=0x68 temp_raw=-84 temp_c=36.28'

expect_failure()
{
    if result=$(MOCK_I2C="$1" "$bin" /dev/null 2>&1); then
        return 1
    else
        test "$?" = 1
    fi
    case "$result" in
        "$2"*) ;;
        *) printf '%s\n' "$result" >&2; return 1 ;;
    esac
}

expect_failure no-i2c 'adapter does not support plain I2C'
expect_failure busy 'I2C_SLAVE'
expect_failure short 'read WHO_AM_I'
expect_failure wrong-id 'unexpected WHO_AM_I'
printf '%s\n' 'I2C mock: repeated start, 7-bit address, wake, signed temperature, short transfer, busy adapter and wrong identity'
