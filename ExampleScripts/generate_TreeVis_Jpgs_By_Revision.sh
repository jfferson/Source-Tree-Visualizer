#!/bin/bash

cd ~/Projects/source_tree_vis/
./bin/simple_tree -r -g ~/Projects/source_tree_vis -O sourceTreeVis.jpg
cd ~/Projects/source_tree_vis/ExampleScripts
convert -limit memory 400MiB -limit map 400MiB ~/Projects/source_tree_vis/out/*.jpg sourceTreeVis.gif


