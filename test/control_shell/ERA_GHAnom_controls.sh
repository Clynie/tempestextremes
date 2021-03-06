#!/bin/bash

#This script will automate the PV blocking calculations for the input data list

export ystart=$1
export yend=$2
export yint=$3
export DATA_DIR="/Volumes/ExFAT_drive/ERA_files/"
export AVG_INPUT="ERA_avg_GH_list.txt"
export DEV_INPUT="ERA_dev_GH_list.txt"
export BLOB_INPUT="ERA_blob_GH_list.txt"
export SEASONS=( "DJF" "MAM" "JJA" "SON" )
export NDAYS=5


if [ $# -lt 3 ]; then
  echo "Forgot to provide year numbers!"
  exit
fi

if [ ! -d $DATA_DIR ]; then
  echo "Error! Check your file path."
  exit
fi

cd $DATA_DIR

if [ -e $AVG_INPUT ]; then
  rm $AVG_INPUT
fi

if [ -e $DEV_INPUT ]; then
  rm $DEV_INPUT
fi

if [ -e $BLOB_INPUT ]; then
  rm $BLOB_INPUT
fi

#Search for files over specified range
ys=$((ystart))
ye=$((yend))
for ((y=$ys; y<=$ye; y++)); do
  for m in $(seq -f "%02g" 1 12); do
    infile="ERA_"$y"/ERA_"$y"_"$m"_vars.nc"
    outfile="ERA_"$y"/ERA_"$y"_"$m"_vars_z500.nc"
    #run integration code if file doesn't already exist
    if [ ! -e $outfile ]; then
      ~/tempestextremes/bin/Var4Dto3D --in $infile --varlist Z --gh  --out $outfile --hpa
    fi
    echo $outfile >> $AVG_INPUT
  done
done

#List of files for year that PV* is calculated
yi=$((yint))
for m in $(seq -f "%02g" 1 12); do
  infile="ERA_"$yi"/ERA_"$yi"_"$m"_vars_z500.nc"
  echo $infile >> $DEV_INPUT
done

#Calculate block average
avg_outfile="ERA_avg/ERA_"$ys"_"$ye"_Z500_avg.nc"
if [ ! -e $avg_outfile ]; then
  ~/tempestextremes/bin/BlockingAvg --inlist $AVG_INPUT --out $avg_outfile --varname Z --avgname AVGZ
fi

c=0
#Make list to input for StitchBlobs
for m in $(seq -f "%02g" 1 12); do
  devfile="ERA_"$yi"/ERA_"$yi"_"$m"_vars_z500_devs.nc"
  if [ ! -e $devfile ]; then
    c=$((c+1))
  fi
  echo $devfile >> $BLOB_INPUT
done

#echo $c " files missing"
#calculate deviations
#if [ c -gt 0 ]; then
  ~/tempestextremes/bin/BlockingDevs --inlist $DEV_INPUT --avg $avg_outfile --varname Z --avgname AVGZ --gh
#fi

#nsteps=$((NDAYS*4))
#Run StitchBlobs for whole year
#blobsfile="ERA_"$yi"_"$NDAYS"day_GHAnom_blobs.nc"
#~/tempestextremes/StitchBlobs --inlist $BLOB_INPUT --out $blobsfile --var INT_ADGH --minsize 5 --mintime $nsteps

#Calculate blocking density for year
#densfile="ERA_"$yi"_GHAnom_density.nc"
#~/tempestextremes/blockingDensity --in $blobsfile --var INT_ADGHtag --out $densfile

#Make plot of density for year
#python ~/tempestextremes/plot_density.py $densfile year $yi "GH Anomaly"

#echo "Finished year plot."
#Run StitchBlobs for seasonal (DJF,MAM,JJA,SON)
#DJF
#yprev=$((yi-1))
#check that December (previous year!) file exists
#dfile="ERA_"$yprev"/ERA_"$yprev"_12_vars_GH_devs.nc"
#if [ ! -e $dfile ]; then
#  echo "Error: deviations not calculated for December of "$yprev
#  exit
#else
  #array of season list file names
#  x=0
#  declare -a SFILES
#  for s in ${SEASONS[@]}; do
#    echo $x
#    SFILES[x]="ERA_"$s"_GHAnom_blobs_list.txt"
#    x=$((x+1))
#  done
  #initialize lists
#  echo $dfile > ${SFILES[0]}
#  echo "ERA_"$yi"/ERA_"$yi"_03_vars_GH_devs.nc" > ${SFILES[1]}
#  echo "ERA_"$yi"/ERA_"$yi"_06_vars_GH_devs.nc" > ${SFILES[2]}
#  echo "ERA_"$yi"/ERA_"$yi"_09_vars_GH_devs.nc" > ${SFILES[3]}
#  #other files added to lists
#  for n in $(seq 1 2); do
#    for x in $(seq 0 3); do
#      n1=$((3*$x+$n))
#      echo $n1
#      n2=$(printf "%02d" $n1)
#      echo $n2
#      echo "ERA_"$yi"/ERA_"$yi"_"$n2"_vars_GH_devs.nc" >> ${SFILES[$x]}
#    done
#  done
#fi

# Run StitchBlobs and density calcs for seasonal
#for x in $(seq 0 3); do 
#  blobsfile="ERA_"$yi"_"${SEASONS[$x]}"_"$NDAYS"day_GHAnom_blobs.nc"
#  densfile="ERA_"$yi"_"${SEASONS[$x]}"_GHAnom_density.nc"
#
#  ~/tempestextremes/StitchBlobs --inlist ${SFILES[$x]} --out $blobsfile --var INT_ADGH --minsize 5 --mintime $nsteps
#  ~/tempestextremes/blockingDensity --in $blobsfile --var INT_ADGHtag --out $densfile
#  python ~/tempestextremes/plot_density.py $densfile ${SEASONS[$x]} $yi "GH Anomaly" 
#done

