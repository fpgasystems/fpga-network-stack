#!/bin/bash

HLS_DIR="$PWD"

for dir in ${HLS_DIR}/*/; do
	dirpath=${dir%*/}
	dirname=${dirpath##*/}
	projpath="${dirpath}/${dirname}_prj"
	echo ${dirpath}
	echo ${dirname}
	if [ -d "${projpath}" ]; then
		echo "Deleting: ${projpath}"
		rm -rf ${projpath}
	fi
done

echo "Cleand all HLS IPs."
