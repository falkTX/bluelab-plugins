#!/bin/sh

# bluelabpxx@ssh.cluster026.hosting.ovh.net:bluelab.git

# git submodules init
# git submodules update

git pull --recurse-submodules

cd bl-darknet
git checkout master
git pull ovh master

cd ..
cd BL-Plugins
git checkout master
git pull ovh master

cd ..
cd BuildSystem
git checkout master
git pull ovh master

cd ..
cd iPlug2
git checkout bluelab
git pull ovh bluelab

cd ..
cd TestSystem-mac
git checkout master
git pull origin master

cd ../Libs
cd CrossoverFilter
git checkout master
git pull ovh master

cd ..
cd darknet
git checkout bluelab
git pull ovh bluelab

cd ..
cd Decimator
git checkout master
git pull ovh master

cd ..
cd fastapprox
git checkout master
git pull ovh master

cd ..
cd fast-math
git checkout master
git pull ovh master

cd ..
cd flac
git checkout bluelab
git pull ovh bluelab

cd ..
cd fmem
git checkout bluelab
git pull origin bluelab

cd ..
cd freeverb
git checkout master
git pull ovh master

cd ..
cd glm
git checkout bluelab
git pull origin bluelab

cd ..
cd ini_parser
git checkout master
git pull ovh master

cd ..
cd KalmanFilter
git checkout master
git pull ovh master

cd ..
cd libdct
git checkout master
git pull ovh master

cd ..
cd libebur128
git checkout bluelab
git pull origin bluelab

cd ..
cd libsamplerate/
git checkout master
git pull ovh master

cd ..
cd libsndfile
git checkout master
git pull ovh master

cd ..
cd OpenBLAS
git checkout bluelab
git pull ovh bluelab

cd ..
cd PhaseVocoder-DSP
git checkout master
git pull ovh master

cd ..
cd r8brain
git checkout master
git pull ovh master

cd ..
cd RandomSequence
git checkout bluelab
git pull origin bluelab

cd ..
cd RTConvolve
git checkout bluelab
git pull origin bluelab

cd ..
cd smbPitchShift
git checkout master
git pull ovh master

cd ..
cd sndfilter
git checkout bluelab
git pull origin bluelab

cd ..
cd ..
