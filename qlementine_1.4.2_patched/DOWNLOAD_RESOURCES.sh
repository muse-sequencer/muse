#!/bin/bash

# script for dowloading additional content:
#./docs
#./sandbox/resources
#./lib/resources
#./showcase/resources
#./branding


# Define repository details
REPO_OWNER="oclero"
REPO_NAME="qlementine"
TAG_OR_BRANCH="v1.4.2"

# GitHub archive URL
ARCHIVE_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/archive/refs/tags/${TAG_OR_BRANCH}.tar.gz"

echo "Downloading directories from ${REPO_OWNER}/${REPO_NAME} at ${TAG_OR_BRANCH}..."

# 1. Download 'branding' directory to the current directory
echo "Extracting 'branding' directory to current directory..."
# The tar archive contains a root folder named <repo>-<tag_without_v>.
# Example: qlementine-1.4.2
# --strip-components=1 removes this root folder from the extraction path.
curl -sL "${ARCHIVE_URL}" | tar -xz --strip-components=1 "${REPO_NAME}-${TAG_OR_BRANCH#v}/branding"
echo "Successfully extracted 'branding'."

# 2. Download 'lib/resources' to ./lib/
echo "Extracting 'lib/resources' directory to ./lib/ ..."
# Creating the target directory isn't strictly necessary for tar if we are just dropping the folder structure in,
# but it's good practice to ensure the parent exists if we were targeting files specifically.
mkdir -p lib
curl -sL "${ARCHIVE_URL}" | tar -xz --strip-components=1 "${REPO_NAME}-${TAG_OR_BRANCH#v}/lib/resources"
echo "Successfully extracted 'lib/resources'."

# 3. Download 'sandbox/resources' to ./sandbox/
echo "Extracting 'sandbox/resources' directory to ./sandbox/ ..."
mkdir -p sandbox
curl -sL "${ARCHIVE_URL}" | tar -xz --strip-components=1 "${REPO_NAME}-${TAG_OR_BRANCH#v}/sandbox/resources"
echo "Successfully extracted 'sandbox/resources'."

# 4. Download 'showcase/resources' to ./showcase/
echo "Extracting 'showcase/resources' directory to ./showcase/ ..."
mkdir -p showcase
curl -sL "${ARCHIVE_URL}" | tar -xz --strip-components=1 "${REPO_NAME}-${TAG_OR_BRANCH#v}/showcase/resources"
echo "Successfully extracted 'showcase/resources'."

# 5. Download 'docs' to base folder
echo "Extracting 'docs' directory to current directory..."
curl -sL "${ARCHIVE_URL}" | tar -xz --strip-components=1 "${REPO_NAME}-${TAG_OR_BRANCH#v}/docs"
echo "Successfully extracted 'docs'."

echo "Download complete!"
