#!/bin/bash

#should abort on errors!
set -e
export LC_NUMERIC=en_US.UTF-8

if [[ "$#" -lt 1 ]]
then
    echo "Illegal number of parameters"
    echo "Usage: $0 PATH_TO_MD-Flexible <-s>"
    echo " -s Silent: no scaling files are written and number of molecules and repetitions is reduced significantly."
    exit -1
fi

EXECUTABLE=$(readlink -e "${1}")
[[ $? -ne 0 ]] && echo "Path to md-flexible invalid!" && exit -2

SILENT=false
if [[ "${2}" =~ '-s' ]]
then
    SILENT=true
    echo "Running in silent mode."
fi

if [[ ${SILENT} = false ]]
then
    timestamp=`date +%Y-%m-%d_%H-%M-%S`
    outputDir="measurePerf_${timestamp}"

    mkdir ${outputDir}
    cd ${outputDir}

    Mols=(   16    32    64   128  256  512 1024 2048 4096 8192 16384 32768 65536)
    Reps=(10000 10000 10000 10000 1000 1000  200  100   20   20    20    20    20)
else
    Mols=(   16    32    64 )
    Reps=(   10    10    10 )
fi


# places text in the middle of a dashed line
function separate {
    sepSymbols="------------------------------------"
    text=$1
    echo
    echo "${sepSymbols:$(( (${#text} + 1 ) / 2))} ${text} ${sepSymbols:$((${#text} / 2))}"
    echo
}


# workaround because bash3 does not support declare -A
traversals__DirectSum=directSumTraversal
traversals__LinkedCells=c08
traversals__VerletLists=c08ThisIsADummy
traversals__VerletCells=verletC18
traversals__VerletCluster=c01

# iterate over containers
for container in DirectSum LinkedCells VerletLists VerletCluster VerletCells ;
do
    separate "Container: ${container}"
    # only set verlet options if needed
    if [[ ${container} =~ 'Verlet' ]];
    then
        VLRebuild=(1   5  10  20)
        VLSkin=( 0.0 0.1 0.2 0.3)
    else
        VLRebuild=(100000)
        VLSkin=(0)
    fi

    for dataLayout in AoS SoA ;
    do
        separate "${dataLayout}"

        for newton3Opt in on off ;
        do

            separate "Newton 3 ${newton3Opt}"

            # loop for different verlet rebuild frequencies and skins
            for iVL in `seq 0 $(( ${#VLRebuild[@]} - 1 ))` ;
            do
                configPrinted=false

                # since this loop only has one iteration for non verlet container only print for verlet
                if [[ ${container} =~ 'Verlet' ]];
                then
                    separate "VLRebuild: ${VLRebuild[$iVL]} VLSkin: ${VLSkin[$iVL]}"
                    filename="runtimes_${container}_${dataLayout}_N3${newton3Opt}_${VLRebuild[$iVL]}_${VLSkin[$iVL]}.csv"
                else
                    filename="runtimes_${container}_${dataLayout}_N3${newton3Opt}.csv"
                fi

                # workaround because there is no traversal for Verlet clusters with newton 3 yet.
                if [[ ${container} =~ 'VerletCluster' && ${newton3Opt} =~ 'on' ]];
                then
                    continue
                fi

                # iterate over molecules with the correct repetition
                for i in `seq 0 $(( ${#Mols[@]} - 1 ))` ;
                do
                    thisReps=${Reps[$i]}
                    # Direct sum is slow for huge number of particles -> limit number of iterations
                    if [[ ${container} = 'DirectSum' && ${Mols[$i]} -ge 2048 ]];
                    then
                        thisReps=3
                    fi

                    separate "Particles: ${Mols[$i]} Iterations: ${thisReps}"

                    # workaround because bash3 does not support declare -A
                    t=traversals__${container}

                    output=$(${EXECUTABLE} \
                        --container ${container} \
                        --traversal ${!t} \
                        --data-layout ${dataLayout} \
                        --cutoff 1 \
                        --box-length 10 \
                        --particles-generator uniform \
                        --particles-total ${Mols[$i]} \
                        --iterations ${thisReps} \
                        --tuning-interval $(( ${thisReps} + 1 )) \
                        --verlet-rebuild-frequency ${VLRebuild[$iVL]} \
                        --verlet-skin-radius ${VLSkin[$iVL]} \
                        --no-flops \
                        --newton3 ${newton3Opt}
                    )

                    printf "${output}\n"

                    if [[ "${SILENT}" = false ]] ; then
                        if [[ "${configPrinted}" = false ]] ; then
                            configPrinted=true
                            # print all output lines until, excluding, "Using" (this is the whole config part)
                            allMols=${Mols[@]}
                            allIterations=${Reps[@]}
                            sed '/Using/Q' <<< "${output}" | sed -e "s|\( *total[ :]*\)[0-9]*|\1${allMols}|" -e "s|\(Iterations[ :]*\)[0-9]*|\1${allIterations}|" >> ${filename}
                            echo >> ${filename}
                            printf "%12s%15s%15s%15s%24s\n" "NumParticles" "GFLOPs/s" "MFUPs/s" "Time[micros]" "SingleIteration[micros]" >> ${filename}
                        fi

                        gflops=$(echo "$output" | sed --quiet -e 's|GFLOPs/sec.*: \(.*\)|\1|gp')
                        mfups=$(echo "$output" | sed --quiet -e 's|MFUPs/sec.*: \(.*\)|\1|gp')
                        timeTotal=$(echo "$output" | sed --quiet -e 's|Time total.*: \(.*\) .*s (.*|\1|gp')
                        timeOne=$(echo "$output" | sed --quiet -e 's|One iteration.*: \(.*\) .*s (.*|\1|gp')

                        printf "%12d%15.2f%15.2f%15.2f%24.2f\n" "${Mols[$i]}" "$gflops" "$mfups" "$timeTotal" "$timeOne"  >> ${filename}
                    fi
                done
            done
        done
    done
done