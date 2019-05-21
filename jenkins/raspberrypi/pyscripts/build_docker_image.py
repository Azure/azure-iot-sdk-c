# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information
import os
import docker
import json
import sys
import docker_tags
import argparse
import datetime
import colorama
from colorama import Fore

colorama.init(autoreset=True)

default_repo = "(Azure/azure-iot-sdk-BLAH)"

parser = argparse.ArgumentParser(description="build docker image for testing")
parser.add_argument("--repo", help="repo with source", type=str, default=default_repo)
parser.add_argument(
    "--commit", help="commit to apply (ref or branch)", type=str, default="master"
)
args = parser.parse_args()

if args.repo == default_repo:
    args.repo = "Azure/azure-iot-sdk-c"
    print(Fore.YELLOW + "Repo not specified.  Defaulting to " + args.repo)
    
print_separator = "".join("/\\" for _ in range(80))

auth_config = {
    "username": os.environ["AZURECR_REPO_USER"],
    "password": os.environ["AZURECR_REPO_PASSWORD"],
}


def print_filtered_docker_line(line):
    try:
        obj = json.loads(line.decode("utf-8"))
    except:
        print(line)
    else:
        if "status" in obj:
            if "id" in obj:
                if obj["status"] not in [
                    "Waiting",
                    "Downloading",
                    "Verifying Checksum",
                    "Extracting",
                    "Preparing",
                    "Pushing",
                ]:
                    print("{}: {}".format(obj["status"], obj["id"]))
                else:
                    pass
            else:
                print(obj["status"])
        elif "error" in obj:
            raise Exception(obj["error"])
        else:
            print(line)


def build_image(tags):
    
    print(print_separator)
    print("BUILDING IMAGE")
    print(print_separator)

    dockerfile = "Dockerfile"
    dockerfile_directory = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../"))

    api_client = docker.APIClient(base_url="unix://var/run/docker.sock")
    build_args = {
        "CLIENTLIBRARY_REPO": tags.repo,
        "CLIENTLIBRARY_COMMIT_NAME": tags.commit_name,
        "CLIENTLIBRARY_COMMIT_SHA": tags.commit_sha
    }

    if tags.image_tag_to_use_for_cache:
        cache_from = [
            tags.docker_full_image_name + ":" + tags.image_tag_to_use_for_cache
        ]
        print("using {} for cache".format(cache_from[0]))
    else:
        cache_from = []


    print(
        Fore.YELLOW
        + "Building image for "
        + tags.docker_image_name
        + " using "
        + dockerfile
    )
    for line in api_client.build(
        path=dockerfile_directory,
        tag=tags.docker_image_name,
        buildargs=build_args,
        cache_from=cache_from,
        dockerfile=dockerfile,
    ):
        try:
            sys.stdout.write(json.loads(line.decode("utf-8"))["stream"])
        except KeyError:
            print_filtered_docker_line(line)


def tag_images(tags):
    print(print_separator)
    print("TAGGING IMAGE")
    print(print_separator)
    api_client = docker.APIClient(base_url="unix://var/run/docker.sock")
    print("Adding tags")
    for image_tag in tags.image_tags:
        print("Adding " + image_tag)
        api_client.tag(tags.docker_image_name, tags.docker_full_image_name, image_tag)


def push_images(tags):
    print(print_separator)
    print("PUSHING IMAGE")
    print(print_separator)
    api_client = docker.APIClient(base_url="unix://var/run/docker.sock")
    for image_tag in tags.image_tags:
        print("Pushing {}:{}".format(tags.docker_full_image_name, image_tag))
        for line in api_client.push(
            tags.docker_full_image_name, image_tag, stream=True, auth_config=auth_config
        ):
            print_filtered_docker_line(line)

def extract_artifacts(tags):
    print(print_separator)
    print("GETTING CMAKE AS ARCHIVE")
    print(print_separator)
    # Publish directory should be in the top level folder of the sdk.
    source_artifacts = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../../source_artifacts.tar"))
    
    api_client = docker.APIClient(base_url="unix://var/run/docker.sock")
    for image in api_client.images():
        print("Image:")
        print(image)
    print("Creating Container: {}".format(tags.docker_image_name))
    con = api_client.create_container(tags.docker_image_name)
    print(con)

    bits, stat = api_client.get_archive(con["Id"],"/sdk/cmake")
    print(stat)
    with open(source_artifacts, 'wb') as f:
        for chunk in bits:
            f.write(chunk)
    

def prefetch_cached_images(tags):
    if docker_tags.running_on_azure_pipelines():
        print(print_separator)
        print(Fore.YELLOW + "PREFETCHING IMAGE")
        print(print_separator)
        tags.image_tag_to_use_for_cache = None
        api_client = docker.APIClient(base_url="unix://var/run/docker.sock")
        for image_tag in tags.image_tags:
            print(
                Fore.YELLOW
                + "trying to prefetch {}:{}".format(
                    tags.docker_full_image_name, image_tag
                )
            )
            try:
                for line in api_client.pull(
                    tags.docker_full_image_name,
                    image_tag,
                    stream=True,
                    auth_config=auth_config,
                ):
                    print_filtered_docker_line(line)
                tags.image_tag_to_use_for_cache = image_tag
                print(
                    Fore.GREEN
                    + "Found {}.  Using this for image cache".format(image_tag)
                )
                return
            except docker.errors.APIError:
                print(Fore.YELLOW + "Image not found in repository")


tags = docker_tags.get_docker_tags_from_commit(args.repo, args.commit)
prefetch_cached_images(tags)
build_image(tags)
tag_images(tags)
push_images(tags)
extract_artifacts(tags)
