#!/bin/bash

i=0 # Initialize the counter variable
cd ..
while [[ $i -lt 10 ]]; do
  echo "Creating delete directory  .... "; mkdir delete; cd delete; echo "DELETE ME !!!! " > delete.me
  ((i++)) # Increment i
done

echo "Loop finished. Final value of i: $i"
