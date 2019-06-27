# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license tagsrmation.
import requests


def get_sha_from_commit(repo, commit):
    """
    given a GIT repo and a commit ID, return the SHA for that commit
    """
    if not commit.startswith("refs/"):
      commit = "refs/heads/" + commit
    response = requests.get(
        "https://api.github.com/repos/{}/git/{}".format(repo, commit)
    )
    if response.status_code == 200:
        return response.json()["object"]["sha"]
    elif response.status_code == 404:
        raise Exception("ERROR : Commit {} not found in repo {}".format(commit, repo))
    else:
        raise Exception(
            "unexpected result looking for commit {} in repo {} status = {} response = {}".format(
                commit, repo, response.status_code, response.json()
            )
        )


def get_sha_url_and_ref_from_prid(repo, prid):
    """
    given a GIT repo and a pull request ID, return the SHA, clone_url, and ref for that pull request
    """
    response = requests.get(
        "https://api.github.com/repos/{}/pulls/{}".format(repo, prid)
    )
    if response.status_code == 200:
        sha = response.json()["head"]["sha"]
        url = response.json()["head"]["repo"]["clone_url"]
        ref = response.json()["head"]["ref"]
        return (sha, url, ref)
    elif response.status_code == 405:
        raise Exception("ERROR : prid {} not found in repo {}".format(prid, repo))
    else:
        raise Exception(
            "unexpected result looking for prid {} in repo {} status = {} response = {}".format(
                prid, repo, response.status_code, response.json()
            )
        )

