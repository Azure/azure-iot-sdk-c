# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license tagsrmation.
import os
import docker
import github


class DockerTags:
    def __init__(self):
        self.repo = None
        self.commit_name = None
        self.commit_sha = None
        self.docker_repo = os.environ.get("AZURECR_REPO_ADDRESS")
        self.image_tags = []
        self.image_tag_to_use_for_cache = None


def image_tag_prefix():
    """
    Return a prefix to use on all tags
    """
    # put the prefix as an attribute on this function to make it
    # behave like a function-scope static variable that we only
    # initialize once
    if not hasattr(image_tag_prefix, "prefix"):
        client = docker.from_env()
        version = client.version()
        image_tag_prefix.prefix = "{}-{}-dockerV{}".format(
            version["Os"], version["Arch"], version["Version"].split(".")[0]
        )
    return image_tag_prefix.prefix


def shorten_sha(str):
    """
    return the short (7-character) version of a git sha
    """
    return str[:7]


def sanitize_string(str):
    """
    sanitize a string by removing /, \\, and -, and convert to PascalCase
    """
    for strip in ["/", "\\", "-"]:
        str = str.replace(strip, " ")
    components = str.split()
    return "".join(x.title() for x in components)


def running_on_azure_pipelines():
    """
    return True if the script is running inside of an Azure pipeline
    """
    return "BUILD_BUILDID" in os.environ


def get_commit_name(commit):
    if commit.startswith("refs/heads/"):
        return commit.split("/")[2]
    elif commit.startswith("refs/tags/"):
        return commit.split("/")[2]
    elif commit.startswith("refs/pull/"):
        return "PR" + commit.split("/")[2]
    else:
        return commit


def get_docker_tags_from_commit(repo, commit):
    tags = DockerTags()
    tags.docker_image_name = "raspi-c"
    tags.docker_full_image_name = "{}/{}".format(
        tags.docker_repo, tags.docker_image_name
    )
    tags.repo = repo
    tags.commit_name = get_commit_name(commit)
    tags.commit_sha = github.get_sha_from_commit(repo, commit)
    
    """
    The importance of tags:

    Horton performance relies heavily on the use of Docker tags and the --cache-from option in
    the `docker build` command.  The reason for this is because `docker build` relies heavily
    on smartly written Dockerfile's and local caching of docker images.  In other words, Docker
    tries very hard to not re-build everything again-and-again when you run `docker build`.

    Normally, on a single user machine, if you make an image that starts with "from ubuntu",
    followed by "apt install VIM", docker will only need to download the ubuntu base once and
    run the apt command once.  If you run `docker build` 10 times, it only needs to run apt
    once because it can cache the result from that first time.

    The need for this tag schema comes from the fact that we're running Horton on many machines
    and, in production, we always start with a clean machine that has an empty docker cache.
    If Horton ran `docker build` on a clean machine every time, it would have to run `apt install git`
    every time.  This would negate the effect of docker image caching and make builds very slow.

    To make builds faster, we tag images to give docker a starting point for it's build.  In effect,
    if we build the C sdk from the master branch, we tag it as such.  Then, next time we need to
    build the C sdk from the master branch, we can load the last build and let docker use it
    as a starting point.

    Every image we make has (at least) 4 tags.  For example, an image might have:
        pythonpreview-e2e-v2:vsts-12345
        pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview
        pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview-Pr59
        pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview-Pr59-510b1f9

    When we're build a docker image, we first look in our container registry to find an image to
    cache from.  This image should be as specific as possible, so we pull starting with the most
    specific tag and move to the lest specific tag.  If we can't find any tags, we just start
    from the beginning.

    The first tag is used when we manually want to grab a particular image.  In the example above, the
    vsts-12345 tag makes it very easy for us to grab "the image that was tested in pipeline job 12345.

    The next 3 tags increase in specificity to let us start from the most specific starting spot.  If we're
    building pull-request (PR) 59 and the SHA is 510b1f9, and we can pull an image with this tag, it's
    probably exactly what we want and the `docker build` command will be very fast.

    If we're building PR 59, but we've made changes so the SHA has changed, we won't be able to find the
    tag that ends with -510b1f9, but we will find the tag that ends with PR59.  It won't be quite as
    perfect, but it will be a good starting point.

    Finally, if we're building with a new PR (say PR 60), and this is the first time Horton is
    building an image, there won't be any PR60 tags, but there will be the tag that ends with
    AzureIotSdkPythonPreview.  This will give `docker build` a starting point based on the same
    repo.

    If we're building with a Dockerfile variant, and we know the name of that variant, we
    insert the name of the variant into the tag so tags don't collide with builds that use
    other variants.
    """
    # eg: pythonpreview-e2e-v2:linux-amd64-dockerV18
    tags.image_tags.insert(0, "{}".format(image_tag_prefix()))
    # eg: pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview
    tags.image_tags.insert(
        0, "{}-{}".format(image_tag_prefix(), sanitize_string(tags.repo))
    )
    # eg: pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview-Pr59
    tags.image_tags.insert(
        0,
        "{}-{}-{}".format(
            image_tag_prefix(),
            sanitize_string(tags.repo),
            sanitize_string(tags.commit_name),
        ),
    )
    # eg: pythonpreview-e2e-v2:linux-amd64-dockerV18-AzureAzureIotSdkPythonPreview-Pr59-510b1f9
    tags.image_tags.insert(
        0,
        "{}-{}-{}-{}".format(
            image_tag_prefix(),
            sanitize_string(tags.repo),
            sanitize_string(tags.commit_name),
            shorten_sha(tags.commit_sha),
        ),
    )
    return tags
