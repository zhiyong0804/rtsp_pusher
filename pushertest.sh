#!/bin/bash
prefix=$1

for ((i=0; i<50; ++i))
do  
	./PusherModuleTest $1$i ./media &  
done  
