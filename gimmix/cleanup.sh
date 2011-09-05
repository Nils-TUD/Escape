#!/bin/bash
find build/src -type f -name "*.gcda" -print0 | xargs -0 rm -f
