#!/bin/bash

image_file=$1

while inotifywait -e close_write $image_file
do
    scp $image_file pi@ledsuit:~/test_image.png
done
