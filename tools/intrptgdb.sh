#!/bin/bash

PID=`pgrep '^gdbtui$'`
if [ "$PID" != "" ]; then
	kill -INT $PID
fi
