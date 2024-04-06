#!/bin/bash
for filename in ./*.png; do
	[ -e "$filename" ] || continue
	convert "$filename" -background black -alpha copy -type truecolormatte PNG32:"$(basename "$filename" .png).png"
done

