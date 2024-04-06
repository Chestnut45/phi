#!/bin/bash
for filename in ./*.png; do
	[ -e "$filename" ] || continue
	convert "$filename"  -fx 'a==0 ? 0 : u' "$(basename "$filename" .png).png"
done

