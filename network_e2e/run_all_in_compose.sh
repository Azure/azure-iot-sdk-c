#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
build_folder=$build_root/cmake/linux_network_e2e
compose_dir=$script_dir/compose.linux

# remove previous containers.  we have to do this so we don't get logs from old runs
docker rm $(docker ps -a -f status=exited -f ancestor=jenkins-network-e2e -q) > /dev/null 2> /dev/null

# run the tests
pushd $compose_dir
docker-compose up 

# log header
echo -----------------------------------------
echo -----------------------------------------
echo JOBS COMPLETE
echo -----------------------------------------
echo -----------------------------------------
echo

# get a list of failed jobs
exited_jobs=$(docker ps -a -f status=exited -f ancestor=jenkins-network-e2e -q)
failed_jobs=$(docker inspect -f '{{if ne 0.0 .State.ExitCode }}{{.Id}}{{ end }}' $exited_jobs)

# full logs for failed jobs
failed_job_count=0
for job in $failed_jobs 
do
	commandline=$(docker inspect -f {{.Args}} $job)
	failed_job_count=$((failed_job_count+1))
	echo -----------------------------------------
	echo FULL OUTPUT from $commandline
	docker logs -t $job
done

# job summary for failed jobs
for job in $failed_jobs 
do
	commandline=$(docker inspect -f {{.Args}} $job)
	echo -----------------------------------------
	echo SUMMARY of $commandline
	# Match these:
	# 2017-03-23T22:13:08.456515280Z 1: Suceeded.
	# 2017-03-23T22:13:08.456542929Z 1: Executing test IotHub_BadNetwork_disconnect_create_send_reconnect_X509 ... 
	# 2017-03-23T22:12:53.069166382Z 1: !!! FAILED !!! 
	# 1: 6 tests ran, 1 failed, 5 succeeded.
	docker logs -t $job | grep -E "Executing test|\!\!\! FAILED| Suceeded.$| tests ran,"
done

echo -----------------------------------------
echo compose summary
total_job_count=0
for job in $exited_jobs
do
	total_job_count=$((total_job_count+1))
	commandline=$(docker inspect -f {{.Args}} $job)
	exitcode=$(docker inspect -f {{.State.ExitCode}} $job)
	started_at=$(docker inspect -f {{.State.StartedAt}} $job)
	finised_at=$(docker inspect -f {{.State.FinishedAt}} $job)
	
	StartDate=$(date -u -d "$started_at" +"%s")
	FinishDate=$(date -u -d "$finised_at" +"%s")
	elapsed=$(date -u -d "0 $FinishDate sec - $StartDate sec" +"%H:%M:%S")
	
	if [ $exitcode -eq 0 ]
	then
		echo PASSED: \($elapsed\) $commandline
	else
		echo FAILED: \($elapsed\) $commandline
	fi
done

echo
echo $((total_job_count-failed_job_count))/$total_job_count passed

exit $failed_job_count




